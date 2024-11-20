# Detect the operating system
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
    OS := Linux
else ifeq ($(OS),Windows_NT)
    OS := Windows
else
    OS := Unknown
endif

# Default configurations
ifeq ($(OS),Linux)
    C_COMPILER := gcc
    CPP_COMPILER := g++
    CFLAGS_COMMON := -Wall
    CFLAGS_C :=
    CFLAGS_CPP := -std=c++17
    LDFLAGS :=
    OBJ_DIR := ./obj
    BIN_DIR := ./bin
    BIN_NAME := main
    SRC_DIRS := ./src
    SRC_FILES :=
    PATH_SEPARATOR := /

    # Additional Linux-specific settings
    INSTALL_CMD := sudo apt-get install -y

    # Include paths
    INCLUDE_PATHS := -I./include

    # Libraries and paths
    LIB_PATHS := -L/usr/local/lib
    LIBS := -lglfw -lGLEW -lGL -lPcap++ -lPacket++ -lCommon++ -lpcap -lboost_system -lboost_thread -lpthread #-lwebsocketpp

    # Additional flags
    CFLAGS_COMMON += -DNUNAVUT_ASSERT\(x\)=assert\(x\)

    # Source directories
    SRC_DIRS += ./external/imgui
    # Do NOT include ./external/imgui/backends in SRC_DIRS

    SRC_DIRS += ./external/implot
    SRC_DIRS += ./external/ImGuiFileDialog
    SRC_DIRS += ./external/libcanard/libcanard
    SRC_DIRS += ./external/dsdl/compiled

    # Include paths
    INCLUDE_PATHS += -I./external/imgui -I./external/imgui/backends
    INCLUDE_PATHS += -I./external/implot
    INCLUDE_PATHS += -I./external/ImGuiFileDialog
    INCLUDE_PATHS += -I./external/libcanard
    INCLUDE_PATHS += -I./external/dsdl/compiled
    INCLUDE_PATHS += -I./external/json/include

    # Source files (only specific backend files)
    SRC_FILES += ./external/imgui/backends/imgui_impl_glfw.cpp
    SRC_FILES += ./external/imgui/backends/imgui_impl_opengl3.cpp

else ifeq ($(OS),Windows)
    # Assume Windows settings
    C_COMPILER := gcc
    CPP_COMPILER := g++
    CFLAGS_COMMON := -Wall
    CFLAGS_C :=
    CFLAGS_CPP := -std=c++17
    LDFLAGS :=
    OBJ_DIR := .\obj
    BIN_DIR := .\bin
    BIN_NAME := main
    SRC_DIRS := .\src
    SRC_FILES :=
    PATH_SEPARATOR := \

    # Additional Windows-specific settings
    INSTALL_CMD := choco install -y

    # Paths to external libraries (adjust these paths based on your directory structure)
    EXTERNAL_DIR := $(CURDIR)\external

    # Include paths
    INCLUDE_PATHS := -I$(EXTERNAL_DIR)\include

    # OpenGL library settings
    INCLUDE_PATHS += -I$(EXTERNAL_DIR)\glfw\glfw-3.3.10.bin.WIN64\include
    INCLUDE_PATHS += -I$(EXTERNAL_DIR)\glew\glew-2.1.0\include
    LIB_PATHS += -L$(EXTERNAL_DIR)\glfw\glfw-3.3.10.bin.WIN64\lib-mingw-w64
    LIB_PATHS += -L$(EXTERNAL_DIR)\glew\glew-2.1.0\lib\Release\x64
    LIBS += -lglew32 -lglfw3 -lopengl32 -lgdi32

    # ImGui settings
    INCLUDE_PATHS += -I$(EXTERNAL_DIR)\imgui
    INCLUDE_PATHS += -I$(EXTERNAL_DIR)\imgui\backends
    SRC_DIRS += $(EXTERNAL_DIR)\imgui
    SRC_FILES += $(EXTERNAL_DIR)\imgui\backends\imgui_impl_glfw.cpp
    SRC_FILES += $(EXTERNAL_DIR)\imgui\backends\imgui_impl_opengl3.cpp

    # ImPlot settings
    INCLUDE_PATHS += -I$(EXTERNAL_DIR)\implot
    SRC_DIRS += $(EXTERNAL_DIR)\implot

    # JSON library
    INCLUDE_PATHS += -I$(EXTERNAL_DIR)\json\include

    # WebSocket++ and Boost libraries (installed via vcpkg)
    INCLUDE_PATHS += -I$(EXTERNAL_DIR)\vcpkg\installed\x64-windows-static\include
    LIB_PATHS += -L$(EXTERNAL_DIR)\vcpkg\installed\x64-windows-static\lib
    LIBS += -lboost_system-vc143-mt-x64-1_86
    LIBS += -lboost_context-vc143-mt-x64-1_86
    LIBS += -lboost_coroutine-vc143-mt-x64-1_86
    LIBS += -lboost_thread-vc143-mt-x64-1_86
    LIBS += -lws2_32

    # Error out since Windows support is not fully implemented
    $(error Windows support is not fully implemented in this Makefile)
