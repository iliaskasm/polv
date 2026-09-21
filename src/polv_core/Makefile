#
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
# 
CC ?= cc
CFLAGS ?= -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
LDLIBS ?=

ROOT_DIR := ../..
INCLUDE_DIR := $(ROOT_DIR)/include

BUILD_DIR ?= $(abspath $(ROOT_DIR)/build/polv_core)

TARGET := $(BUILD_DIR)/libpolvcore.so

WARNINGS := -Wall -Wextra -Wpedantic
PICFLAGS := -fPIC
DEPFLAGS := -MMD -MP

LOCAL_CPPFLAGS := -I$(INCLUDE_DIR) -I.

SOURCES := $(wildcard *.c)
OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)

.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) -shared $(LDFLAGS) \
		-Wl,-soname,libpolvcore.so \
		-Wl,-rpath,'$$ORIGIN' \
		-o $@ $(OBJECTS) \
		-lvulkan $(LDLIBS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) \
		$(WARNINGS) $(PICFLAGS) $(DEPFLAGS) \
		-c $< -o $@

check:
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) \
		$(WARNINGS) -Werror -fsyntax-only $(SOURCES)

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)