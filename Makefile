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

POLV_DIR := src/polv
POLV_CORE_DIR := src/polv_core

PACKAGE := polv

VERSION_FILE := VERSION
VERSION := $(strip $(shell cat $(VERSION_FILE)))
VERSION_PARTS := $(subst ., ,$(VERSION))
VERSION_MAJOR := $(word 1,$(VERSION_PARTS))
VERSION_MINOR := $(word 2,$(VERSION_PARTS))
VERSION_PATCH := $(word 3,$(VERSION_PARTS))
VERSION_HEADER := include/polv_version.h

DIST_NAME := $(PACKAGE)-$(VERSION)
DIST_ARCHIVE := $(DIST_NAME).tar.gz
DIST_FILES := Makefile README.md LICENSE VERSION include src samples

INSTALL_DIR ?= $(CURDIR)/install
INSTALL_INCLUDE_DIR := $(INSTALL_DIR)/include
INSTALL_LIBRARY_DIR := $(INSTALL_DIR)/lib

SAMPLES_DIR := $(CURDIR)/samples

.PHONY: all clean polvcore polv install uninstall check dist version-header

all: polvcore polv

polvcore: version-header
	$(MAKE) -C $(POLV_CORE_DIR)

polv: version-header polvcore
	$(MAKE) -C $(POLV_DIR)

install: all
	mkdir -p $(INSTALL_INCLUDE_DIR) $(INSTALL_LIBRARY_DIR)

	install -m 644 include/polv_core.h \
		$(INSTALL_INCLUDE_DIR)/polv_core.h

	install -m 644 include/polv.h \
		$(INSTALL_INCLUDE_DIR)/polv.h

	install -m 644 include/polv_version.h \
		$(INSTALL_INCLUDE_DIR)/polv_version.h

	install -m 755 $(POLV_CORE_DIR)/libpolvcore.so \
		$(INSTALL_LIBRARY_DIR)/libpolvcore.so

	install -m 755 $(POLV_DIR)/libpolv.so \
		$(INSTALL_LIBRARY_DIR)/libpolv.so

uninstall:
	rm -f $(INSTALL_INCLUDE_DIR)/polv_core.h
	rm -f $(INSTALL_INCLUDE_DIR)/polv.h
	rm -f $(INSTALL_INCLUDE_DIR)/polv_version.h
	rm -f $(INSTALL_LIBRARY_DIR)/libpolvcore.so
	rm -f $(INSTALL_LIBRARY_DIR)/libpolv.so

check: version-header
	$(MAKE) -C $(POLV_CORE_DIR) check
	$(MAKE) -C $(POLV_DIR) check

clean:
	$(MAKE) -C $(POLV_DIR) clean
	$(MAKE) -C $(POLV_CORE_DIR) clean

dist: version-header
	rm -rf $(DIST_NAME) $(DIST_ARCHIVE)
	mkdir -p $(DIST_NAME)
	cp -a $(DIST_FILES) $(DIST_NAME)/
	$(MAKE) -C $(DIST_NAME) clean
	$(MAKE) -C $(DIST_NAME)/samples clean
	tar -czf $(DIST_ARCHIVE) $(DIST_NAME)
	rm -rf $(DIST_NAME)
	@echo "Created $(DIST_ARCHIVE)"

distclean: clean uninstall
	rm -rf $(DIST_NAME) $(DIST_ARCHIVE)
	rm -rf $(INSTALL_DIR)
	$(MAKE) -C $(SAMPLES_DIR) clean

version-header:
	@printf '%s\n' \
		'/* Generated from VERSION. Do not edit manually. */' \
		'#ifndef POLV_VERSION_H' \
		'#define POLV_VERSION_H' \
		'' \
		'#define POLV_VERSION_MAJOR $(VERSION_MAJOR)' \
		'#define POLV_VERSION_MINOR $(VERSION_MINOR)' \
		'#define POLV_VERSION_PATCH $(VERSION_PATCH)' \
		'' \
		'#define POLV_VERSION_STRING "$(VERSION)"' \
		'' \
		'#endif /* POLV_VERSION_H */' \
		> $(VERSION_HEADER).tmp
	@mv $(VERSION_HEADER).tmp $(VERSION_HEADER)