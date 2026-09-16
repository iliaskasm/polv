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
GLSLC ?= glslangValidator

POLV_INSTALL_DIR ?= ../../../install
POLV_LIBRARY_DIR = $(POLV_INSTALL_DIR)/lib
POLV_HEADERS_DIR = $(POLV_INSTALL_DIR)/include

CPPFLAGS = -I$(POLV_HEADERS_DIR) -I..
CFLAGS = -O2 -g -Wall -Wextra -Wpedantic
LDFLAGS = -L$(POLV_LIBRARY_DIR) \
	-Wl,-rpath-link,$(POLV_LIBRARY_DIR) \
	-Wl,-rpath,$(POLV_LIBRARY_DIR)
LDLIBS = -lpolv

COMMON = ../common
TARGET = saxpy
SHADER = saxpy.spv

.PHONY: all clean run

all: $(SHADER) $(TARGET)

$(COMMON).o: $(COMMON).c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(SHADER): saxpy.comp
	$(GLSLC) -V $< -o $@

$(TARGET): $(TARGET).c $(SHADER) $(COMMON).o
	$(CC) $(CPPFLAGS) $(CFLAGS) $< $(COMMON).o $(LDFLAGS) $(LDLIBS) -o $@

run: all
	@printf "[$(TARGET)]: "
	@./$(TARGET)

clean:
	rm -f $(TARGET) $(SHADER)
