BUILD_DIR ?= build
BUILD_TYPE ?= Release

.PHONY: all configure configure-debug build clean run

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

configure-debug:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

build: configure
	cmake --build $(BUILD_DIR)

clean:
	cmake --build $(BUILD_DIR) --target clean

run: build
	./$(BUILD_DIR)/mime-settings
