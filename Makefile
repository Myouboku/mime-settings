BUILD_DIR ?= build
BUILD_TYPE ?= Release

.PHONY: all configure configure-debug build clean run format

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

configure-debug:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

build: configure
	cmake --build $(BUILD_DIR)
	sudo mkdir -p /opt/mime-settings
	sudo cp $(BUILD_DIR)/mime-settings /opt/mime-settings/
	sudo cp mime-settings.desktop /usr/share/applications/
	sudo update-desktop-database /usr/share/applications

clean:
	cmake --build $(BUILD_DIR) --target clean

run: build
	./$(BUILD_DIR)/mime-settings

FORMAT_FILES := $(shell find src -type f \( -name '*.cpp' -o -name '*.h' \))

format:
	clang-format -i $(FORMAT_FILES)
