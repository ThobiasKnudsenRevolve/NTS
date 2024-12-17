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

    SRC_DIRS += ./external/libcanard/libcanard
    SRC_DIRS += ./external/dsdl/compiled

    # Include paths
    INCLUDE_PATHS += -I./external/libcanard
    INCLUDE_PATHS += -I./external/dsdl/compiled
    INCLUDE_PATHS += -I./external/json/include

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
all: prepare_dirs compile

# Prepare necessary directories
.PHONY: prepare_dirs
prepare_dirs:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

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
