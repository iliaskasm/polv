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
 * POLV re-initialization
 */
#include "test.h"

#define NITERS 10

typedef struct {
	int initialized;
} State;

static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	int iter;

	// Initialize and finalize POLV `NITERS` times
	for (iter = 0; iter < NITERS; iter++)
	{
		TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
		state->initialized = 1;
		TEST_REQUIRE(test, polvGetNumDevices() > 0);

		// Re-initialization is expected to be harmless
		TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
		TEST_REQUIRE(test, polvGetNumDevices() > 0);

		polvFinalize();
		state->initialized = 0;
		TEST_REQUIRE(test, polvGetNumDevices() == POLV_ERROR_NOT_INITIALIZED);

		// Finalizing an already-finalized API is expected to be harmless
		polvFinalize();
	}

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	if (state->initialized)
		polvFinalize();
}


int main(void)
{
	State state = { 0 };
	return test_run("lifecycle/reinit", run, cleanup, &state);
}
