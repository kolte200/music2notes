ifndef DEBUG
DEBUG := 0
endif

OPTCFLAGS := -O3 -march=native
DBGFLAGS := -O0 -g #-fsanitize=address

CC  := gcc -fdiagnostics-color=always
CXX := g++ -fdiagnostics-color=always

CFLAGS   := -Wall -Wextra -Wno-unused-parameter -std=c17 $(shell pkg-config --cflags sdl2)
CXXFLAGS := -Wall -Wextra -Wno-unused-parameter -std=c++20 $(shell pkg-config --cflags sdl2)

LDFLAGS := -Wl,--copy-dt-needed-entries
LDLIBS := $(shell pkg-config --libs sdl2) -lm -lportaudio

ifeq ($(DEBUG),)
DEBUG := 0
endif

ifeq ($(DEBUG),0)
CFLAGS := $(CFLAGS) $(OPTCFLAGS)
CXXFLAGS := $(CXXFLAGS) $(OPTCFLAGS)
LDFLAGS := $(LDFLAGS) $(OPTCFLAGS)
else
CFLAGS := $(CFLAGS) $(DBGFLAGS)
CXXFLAGS := $(CXXFLAGS) $(DBGFLAGS)
LDFLAGS := $(LDFLAGS) $(DBGFLAGS)
endif

SRC_DIR := src
OBJ_DIR := obj
BUILD_DIR := build
TESTS_DIR := tests

OUTPUT := music2notes

INCLUDES := -I$(SRC_DIR)

HDR_FILES := $(shell find $(SRC_DIR)/ -type f -name '*.h')
SRC_FILES := $(shell find $(SRC_DIR)/ -type f -name '*.cpp')
#SRC_FILES := $(filter-out $(SRC_MAINS),$(SRC_FILES))

OBJ_FILES := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_FILES))

all: $(OUTPUT)

docs: doxygen/doxygen.conf $(HDR_FILES)
	doxygen doxygen/doxygen.conf

$(OUTPUT): $(OBJ_FILES)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $(BUILD_DIR)/$@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	rm -rf $(BUILD_DIR)

-include $(patsubst %.o, %.d, $(OBJ_FILES))


# Tests

build/%: $(TESTS_DIR)/%.cpp $(BUILD_DIR) $(OBJ_DIR) $(OBJ_FILES)
	@mkdir -p $(OBJ_DIR)/$(TESTS_DIR)
	@mkdir -p $(BUILD_DIR)/$(TESTS_DIR)
	$(eval OBJ_TEST := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$<))
	$(eval OUT_TEST := $(patsubst build/%,$(BUILD_DIR)/$(TESTS_DIR)/%,$@))
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $(OBJ_TEST)
	$(CXX) $(LDFLAGS) $(OBJ_FILES) $(OBJ_TEST) $(LDLIBS) -o $(OUT_TEST)

run/%:
	$(eval OUT_TEST := $(patsubst run/%,$(BUILD_DIR)/$(TESTS_DIR)/%,$@))
	./$(OUT_TEST) $(ARGS)

test/%: build/%
	$(eval OUT_TEST := $(patsubst test/%,$(BUILD_DIR)/$(TESTS_DIR)/%,$@))
	./$(OUT_TEST) $(ARGS)
