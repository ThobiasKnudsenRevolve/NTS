#include "../include/Analyze.hpp"
#include "../include/InfluxDBClient.hpp"

#include <chrono>

int main() {

	time_t timestamp = convertLogIdToTimestampMs("CAN_2024-11-20(142000)");
	printf("timestamp = %ld\n", timestamp);

	InfluxDBClient client("localhost", "8086", "30ffd1384cc64fb7", "MyInitialAdminToken0==");
    std::string timestamp_str_ns = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
	client.postToInfluxDB("test", "INS.vx,location=office value=23.5 " + timestamp_str_ns + "\nINS.vy,location=office value=24.5 " + timestamp_str_ns);
	Analyze analyze;
	analyze.start();
	return 0;
}