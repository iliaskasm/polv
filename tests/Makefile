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
.DEFAULT_GOAL := all

POLV_TEST_DIR := polv/
POLV_CORE_TEST_DIR := polv_core/

.PHONY: \
	all \
	polv \
	polv_core \
	test \
	test_polv \
	test_polv_core \
	run \
	check \
	clean_polv \
	clean_polv_core \
	clean

polv:
	$(MAKE) -C $(POLV_TEST_DIR)

polv_core:
	$(MAKE) -C $(POLV_CORE_TEST_DIR)

all: polv polv_core

test_polv:
	$(MAKE) -C $(POLV_TEST_DIR) test

test_polv_core:
	$(MAKE) -C $(POLV_CORE_TEST_DIR) test

test: test_polv test_polv_core

run check: test

clean_polv:
	$(MAKE) -C $(POLV_TEST_DIR) clean

clean_polv_core:
	$(MAKE) -C $(POLV_CORE_TEST_DIR) clean

clean: clean_polv clean_polv_core
