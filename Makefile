# Convenience wrapper. Prefer: cmake -S . -B build -G Ninja && cmake --build build

BUILD_DIR ?= build
CMAKE ?= cmake
GEN ?= Ninja

.PHONY: build test run format lint sanitize clean

build:
	$(CMAKE) -S . -B $(BUILD_DIR) -G $(GEN) -DCMAKE_BUILD_TYPE=Debug -DGB_BUILD_TESTS=ON
	$(CMAKE) --build $(BUILD_DIR)

test: build
	$(BUILD_DIR)/gb_tests

run: build
	@echo "usage: make run ROM=path/to.gb"
	$(BUILD_DIR)/gb_desktop $(ROM)

format:
	./tools/format.sh

lint:
	clang-tidy -p $(BUILD_DIR) src/**/*.c -- || true
	cppcheck --enable=warning,style --std=c17 -I include src || true

sanitize:
	./tools/run_sanitizers.sh

clean:
	rm -rf $(BUILD_DIR)
