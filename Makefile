.PHONY: all build clean configure flash

BUILD_PRESET ?= Debug
BUILD_DIR := build/$(BUILD_PRESET)

all: build

configure:
	cmake --fresh --preset $(BUILD_PRESET)

build: configure
	cmake --build --preset $(BUILD_PRESET)

clean:
	cmake --build --preset $(BUILD_PRESET) --target clean

flash: build
	@echo "Flash command is project-specific. Add OpenOCD, pyOCD, or STM32CubeProgrammer invocation here."
