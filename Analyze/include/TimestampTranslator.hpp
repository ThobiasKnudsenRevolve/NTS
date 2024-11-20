#pragma once

#include "rclcpp/rclcpp.hpp"
#include <cstdint>
#include <mutex>
#include <chrono>

// Thread safe helper for translating between different time references.
class TimestampTranslator
{
public:
  enum class Method {
    // Assume zero delay in communication protocols, and use system clock for timestamping
    kPrimitive = 0,
    /**
     * @brief Turn GPS time into system (unix) timestamps by using a known offset. Assumes that the system time is
     * set/synchronized to the correct unix time.
     */
    kGpsToUnix = 1,
    /**
     * @brief Translate sime since PPS signal into system time, assuming zero communication delay. However, all
     * timestamps based on PPS with the same pulse source should be correct relative to each other with PPS precision if
     * the system offest and counting is started with the same system time offset.
     */
    kPpsToSystemClock = 2,
  };

  struct Parameters {
    // Difference between TAI and UTC in seconds. This will change unpredictably, yet slowly, and is announcedby an
    // official organization. An updated list can be found here:
    // https://hpiers.obspm.fr/eoppc/bul/bulc/ntp/leap-seconds.list
    std::chrono::seconds dtai;

    // When calling `updateOffsets`, the time wil be updated every `update_period`th call
    unsigned int update_period{ 1 };

    Method method;
  };

  /**
   * @brief Evaluate the nanosecond offset between GPS and UNIX timestamps.
   *
   * @details For a thorough overview of relevant time standards, read
   * https://geometrian.com/programming/reference/timestds/index.php.
   * Other material exists, but is often contradictory.
   */
  TimestampTranslator(Parameters) : params_{ params } {
    // https://en.wikipedia.org/wiki/Time_standard#Conversions
    std::chrono::seconds gps_to_tai_offset{ 19 };
    // https://stackoverflow.com/a/20528332
    std::chrono::seconds gps_unix_epoch_offset{ 315964800 };
    gps_to_unix_offset_ = gps_unix_epoch_offset + gps_to_tai_offset - params.dtai;
  }

  /**
   * @brief Try to update PPS pulse phase (time within second) and VCU startup time. It is assumed that the three
   * timestamps are from the same point in time.
   *
   * @param gps_time as per the GPS time standard
   * @param time_since_pps_pulse time since the last PPS pulse (Pulse Per Second synchronization protocol)
   * @param time_since_vcu_startup VCU timestamp format, which starts at zero when the VCU is reset
   * @return true if the values were updated,
   */
  bool updateOffsets(std::chrono::nanoseconds gps_time, std::chrono::nanoseconds time_since_pps_pulse,
                     std::chrono::microseconds time_since_vcu_startup) {
    update_counter_mutex_.lock();
    bool updating{ update_counter_ == 0 };
    if (updating)
    {
      vcu_startup_time_mutex_.lock();
      pps_mutex_.lock();
      pps_pulse_phase_ = gps_time - std::chrono::floor<std::chrono::microseconds>(gps_time - pps_time);
      pps_mutex_.unlock();
      if (params_.method == Method::kGpsToUnix)
      {
        vcu_startup_time_ = gps_time + gps_to_unix_offset_ - time_since_vcu_startup;
      }
      else if (params_.method == Method::kPpsToSystemClock)
      {
        vcu_startup_time_ =
            std::chrono::nanoseconds{ std::chrono::system_clock::now().time_since_epoch() } - time_since_vcu_startup;
      }
      vcu_startup_time_mutex_.unlock();
    }
    update_counter_ = (update_counter_ + 1) % params_.update_period;
    update_counter_mutex_.unlock();

    return updating;
  }

