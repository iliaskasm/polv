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
 * POLV Core kernel dispatch tests
 */
#include <stdint.h>
#include "test.h"

typedef struct {
	int initialized;
	void *data;
	void *shape;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const uint32_t n = 1000, threads = 64;
	const uint32_t grid_x = (n + threads - 1) / threads;
	uint32_t *shape;
	float *data;
	void *args[2];
	uint32_t i;
	POLVDim grid = { grid_x, 1, 1 },
	        group = { threads, 1, 1 };

	// Initialize POLV
	TEST_REQUIRE_SUCCESS(test, polvInit());
	state->initialized = 1;

	// Get device and create context
	TEST_REQUIRE(test, polvGetNumDevices() > 0);
	TEST_REQUIRE_SUCCESS(test, polvSetDevice(0));

	// Allocate host-coherent input and shape buffers
	state->data = polvAlloc(n * sizeof(float), polvMemHostCoherent);
	state->shape = polvAlloc(sizeof(uint32_t), polvMemHostCoherent);
	TEST_REQUIRE(test, state->data != NULL);
	TEST_REQUIRE(test, state->shape != NULL);

	data = polvGetHostPointer(state->data);
	shape = polvGetHostPointer(state->shape);
	TEST_REQUIRE(test, data != NULL);
	TEST_REQUIRE(test, shape != NULL);

	for (i = 0; i < n; i++)
		data[i] = (float) i;
	*shape = n;

	// Create and launch increment kernel
	args[0] = state->data;
	args[1] = state->shape;
	TEST_REQUIRE_SUCCESS(test, polvKernelLaunch(TEST_SHADER("increment"), args, 2, grid, group));

	// Verify host-coherent results
	for (i = 0; i < n; i++)
		TEST_REQUIRE(test, data[i] == (float) i + 1.0f);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->data, state->shape };

	if (state->initialized)
	{
		test_free_polv(polv_ptrs, 2);
		polvFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("kernels/increment", run, cleanup, &state);
}
