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

BUILD_DIR ?= build
BUILD_ROOT := $(abspath $(BUILD_DIR))
POLV_BUILD_DIR := $(BUILD_ROOT)/polv
POLV_CORE_BUILD_DIR := $(BUILD_ROOT)/polv_core

POLV_LIBRARY := $(POLV_BUILD_DIR)/libpolv.so
POLV_CORE_LIBRARY := $(POLV_CORE_BUILD_DIR)/libpolvcore.so

POLV_INPUTS := \
	$(wildcard $(POLV_DIR)/*.c) \
	$(wildcard $(POLV_DIR)/*.h) \
	$(POLV_DIR)/Makefile \
	include/polv.h \
	include/polv_core.h \
	$(VERSION_HEADER)

POLV_CORE_INPUTS := \
	$(wildcard $(POLV_CORE_DIR)/*.c) \
	$(wildcard $(POLV_CORE_DIR)/*.h) \
	$(POLV_CORE_DIR)/Makefile \
	include/polv_core.h \
	$(VERSION_HEADER)

PACKAGE := polv

LICENSE_FILE := LICENSE

VERSION_FILE := VERSION
VERSION := $(strip $(shell cat $(VERSION_FILE)))
VERSION_PARTS := $(subst ., ,$(VERSION))
VERSION_MAJOR := $(word 1,$(VERSION_PARTS))
VERSION_MINOR := $(word 2,$(VERSION_PARTS))
VERSION_PATCH := $(word 3,$(VERSION_PARTS))
VERSION_HEADER := include/polv_version.h

DIST_NAME := $(PACKAGE)-$(VERSION)
DIST_ARCHIVE := $(DIST_NAME).tar.gz
DIST_FILES := Makefile README.md LICENSE VERSION include src samples tests

INSTALL_DIR ?= $(CURDIR)/install
INSTALL_INCLUDE_DIR := $(INSTALL_DIR)/include
INSTALL_LIBRARY_DIR := $(INSTALL_DIR)/lib

INSTALL_FILES := \
	$(INSTALL_INCLUDE_DIR)/polv_core.h \
	$(INSTALL_INCLUDE_DIR)/polv.h \
	$(INSTALL_INCLUDE_DIR)/polv_version.h \
	$(INSTALL_LIBRARY_DIR)/libpolvcore.so \
	$(INSTALL_LIBRARY_DIR)/libpolv.so

TESTS_DIR := $(CURDIR)/tests
SAMPLES_DIR := $(CURDIR)/samples

TEST_TARGETS := test test_polv test_polv_core

.PHONY: all clean distclean polv_core polv install uninstall check dist \
	$(TEST_TARGETS)

all: $(POLV_CORE_LIBRARY) $(POLV_LIBRARY)

polv_core: $(POLV_CORE_LIBRARY)

polv: $(POLV_LIBRARY)

$(POLV_CORE_LIBRARY): $(POLV_CORE_INPUTS)
	$(MAKE) -C $(POLV_CORE_DIR) \
		BUILD_DIR=$(POLV_CORE_BUILD_DIR)

$(POLV_LIBRARY): $(POLV_INPUTS) $(POLV_CORE_LIBRARY)
	$(MAKE) -C $(POLV_DIR) \
		BUILD_DIR=$(POLV_BUILD_DIR) \
		POLV_CORE_BUILD_DIR=$(POLV_CORE_BUILD_DIR)

install: $(INSTALL_FILES)

$(INSTALL_INCLUDE_DIR)/polv_core.h: include/polv_core.h
	@mkdir -p $(dir $@)
	install -m 644 $< $@

$(INSTALL_INCLUDE_DIR)/polv.h: include/polv.h
	@mkdir -p $(dir $@)
	install -m 644 $< $@

$(INSTALL_INCLUDE_DIR)/polv_version.h: $(VERSION_HEADER)
	@mkdir -p $(dir $@)
	install -m 644 $< $@

$(INSTALL_LIBRARY_DIR)/libpolvcore.so: $(POLV_CORE_LIBRARY)
	@mkdir -p $(dir $@)
	install -m 755 $< $@

$(INSTALL_LIBRARY_DIR)/libpolv.so: $(POLV_LIBRARY)
	@mkdir -p $(dir $@)
	install -m 755 $< $@

uninstall:
	rm -f $(INSTALL_INCLUDE_DIR)/polv_core.h
	rm -f $(INSTALL_INCLUDE_DIR)/polv.h
	rm -f $(INSTALL_INCLUDE_DIR)/polv_version.h
	rm -f $(INSTALL_LIBRARY_DIR)/libpolvcore.so
	rm -f $(INSTALL_LIBRARY_DIR)/libpolv.so

check: $(VERSION_HEADER)
	$(MAKE) -C $(POLV_CORE_DIR) check
	$(MAKE) -C $(POLV_DIR) check

clean:
	rm -rf $(BUILD_ROOT)
	$(MAKE) -C $(SAMPLES_DIR) clean
	$(MAKE) -C $(TESTS_DIR) clean

dist: $(VERSION_HEADER)
	rm -rf $(DIST_NAME) $(DIST_ARCHIVE)
	mkdir -p $(DIST_NAME)
	cp -a $(DIST_FILES) $(DIST_NAME)/
	$(MAKE) -C $(DIST_NAME) clean
	tar -czf $(DIST_ARCHIVE) $(DIST_NAME)
	rm -rf $(DIST_NAME)
	@echo "Created $(DIST_ARCHIVE)"

distclean: clean uninstall
	rm -rf $(DIST_NAME) $(DIST_ARCHIVE)

$(TEST_TARGETS): install
	$(MAKE) -C $(TESTS_DIR) \
		POLV_INSTALL_DIR=$(abspath $(INSTALL_DIR)) \
		$@

$(VERSION_HEADER): $(VERSION_FILE) $(LICENSE_FILE)
	@{ \
		printf '/*\n'; \
		sed 's/^/ * /' $(LICENSE_FILE); \
		printf ' */\n\n'; \
		printf '/* Generated from VERSION (%s). Do not edit manually. */\n' "$(VERSION)"; \
		printf '#ifndef POLV_VERSION_H\n'; \
		printf '#define POLV_VERSION_H\n\n'; \
		printf '#define POLV_VERSION_MAJOR %s\n' "$(VERSION_MAJOR)"; \
		printf '#define POLV_VERSION_MINOR %s\n' "$(VERSION_MINOR)"; \
		printf '#define POLV_VERSION_PATCH %s\n\n' "$(VERSION_PATCH)"; \
		printf '#define POLV_VERSION_STRING "%s"\n\n' "$(VERSION)"; \
		printf '#endif /* POLV_VERSION_H */\n'; \
	} > $@.tmp
	@mv $@.tmp $@