/* 
 * MIT License
 * 
 * Copyright (c) 2026 Ilias K. Kasmeridis
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*
 * Template for new tests
 *
 * Don't forget to include your new test in the Makefile
 */
#include <stdint.h>
#include "test.h"

typedef struct {
	int initialized;
	void *da;
	float *a;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;
	
	return 1; // success
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->da };
	void *host_ptrs[] = { state->a };

	// cleanup: use test_free_polv for polv_ptrs and test_free_host
	// for host_ptrs
	if (state->initialized) 
	{ 
		// test_free_polv(...)
		polvFinalize();
	}

	// test_free_host(...)
}


int main(void)
{
	State state = { 0 };

	return test_run("Template/new", run, cleanup, &state);
}