else
    $(error Unsupported OS)
endif

# Set VPATH to the directories containing source files
# Include SRC_DIRS and directories of SRC_FILES
VPATH := $(SRC_DIRS) $(sort $(dir $(SRC_FILES)))

# Collect source files from specified directories (non-recursive)
SRCS_C := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
SRCS_CPP := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
SRCS := $(SRCS_C) $(SRCS_CPP) $(SRC_FILES)

# Object files mapping: obj/filename.o
# Ensure that each object file is named after its source file without directory paths
OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRCS_C)))
OBJS += $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(SRCS_CPP)))
OBJS += $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(notdir $(SRC_FILES))))

# Combine flags
CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_C) $(INCLUDE_PATHS)
CPPFLAGS := $(CFLAGS_COMMON) $(CFLAGS_CPP) $(INCLUDE_PATHS)
LDFLAGS += $(LIB_PATHS) $(LIBS)

# Default target
.PHONY: all
all: prepare_dirs install_tools install_libraries compile

# Prepare necessary directories
.PHONY: prepare_dirs
prepare_dirs:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# Install necessary tools
.PHONY: install_tools
install_tools:
ifeq ($(OS),Linux)
	@which gcc >/dev/null 2>&1 || ($(INSTALL_CMD) build-essential)
	@which g++ >/dev/null 2>&1 || ($(INSTALL_CMD) g++)
	@which git >/dev/null 2>&1 || ($(INSTALL_CMD) git)
	@which make >/dev/null 2>&1 || ($(INSTALL_CMD) make)
	@which cmake >/dev/null 2>&1 || ($(INSTALL_CMD) cmake)
	@which curl >/dev/null 2>&1 || ($(INSTALL_CMD) curl)
else ifeq ($(OS),Windows)
	@if not exist "$(C_COMPILER)" ($(INSTALL_CMD) mingw)
	@if not exist "$(CPP_COMPILER)" ($(INSTALL_CMD) mingw)
	@if not exist "$(EXTERNAL_DIR)$(PATH_SEPARATOR)git.exe" ($(INSTALL_CMD) git)
	@if not exist "$(EXTERNAL_DIR)$(PATH_SEPARATOR)cmake.exe" ($(INSTALL_CMD) cmake)
	@if not exist "$(EXTERNAL_DIR)$(PATH_SEPARATOR)curl.exe" ($(INSTALL_CMD) curl)
endif

# Install necessary libraries
.PHONY: install_libraries
install_libraries: install_opengl install_imgui install_implot install_json install_websocketpp

.PHONY: install_opengl
install_opengl:
ifeq ($(OS),Linux)
	@dpkg -s libglfw3-dev libglew-dev >/dev/null 2>&1 || ($(INSTALL_CMD) libglfw3-dev libglew-dev)
