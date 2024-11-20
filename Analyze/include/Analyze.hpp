
#pragma once

// Dear ImGui: standalone example application for GLFW + OpenGL 3, using programmable pipeline
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "ImGuiFileDialog.h"

#include <stdio.h>
#include <future>
#include <chrono>
#include <string>
#include <filesystem>
#include <mutex>
#include <atomic>
#include <typeinfo>
#include <cxxabi.h>

#include "DataManager.hpp"
#include "Deserialization.hpp"
#include "PcapReader.hpp"
#include "InfluxDBClient.hpp"

#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
	#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
	#pragma comment(lib, "legacy_stdio_definitions")
#endif

class Analyze {

private:

	GLFWwindow* 				window = nullptr;

	bool 						show_demo_window = true;
    bool 						show_another_window = false;
    ImVec4 						clear_color = ImVec4(0.1f, 0.1f, 0.1f, 1.00f);

	std::vector<std::string> 	pcap_file_paths 		= {};
	DataManager 				data_manager;
	DeserializationMap 			deserialization_map;

	InfluxDBClient  			influxDB_client;

	std::mutex 					pcap_mutex;

public:

	static void glfw_error_callback(int error, const char* description) {
	    fprintf(stderr, "GLFW Error %d: %s\n", error, description);
	}

	Analyze() : influxDB_client("localhost", "8086", "30ffd1384cc64fb7", "MyInitialAdminToken0==") {
		// OpenGL/Metal, GLFW, ImGui and ImPLot
		{
			glfwSetErrorCallback(glfw_error_callback);
		    if (!glfwInit()) {
		    	printf("could not init glfw\n");
		        return;
		    }

		    // Decide GL+GLSL versions
			#if defined(IMGUI_IMPL_OPENGL_ES2)
			    // GL ES 2.0 + GLSL 100
			    const char* glsl_version = "#version 100";
			    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
			    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
			    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
			#elif defined(__APPLE__)
			    // GL 3.2 + GLSL 150
			    const char* glsl_version = "#version 150";
			    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
			    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
			    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
			#else
			    // GL 3.0 + GLSL 130
			    const char* glsl_version = "#version 130";
			    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
			    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
			#endif

		    // Create window with graphics context
		    window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+OpenGL3 example", nullptr, nullptr);
		    if (window == nullptr) {
		    	printf("window == nullptr\n");
		        return;
		    }
		    glfwMakeContextCurrent(window);
		    glfwSwapInterval(1); // Enable vsync

		    // Setup Dear ImGui context
		    IMGUI_CHECKVERSION();
		    ImGui::CreateContext();
		    ImPlot::CreateContext();
		    ImGuiIO& io = ImGui::GetIO(); (void)io;
		    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		    // Setup Dear ImGui style
		    ImGui::Spectrum::StyleColorsSpectrum();

		    ImGui_ImplGlfw_InitForOpenGL(window, true);
		    ImGui_ImplOpenGL3_Init(glsl_version);
		    io.Fonts->Clear();
		  	ImGui::Spectrum::LoadFont();
		}

		// deserialization
	    this->deserialization_map = createDeserializationMap();
	}
	~Analyze() {
		// Cleanup
	    ImPlot::DestroyContext();
	    ImGui_ImplOpenGL3_Shutdown();
	    ImGui_ImplGlfw_Shutdown();
	    ImGui::DestroyContext();

	    glfwDestroyWindow(window);
	    glfwTerminate();
	}

	void start() {

	    while (!glfwWindowShouldClose(window))
	    {
	        glfwPollEvents();

	        // Start the Dear ImGui frame
	        ImGui_ImplOpenGL3_NewFrame();
	        ImGui_ImplGlfw_NewFrame();
	        ImGui::NewFrame();

	        layout();

	        // Rendering
	        ImGui::Render();
	        int display_w, display_h;
	        glfwGetFramebufferSize(window, &display_w, &display_h);
	        glViewport(0, 0, display_w, display_h);
	        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
	        glClear(GL_COLOR_BUFFER_BIT);
	        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	        glfwSwapBuffers(window);
	    }
	}

private:


	void layout() {

		readPcapFile();
		writeToInfluxDB();
		plotting1();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
	}

	void readPcapFile() {
	    static bool reading_pcap_files = false;
	    static std::atomic<bool> stop_flag(false);
	    static std::vector<std::future<void>> futures;

	    if (ImGui::Begin("read pcap files", nullptr)) {
	        // Button to open the file dialog
	        bool selecting_files_dialog = ImGui::Button("Select PCAP files");
	        if (selecting_files_dialog && !reading_pcap_files) {
	            IGFD::FileDialogConfig config;
	            config.countSelectionMax = 0; // Allow unlimited selection
	            ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".pcap", config);
	        }

	        // Start reading files
	        if (!reading_pcap_files) {
	            if (ImGui::Button("Read PCAP files") && !ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
	                reading_pcap_files = true;
	                stop_flag.store(false);
	                futures.clear(); // Clear any existing futures
	                // Start tasks for each PCAP file
	                for (const auto& filePath : pcap_file_paths) {
	                	printf("%s\n", filePath.c_str());
	                    futures.emplace_back(std::async(std::launch::async, [this, filePath, stop_flag]() mutable {
	                        readPcapFileToDataManager(filePath, data_manager, deserialization_map, stop_flag);
	                        
	                        // Thread-safe removal of the file path
	                        {
	                            std::lock_guard<std::mutex> lock(pcap_mutex);
	                            auto it = std::find(pcap_file_paths.begin(), pcap_file_paths.end(), filePath);
	                            if (it != pcap_file_paths.end()) {
	                                pcap_file_paths.erase(it);
	                            }
	                        }
	                    }));
	                }
	            }
	        } else {
	            if (ImGui::Button("Stop Reading")) {
	                stop_flag.store(true);
	                reading_pcap_files = false;
	            }
	        }

	        // Check if all tasks have finished
	        bool all_tasks_done = true;
	        for (auto& f : futures) {
	            if (f.valid()) {
	                auto status = f.wait_for(std::chrono::milliseconds(0));
	                if (status != std::future_status::ready) {
	                    all_tasks_done = false;
	                } else {
	                    try {
	                        f.get();
	                    } catch (const std::exception& e) {
	                        std::cerr << "Exception in task: " << e.what() << std::endl;
	                    }
	                }
	            }
	        }

	        if (all_tasks_done && reading_pcap_files) {
	            reading_pcap_files = false;
	            futures.clear(); // Clean up futures
	            data_manager.PrintMetadata();
	        }

	        // Handle file dialog and display code...
	        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
	            if (ImGuiFileDialog::Instance()->IsOk()) {
	                auto map_files = ImGuiFileDialog::Instance()->GetSelection();
	                for (const auto& [filePathName, filePath] : map_files) {
	                    if (std::find(this->pcap_file_paths.begin(), this->pcap_file_paths.end(), filePath) == this->pcap_file_paths.end()) {
	                        this->pcap_file_paths.push_back(filePath);
	                    }
	                }
	            }
	            ImGuiFileDialog::Instance()->Close();
	        }

	        // Separator for visual clarity
	        ImGui::Separator();
	        ImGui::Text("Selected PCAP Files:");

	        // Begin a table to display the files and remove buttons
	        if (ImGui::BeginTable("pcap_files_table", 2, ImGuiTableFlags_Borders)) {
	            // Setup table columns
	            ImGui::TableSetupColumn("File Name", ImGuiTableColumnFlags_WidthStretch);
	            ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 50.0f);
	            ImGui::TableHeadersRow();

