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
 * POLV repeated memory allocations
 */
#include "test.h"

#define NITERS 256 

typedef struct {
	int initialized;
	void *current;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const size_t bytes = 64 * 1024;
	int iter;

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;

	// Allocate and free memory `NITERS` times
	for (iter = 0; iter < NITERS; iter++)
	{
		state->current = polvAlloc(bytes, polvMemDeviceLocal);
		TEST_REQUIRE(test, state->current != NULL);
		polvFree(state->current);
		state->current = NULL;
	}

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	if (state->initialized)
	{
		if (state->current)
			polvFree(state->current);
		polvFinalize();
	}
}


int main(void)
{
	State state = { 0 };
	return test_run("lifecycle/repeated_alloc", run, cleanup, &state);
}