else ifeq ($(OS),Windows)
	@echo "Installing OpenGL libraries..."
	# Commands to download and set up GLFW and GLEW
	# Assuming you have scripts or commands to set up these libraries
endif

.PHONY: install_imgui
install_imgui:
	@if [ ! -d external/imgui ]; then git clone https://github.com/ocornut/imgui.git external/imgui; fi

.PHONY: install_implot
install_implot:
	@if [ ! -d external/implot ]; then git clone https://github.com/epezent/implot.git external/implot; fi

ifeq ($(OS),Linux)
.PHONY: install_ImGuiFileDialog
install_ImGuiFileDialog:
	@if [ ! -d external/ImGuiFileDialog ]; then git clone https://github.com/aiekick/ImGuiFileDialog.git external/ImGuiFileDialog; fi
else
install_ImGuiFileDialog:
	@echo "ImGuiFileDialog is not supported on Windows."
endif

ifeq ($(OS),Linux)
.PHONY: install_libcanard
install_libcanard:
	@if [ ! -d external/libcanard ]; then git clone https://github.com/OpenCyphal/libcanard.git external/libcanard; fi
else
install_libcanard:
	@echo "libcanard is not supported on Windows."
endif

ifeq ($(OS),Linux)
.PHONY: install_PcapPlusPlus
install_PcapPlusPlus:
	@if [ ! -d external/PcapPlusPlus ]; then \
		git clone https://github.com/seladb/PcapPlusPlus.git external/PcapPlusPlus && \
		cd external/PcapPlusPlus && \
		mkdir build && cd build && \
		cmake .. && \
		make -j$(nproc) && \
		sudo make install; \
	fi
else
install_PcapPlusPlus:
	@echo "PcapPlusPlus is not supported on Windows."
endif

ifeq ($(OS),Linux)
.PHONY: install_dsdl
install_dsdl:
	@if [ ! -d external/dsdl ]; then \
		git clone https://github.com/RevolveNTNU/dsdl-definitions.git external/dsdl && \
		cd external/dsdl && \
		python3 -m venv .venvs/nunavut && \
		. .venvs/nunavut/bin/activate && \
		pip install --upgrade pip && \
		pip install nunavut && \
		PATH=.venvs/nunavut/bin:$$PATH python scripts/parse_dsdl_to_c.py; \
	fi
else
install_dsdl:
	@echo "dsdl is not supported on Windows."
endif

.PHONY: install_json
install_json:
	@if [ ! -d external/json ]; then git clone https://github.com/nlohmann/json.git external/json; fi

ifeq ($(OS),Linux)
.PHONY: install_websocketpp
install_websocketpp:
	@dpkg -s libwebsocketpp-dev >/dev/null 2>&1 || ($(INSTALL_CMD) libwebsocketpp-dev libboost-all-dev)
else ifeq ($(OS),Windows)
install_websocketpp:
	@echo "Installing WebSocket++ via vcpkg..."
	# Commands to install WebSocket++ and Boost via vcpkg
endif

ifeq ($(OS),Linux)
.PHONY: install_asio
install_asio:
	@dpkg -s libasio-dev >/dev/null 2>&1 || ($(INSTALL_CMD) libasio-dev)
else
install_asio:
	@echo "Asio is not supported on Windows via this Makefile."
endif


# Compile source files
.PHONY: compile
compile: $(BIN_DIR)/$(BIN_NAME)

# Pattern rules to compile source files into object files
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(C_COMPILER) $(CFLAGS) -MMD -MF $@.d -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CPP_COMPILER) $(CPPFLAGS) -MMD -MF $@.d -c $< -o $@

# Link object files into the final executable
$(BIN_DIR)/$(BIN_NAME): $(OBJS)
	$(CPP_COMPILER) $^ $(LDFLAGS) -o $@

# Clean up build artifacts
.PHONY: clean
clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)/$(BIN_NAME)

# Include dependency files
-include $(OBJS:.o=.d)