	            // Iterate over the pcap_file_paths vector
	            std::lock_guard<std::mutex> lock(pcap_mutex);
	            for (size_t i = 0; i < this->pcap_file_paths.size(); ++i) {
	                ImGui::TableNextRow();
	                ImGui::TableSetColumnIndex(0);
	                // Extract and display the file name from the full path
	                std::filesystem::path filepath(this->pcap_file_paths[i]);
	                ImGui::Text("%s", filepath.filename().string().c_str());
	                ImGui::TableSetColumnIndex(1);
	                // Create a unique identifier for the remove button
	                std::string button_id = "x##" + std::to_string(i);
	                if (ImGui::Button(button_id.c_str())) {
	                    // Remove the file from the vector if the button is clicked
	                    this->pcap_file_paths.erase(this->pcap_file_paths.begin() + i);
	                    --i; // Adjust the index since we removed an item
	                }
	            }
	            ImGui::EndTable();
	        }
	        ImGui::End();
	    }
	}

	void writeToInfluxDB() {
		if (ImGui::Begin("write to InfluxDB", nullptr)) {
			if (ImGui::Button("write to InfluxDB")) {
				auto channel_ptrs = data_manager.GetCommonMembersChannelPtrs(std::string("CAN_2024-11-20(142000)"));
				printf("channel_ptrs.size() = %ld\n", channel_ptrs.size());
				for (auto channel_ptr : channel_ptrs) {
					channel_ptr->m_car = "Hera";
					channel_ptr->m_driver = "Balin";
				}
				data_manager.writeToInfluxDB(influxDB_client);
			}
			ImGui::End();
		}	
	}

	void plotting1() {
		if (this->pcap_file_paths.size() == 0) {
			auto channel_INS_vx = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.vx");
			auto channel_INS_vy = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.vy");
			auto channel_INS_vz = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.vz");
			auto channel_INS_ax = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.ax");
			auto channel_INS_ay = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.ay");
			auto channel_INS_az = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.az");
			auto channel_INS_roll = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.roll");
			auto channel_INS_pitch = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.pitch");
			auto channel_INS_yaw = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.yaw");
			auto channel_INS_roll_rate = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.roll_rate");
			auto channel_INS_pitch_rate = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.pitch_rate");
			auto channel_INS_yaw_rate = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.yaw_rate");
			auto channel_INS_roll_rate_dt = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.roll_rate_dt");
			auto channel_INS_pitch_rate_dt = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.pitch_rate_dt");
			auto channel_INS_yaw_rate_dt = data_manager.GetChannelPtr<float>("CAN_2024-11-20(142000)", "vcu", "INS.yaw_rate_dt");

			if (ImGui::Begin("INS")) {
			    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
			    ImVec2 plot_size = ImGui::GetContentRegionAvail();
			    if (ImPlot::BeginPlot("INS", plot_size)) {
			        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoLabel, ImPlotAxisFlags_NoLabel);
			        if (channel_INS_vx) { channel_INS_vx->postPlot(); }
			        if (channel_INS_vy) { channel_INS_vy->postPlot(); }
			        if (channel_INS_vz) { channel_INS_vz->postPlot(); }
			        if (channel_INS_ax) { channel_INS_ax->postPlot(); }
			        if (channel_INS_ay) { channel_INS_ay->postPlot(); }
			        if (channel_INS_az) { channel_INS_az->postPlot(); }
			        if (channel_INS_roll) { channel_INS_roll->postPlot(); }
			        if (channel_INS_pitch) { channel_INS_pitch->postPlot(); }
			        if (channel_INS_yaw) { channel_INS_yaw->postPlot(); }
			        if (channel_INS_roll_rate) { channel_INS_roll_rate->postPlot(); }
			        if (channel_INS_pitch_rate) { channel_INS_pitch_rate->postPlot(); }
			        if (channel_INS_yaw_rate) { channel_INS_yaw_rate->postPlot(); }
			        if (channel_INS_roll_rate_dt) { channel_INS_roll_rate_dt->postPlot(); }
			        if (channel_INS_pitch_rate_dt) { channel_INS_pitch_rate_dt->postPlot(); }
			        if (channel_INS_yaw_rate_dt) { channel_INS_yaw_rate_dt->postPlot(); }

			        ImPlot::EndPlot();
			    }
			    ImPlot::PopStyleVar();
			    ImGui::PopStyleVar();
			    ImGui::End();
			}

		}
		else {
			printf("cannot plot data while not having read pcap files\n");
			return;
		}
	}

};
