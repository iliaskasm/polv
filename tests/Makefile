# MIT License
#
# Copyright (c) 2026 Ilias K. Kasmeridis
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

CC ?= cc
GLSLC ?= glslangValidator

POLV_INSTALL_DIR ?= ../install
POLV_HEADERS_DIR := $(POLV_INSTALL_DIR)/include
POLV_LIBRARY_DIR := $(POLV_INSTALL_DIR)/lib

BUILD_DIR := build
KERNEL_BUILD_DIR := $(BUILD_DIR)/kernels
COMMON_OBJ := $(BUILD_DIR)/common/test.o

CPPFLAGS += -I$(POLV_HEADERS_DIR) -Icommon \
	-DPOLV_TEST_KERNEL_DIR=\"$(abspath $(KERNEL_BUILD_DIR))\"

CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic

LDFLAGS += -L$(POLV_LIBRARY_DIR) \
	-Wl,-rpath-link,$(abspath $(POLV_LIBRARY_DIR)) \
	-Wl,-rpath,$(abspath $(POLV_LIBRARY_DIR))

LDLIBS += -lpolv

API_SOURCES := $(sort $(wildcard api/test_*.c))
MEMORY_SOURCES := $(sort $(wildcard memory/test_*.c))
KERNEL_TEST_SOURCES := $(sort $(wildcard kernels/test_*.c))
LIFECYCLE_SOURCES := $(sort $(wildcard lifecycle/test_*.c))

TEST_SOURCES := $(API_SOURCES) $(MEMORY_SOURCES) $(KERNEL_TEST_SOURCES)	$(LIFECYCLE_SOURCES)

API_TARGETS := $(patsubst %.c,$(BUILD_DIR)/%,$(API_SOURCES))
MEMORY_TARGETS := $(patsubst %.c,$(BUILD_DIR)/%,$(MEMORY_SOURCES))
KERNEL_TEST_TARGETS := $(patsubst %.c,$(BUILD_DIR)/%,$(KERNEL_TEST_SOURCES))
LIFECYCLE_TARGETS := $(patsubst %.c,$(BUILD_DIR)/%,$(LIFECYCLE_SOURCES))

TEST_TARGETS := $(patsubst %.c,$(BUILD_DIR)/%,$(TEST_SOURCES))

SHADER_SOURCES := $(wildcard common/kernels/*.comp)
SHADER_TARGETS := $(patsubst common/kernels/%.comp,$(KERNEL_BUILD_DIR)/%.spv,$(SHADER_SOURCES))

MEMORY_SHADER_TARGETS := \
	$(KERNEL_BUILD_DIR)/init.spv $(KERNEL_BUILD_DIR)/compare_src_dst.spv \
	 $(KERNEL_BUILD_DIR)/compare_pattern.spv $(KERNEL_BUILD_DIR)/copy.spv

LIFECYCLE_SHADER_TARGETS := \
	$(KERNEL_BUILD_DIR)/increment.spv

.PHONY: \
	all \
	test \
	test_api \
	test_memory \
	test_kernels \
	test_lifecycle \
	run \
	check \
	clean

all: $(SHADER_TARGETS) $(TEST_TARGETS)

$(COMMON_OBJ): common/test.c common/test.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%: %.c $(COMMON_OBJ) common/test.h
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(COMMON_OBJ) \
		$(LDFLAGS) $(LDLIBS) -o $@

$(KERNEL_BUILD_DIR)/%.spv: common/kernels/%.comp
	@mkdir -p $(dir $@)
	$(GLSLC) -V $< -o $@

test_api: $(API_TARGETS)
	@passed=0; failed=0; total=0; \
	for test in $(API_TARGETS); do \
		total=$$((total + 1)); \
		if "$$test"; then \
			passed=$$((passed + 1)); \
		else \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	printf "\nPOLV API tests: %d passed, %d failed, %d total\n" \
		$$passed $$failed $$total; \
	test $$failed -eq 0

test_memory: $(MEMORY_SHADER_TARGETS) $(MEMORY_TARGETS)
	@passed=0; failed=0; total=0; \
	for test in $(MEMORY_TARGETS); do \
		total=$$((total + 1)); \
		if "$$test"; then \
			passed=$$((passed + 1)); \
		else \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	printf "\nPOLV memory tests: %d passed, %d failed, %d total\n" \
		$$passed $$failed $$total; \
	test $$failed -eq 0

test_kernels: $(SHADER_TARGETS) $(KERNEL_TEST_TARGETS)
	@passed=0; failed=0; total=0; \
	for test in $(KERNEL_TEST_TARGETS); do \
		total=$$((total + 1)); \
		if "$$test"; then \
			passed=$$((passed + 1)); \
		else \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	printf "\nPOLV kernel tests: %d passed, %d failed, %d total\n" \
		$$passed $$failed $$total; \
	test $$failed -eq 0

test_lifecycle: $(LIFECYCLE_SHADER_TARGETS) $(LIFECYCLE_TARGETS)
	@passed=0; failed=0; total=0; \
	for test in $(LIFECYCLE_TARGETS); do \
		total=$$((total + 1)); \
		if "$$test"; then \
			passed=$$((passed + 1)); \
		else \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	printf "\nPOLV lifecycle tests: %d passed, %d failed, %d total\n" \
		$$passed $$failed $$total; \
	test $$failed -eq 0

test: all
	@passed=0; failed=0; total=0; \
	for test in $(TEST_TARGETS); do \
		total=$$((total + 1)); \
		if "$$test"; then \
			passed=$$((passed + 1)); \
		else \
			failed=$$((failed + 1)); \
		fi; \
	done; \
	printf "\nPOLV tests: %d passed, %d failed, %d total\n" \
		$$passed $$failed $$total; \
	test $$failed -eq 0

run check: test

clean:
	rm -rf $(BUILD_DIR)