  /**
   * @brief Update VCU startup time by the assumption that there is no delay in the communication between the VCU and
   * PU, and that their clocks are synchronized.
   *
   * @param time_since_vcu_startup VCU timestamp format, which starts at zero when the VCU is reset
   */
  void updatePrimitiveOffsets(std::chrono::microseconds time_since_vcu_startup) {
    if (params_.method == Method::kPrimitive)
    {
      vcu_startup_time_mutex_.lock();
      vcu_startup_time_ =
          std::chrono::nanoseconds{ std::chrono::system_clock::now().time_since_epoch() } - time_since_vcu_startup;
      vcu_startup_time_mutex_.unlock();
    }
  }
  std::chrono::microseconds getMicrosecondsSinceVcuStartup(void) {
    std::chrono::nanoseconds now{ std::chrono::system_clock::now().time_since_epoch() };
    vcu_startup_time_mutex_.lock();
    std::chrono::microseconds out{ std::chrono::duration_cast<std::chrono::microseconds>(now - vcu_startup_time_) };
    vcu_startup_time_mutex_.unlock();
    return out;
  }
  std::chrono::nanoseconds getPpsPhase(void) {
    std::scoped_lock(pps_mutex_);
    return pps_pulse_phase_;
  }
  /**
   * @brief Should be done when there is around half a second since the last pulse, for stable results. If there are
   * multiple clients resetting their PPS counter, the return value of this function is meant to be used such that each
   * client uses the same time of reset.
   *
   * @param time_of_reset optional, the time with which the reset should be associated. If it is 0, time of reset will
   * be evaluated based on PPS timestamp history.
   * @return the time with which the reset is associated
   */
  std::chrono::nanoseconds resetPpsSecondCounter(std::chrono::nanoseconds time_of_reset = std::chrono::nanoseconds{0}) {
    pps_mutex_.lock();
    latest_pps_time_ -= pps_second_counter_;
    if (time_of_reset == 0ns)
    {
      time_of_reset = std::chrono::system_clock::now().time_since_epoch() - latest_pps_time_;
    }
    time_of_last_pps_count_reset_ = time_of_reset;
    pps_second_counter_ = 0s;
    time_of_last_pps_count_ = time_of_reset - 500ms;
    pps_mutex_.unlock();
    if (latest_pps_time_ == 0ns)
    {
      /**
       * @brief This should not have been called near PPS time 0, so this is not realistic. In this case, then, no PPS
       * time has been received. So it is up to a potential requester to set their own time of reset.
       */
      return 0ns;
    }
    return time_of_reset;
  }

  // writing out full std::chrono namespace in signature to avoid doxygen warning
  rclcpp::Time getRosTime(std::chrono::nanoseconds gps_time, std::chrono::nanoseconds pps_time) {
    if (params_.method == Method::kPpsToSystemClock)
    {
      pps_mutex_.lock();

      std::chrono::nanoseconds abs_pps_time{ pps_second_counter_ + pps_time };
      std::chrono::nanoseconds pps_difference{ latest_pps_time_ - abs_pps_time };
      if (pps_difference > 500ms)
      {
        std::chrono::nanoseconds now{ std::chrono::system_clock::now().time_since_epoch() };
        if (now - time_of_last_pps_count_ > 500ms)
        {
          pps_second_counter_++;
          time_of_last_pps_count_ = now;
          abs_pps_time += 1s;
        }
      }
      else if (pps_difference < -500ms)
      {
        abs_pps_time -= 1s;
      }

      latest_pps_time_ = abs_pps_time;
      std::chrono::nanoseconds out{ time_of_last_pps_count_reset_ + abs_pps_time };

      pps_mutex_.unlock();
      return rclcpp::Time{ out.count() };
    }
    else if (params_.method == Method::kGpsToUnix)
    {
      vcu_startup_time_mutex_.lock();
      std::chrono::nanoseconds time{ gps_time + gps_to_unix_offset_ };
      vcu_startup_time_mutex_.unlock();
      return rclcpp::Time{ time.count() };
    }

    return rclcpp::Clock{}.now();
  }

  // writing out full std::chrono namespace in signature to avoid doxygen warning
  rclcpp::Time getRosTime(std::chrono::microseconds time_since_vcu_startup) {
    if (params_.method == Method::kPrimitive){
      return rclcpp::Clock{}.now();
    }
    vcu_startup_time_mutex_.lock();
    std::chrono::nanoseconds time{ vcu_startup_time_ + time_since_vcu_startup };
    vcu_startup_time_mutex_.unlock();
    return rclcpp::Time{ time.count() };
  }

private:
  Parameters params_;

  std::chrono::nanoseconds pps_pulse_phase_;
  std::chrono::seconds pps_second_counter_{};
  std::chrono::nanoseconds latest_pps_time_{};
  std::chrono::nanoseconds time_of_last_pps_count_{};
  std::chrono::nanoseconds time_of_last_pps_count_reset_{};
  std::mutex pps_mutex_;

  std::chrono::nanoseconds gps_to_unix_offset_;
  std::chrono::nanoseconds vcu_startup_time_{};
  std::mutex vcu_startup_time_mutex_;

  std::mutex update_counter_mutex_;
  unsigned int update_counter_{ 0 };
};
