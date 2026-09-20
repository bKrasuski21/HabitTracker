# Local development build. The release build uses CMake; see CMakeLists.txt.
#
# Sources are globbed rather than listed so this file and CMakeLists.txt cannot
# drift apart -- they did once, and the two builds compiled different programs.

CXX      ?= g++
CXXSTD   := -std=c++20
WARNINGS := -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion \
            -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wdouble-promotion
OPTIMIZE ?= -O2
CXXFLAGS ?= $(CXXSTD) $(WARNINGS) $(OPTIMIZE) -Isrc -MMD -MP
LDLIBS   := -lsfml-graphics -lsfml-window -lsfml-system

BUILD_DIR := build/make
TARGET    := $(BUILD_DIR)/HabitTracker
TEST_BIN  := $(BUILD_DIR)/habittracker_tests

CORE_SRC := $(wildcard src/core/*.cpp)
APP_SRC  := $(wildcard src/app/*.cpp) $(wildcard src/view/*.cpp) src/main.cpp
TEST_SRC := $(wildcard tests/*.cpp)

CORE_OBJ := $(CORE_SRC:%.cpp=$(BUILD_DIR)/%.o)
APP_OBJ  := $(APP_SRC:%.cpp=$(BUILD_DIR)/%.o)
TEST_OBJ := $(TEST_SRC:%.cpp=$(BUILD_DIR)/%.o)
ALL_OBJ  := $(CORE_OBJ) $(APP_OBJ) $(TEST_OBJ)

.PHONY: all app test check run clean
all: app test

app: $(TARGET)
test: check

$(TARGET): $(CORE_OBJ) $(APP_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $^ -o $@ $(LDLIBS)

$(TEST_BIN): $(CORE_OBJ) $(TEST_OBJ)
	@mkdir -p $(dir $@)
	$(CXX) $^ -o $@

# Tests link only the core, so they need no window and no SFML.
check: $(TEST_BIN)
	@./$(TEST_BIN)

run: $(TARGET)
	@./$(TARGET)

$(BUILD_DIR)/tests/%.o: tests/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -Itests -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Only this Makefile's own output; CMake build trees also live under build/.
clean:
	rm -rf $(BUILD_DIR)

# Rebuild any object whose headers changed.
-include $(ALL_OBJ:.o=.d)
