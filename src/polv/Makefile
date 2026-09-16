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
POLV_CORE_DIR := ../polv_core

INSTALL_DIR ?= $(ROOT_DIR)/install
INSTALL_INCLUDE_DIR := $(INSTALL_DIR)/include
INSTALL_LIBRARY_DIR := $(INSTALL_DIR)/lib

LOCAL_CPPFLAGS := -I$(INCLUDE_DIR) -I.

OBJS := polv.o contexts.o kernels.o

.PHONY: all clean polvcore check

all: libpolv.so

polvcore:
	$(MAKE) -C $(POLV_CORE_DIR)


libpolv.so: $(OBJS) polvcore
	$(CC) -shared $(LDFLAGS) -Wl,-soname,libpolv.so -o $@ \
		$(OBJS) -L$(POLV_CORE_DIR) -lpolvcore -Wl,-rpath,'$$ORIGIN' $(LDLIBS)


polv.o: polv.c contexts.h kernels.h polv_internal.h \
        $(INCLUDE_DIR)/polv.h $(INCLUDE_DIR)/polv_core.h
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(PICFLAGS) -c $< -o $@

contexts.o: contexts.c contexts.h polv_internal.h \
            $(INCLUDE_DIR)/polv.h $(INCLUDE_DIR)/polv_core.h
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(PICFLAGS) -c $< -o $@

kernels.o: kernels.c kernels.h contexts.h polv_internal.h \
           $(INCLUDE_DIR)/polv.h $(INCLUDE_DIR)/polv_core.h
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) $(PICFLAGS) -c $< -o $@

check:
	$(CC) $(CPPFLAGS) $(LOCAL_CPPFLAGS) $(CFLAGS) $(WARNINGS) -Werror -fsyntax-only \
		polv.c contexts.c kernels.c 

clean:
	rm -f $(OBJS) libpolv.so