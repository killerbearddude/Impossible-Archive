CXX ?= g++
CXXSTD ?= c++20
BUILD_DIR ?= build
TARGET := impossible_archive_mvp_v28_11
SANITIZE_TARGET := impossible_archive_mvp_v28_11_sanitize
CXXFLAGS ?= -Wall -Wextra -pedantic -O0
CXXFLAGS += -MMD -MP
STRICTFLAGS ?= -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -O0
STRICTFLAGS += -MMD -MP
SANITIZEFLAGS ?= -Wall -Wextra -pedantic -O0 -g -fsanitize=address,undefined -fno-omit-frame-pointer
SANITIZEFLAGS += -MMD -MP

SOURCES := $(wildcard src/*.cpp)
OBJECTS := $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
SANITIZE_BUILD_DIR := build_sanitize
SANITIZE_OBJECTS := $(patsubst src/%.cpp,$(SANITIZE_BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all build test strict sanitize smoke release-check clean

all: build

build: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@

$(BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) -std=$(CXXSTD) $(CXXFLAGS) -c $< -o $@

test: build
	@echo "Running self-test: ./$(TARGET) --self-test"
	./$(TARGET) --self-test

strict:
	$(MAKE) build BUILD_DIR=build_strict CXXFLAGS="$(STRICTFLAGS)"
	@echo "Running strict self-test: ./$(TARGET) --self-test"
	./$(TARGET) --self-test

$(SANITIZE_TARGET): $(SANITIZE_OBJECTS)
	$(CXX) $(SANITIZE_OBJECTS) -fsanitize=address,undefined -o $@

$(SANITIZE_BUILD_DIR)/%.o: src/%.cpp
	@mkdir -p $(SANITIZE_BUILD_DIR)
	$(CXX) -std=$(CXXSTD) $(SANITIZEFLAGS) -c $< -o $@

sanitize: $(SANITIZE_TARGET)
	@echo "Running sanitizer self-test: ./$(SANITIZE_TARGET) --self-test"
	ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 ./$(SANITIZE_TARGET) --self-test

smoke: build
	@echo "Running CLI workflow smoke tests"
	./scripts/smoke_test_cli_workflows.sh

release-check:
	$(MAKE) clean
	$(MAKE) test
	$(MAKE) clean
	$(MAKE) CXXSTD=c++17 test
	$(MAKE) clean
	$(MAKE) strict
	$(MAKE) clean
	$(MAKE) sanitize
	$(MAKE) smoke
	@echo "Release check passed."

-include $(OBJECTS:.o=.d)
-include $(SANITIZE_OBJECTS:.o=.d)

clean:
	rm -rf build build17 build_strict build_sanitize *.o *.d chatgpt-prod-* \
		impossible_archive_mvp_v28_11 impossible_archive_mvp_v28_11_sanitize impossible_archive_mvp_v28_11_cpp17 impossible_archive_mvp_v28_11_strict impossible_archive_mvp_v28_4 impossible_archive_mvp_v28_4_sanitize impossible_archive_mvp_v28_4_cpp17 impossible_archive_mvp_v28_4_strict impossible_archive_mvp_v28_2 impossible_archive_mvp_v28_2_sanitize impossible_archive_mvp_v28_2_cpp17 impossible_archive_mvp_v28_2_strict impossible_archive_mvp_v27_4 impossible_archive_mvp_v27_4_sanitize impossible_archive_mvp_v27_4_cpp17 impossible_archive_mvp_v27_4_strict \
		impossible_archive_mvp_v27_3 impossible_archive_mvp_v27_2 impossible_archive_mvp_v27_1 impossible_archive_mvp_v27 \
		impossible_archive_mvp_v26_6 impossible_archive_mvp_v26_5 impossible_archive_mvp_v26_4 impossible_archive_mvp_v26_3 impossible_archive_mvp_v26_2 impossible_archive_mvp_v26_1 impossible_archive_mvp_v26_0_1 impossible_archive_mvp_v26 impossible_archive_mvp_v26_cpp17 impossible_archive_mvp_v26_strict \
		impossible_archive_mvp_v25_1 impossible_archive_mvp_v25_1_cpp17 impossible_archive_mvp_v25_1_strict impossible_archive_mvp_v25 impossible_archive_mvp_v25_cpp17 impossible_archive_mvp_v25_strict \
		impossible_archive_mvp_v242 impossible_archive_mvp_v242_cpp17 impossible_archive_mvp_v242_strict \
		/tmp/impossible_archive_smoke_*.txt
