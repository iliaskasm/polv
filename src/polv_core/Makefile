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
WARNINGS := -Wall -Wextra -Wpedantic
PICFLAGS := -fPIC
LDFLAGS ?=
LDLIBS ?=

ROOT_DIR := ../..
INCLUDE_DIR := $(ROOT_DIR)/include

INSTALL_DIR ?= $(ROOT_DIR)/install
INSTALL_INCLUDE_DIR := $(INSTALL_DIR)/include
INSTALL_LIBRARY_DIR := $(INSTALL_DIR)/lib

LOCAL_CPPFLAGS := -I$(INCLUDE_DIR) -I.

OBJS := polv_core.o runtime.o contexts.o memory.o kernels.o

.PHONY: all clean check

all: libpolvcore.so

libpolvcore.so: $(OBJS)
	$(CC) -shared $(LDFLAGS) -Wl,-rpath,'$$ORIGIN' \
	-Wl,-soname,libpolvcore.so -o $@ $^ -lvulkan $(LDLIBS)

polv_core.o: polv_core.c runtime.h contexts.h memory.h kernels.h \
             polv_core_internal.h \
             $(INCLUDE_DIR)/polv_core.h
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(PICFLAGS) -c $< -o $@

runtime.o: runtime.c runtime.h contexts.h kernels.h polv_core_internal.h \
           $(INCLUDE_DIR)/polv_core.h
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(PICFLAGS) -c $< -o $@

contexts.o: contexts.c contexts.h memory.h kernels.h polv_core_internal.h \
            $(INCLUDE_DIR)/polv_core.h
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(PICFLAGS) -c $< -o $@

memory.o: memory.c memory.h runtime.h polv_core_internal.h \
          $(INCLUDE_DIR)/polv_core.h
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(PICFLAGS) -c $< -o $@

kernels.o: kernels.c kernels.h polv_core_internal.h \
           $(INCLUDE_DIR)/polv_core.h
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(PICFLAGS) -c $< -o $@

check:
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) -Werror -fsyntax-only \
		polv_core.c runtime.c contexts.c memory.c kernels.c

clean:
	rm -f $(OBJS) libpolvcore.so