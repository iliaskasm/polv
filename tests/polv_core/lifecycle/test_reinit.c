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
 * POLV Core re-initialization tests
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

	// Initialize and finalize POLV Core `NITERS` times
	for (iter = 0; iter < NITERS; iter++)
	{
		TEST_REQUIRE_SUCCESS(test, polvCoreInit());
		state->initialized = 1;
		TEST_REQUIRE(test, polvCoreGetNumDevices() > 0);

		// Re-initialization is expected to be harmless
		TEST_REQUIRE_SUCCESS(test, polvCoreInit());
		TEST_REQUIRE(test, polvCoreGetNumDevices() > 0);

		polvCoreFinalize();
		state->initialized = 0;

		// Finalizing an already-finalized API is expected to be harmless
		polvCoreFinalize();
	}

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;

	if (state->initialized)
		polvCoreFinalize();
}


int main(void)
{
	State state = { 0 };

	return test_run("lifecycle/reinit", run, cleanup, &state);
}
