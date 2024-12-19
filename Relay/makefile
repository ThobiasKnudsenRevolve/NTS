
C_COMPILER := gcc
CPP_COMPILER := g++
CFLAGS_COMMON := -Wall
CFLAGS_C :=
CFLAGS_CPP := -std=c++17
LDFLAGS :=
OBJ_DIR := ./obj
BIN_DIR := ./bin
SRC_DIRS := ./src
SRC_FILES :=
PATH_SEPARATOR := /

# Additional Linux-specific settings
INSTALL_CMD := sudo apt-get install -y

# Include paths
INCLUDE_PATHS := -I./include

# Libraries and paths
LIB_PATHS := -L/usr/local/lib
LIBS := -lglfw -lGLEW -lGL -lPcap++ -lPacket++ -lCommon++ -lpcap -lboost_system -lboost_thread -lpthread

# Additional flags
CFLAGS_COMMON += -DNUNAVUT_ASSERT\(x\)=assert\(x\)

SRC_DIRS += ./external/libcanard/libcanard
SRC_DIRS += ./external/dsdl/compiled

# Include paths
INCLUDE_PATHS += -I./external/libcanard
INCLUDE_PATHS += -I./external/dsdl/compiled
INCLUDE_PATHS += -I./external/json/include

# List of main programs (source files with main())
MAINS := main.cpp client.cpp
# Derive binary names by removing the .cpp extension
BIN_NAMES := $(patsubst %.cpp,%,$(MAINS))

# Set VPATH to the directories containing source files
VPATH := $(SRC_DIRS) $(sort $(dir $(SRC_FILES)))

# Collect C and C++ source files (excluding mains)
SRCS_C := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
ALL_SRCS_CPP := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp))
NON_MAIN_SRCS_CPP := $(filter-out $(MAINS),$(ALL_SRCS_CPP))
SRCS := $(SRCS_C) $(NON_MAIN_SRCS_CPP) $(SRC_FILES)

# Object files for non-main sources
OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(SRCS_C)))
OBJS += $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(NON_MAIN_SRCS_CPP)))
OBJS += $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(SRC_FILES)))

# Object files for main sources
MAIN_OBJS := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(MAINS)))

# Combine flags
CFLAGS := $(CFLAGS_COMMON) $(CFLAGS_C) $(INCLUDE_PATHS)
CPPFLAGS := $(CFLAGS_COMMON) $(CFLAGS_CPP) $(INCLUDE_PATHS)
LDFLAGS += $(LIB_PATHS) $(LIBS)

# Default target: build all binaries
.PHONY: all
all: prepare_dirs $(BIN_NAMES:%=$(BIN_DIR)/%)

# Prepare necessary directories
.PHONY: prepare_dirs
prepare_dirs:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(BIN_DIR)

# Pattern rules to compile source files into object files
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(C_COMPILER) $(CFLAGS) -MMD -MF $@.d -c $< -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CPP_COMPILER) $(CPPFLAGS) -MMD -MF $@.d -c $< -o $@

# Specific linking rules for each binary
$(BIN_DIR)/main: $(OBJ_DIR)/main.o $(filter-out $(OBJ_DIR)/client.o, $(OBJS))
	$(CPP_COMPILER) $^ $(LDFLAGS) -o $@

$(BIN_DIR)/client: $(OBJ_DIR)/client.o $(filter-out $(OBJ_DIR)/main.o, $(OBJS))
	$(CPP_COMPILER) $^ $(LDFLAGS) -o $@

# Clean up build artifacts
.PHONY: clean
clean:
	@rm -rf $(OBJ_DIR) $(foreach bin,$(BIN_NAMES),$(BIN_DIR)/$(bin))

# Include dependency files
-include $(OBJS:.o=.d)
-include $(MAIN_OBJS:.o=.d)
