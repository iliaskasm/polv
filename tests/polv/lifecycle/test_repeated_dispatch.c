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
 * POLV repeated kernel launches
 */
#include <stdint.h>
#include "test.h"

#define NITERS 100

typedef struct {
	int initialized;
	void *data;
	void *shape;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const uint32_t n = 1000;
	const uint32_t threads = 64;
	float *data;
	uint32_t *shape, i;
	void *args[2];
	POLVDim group = { threads, 1, 1 },
	        grid = { (n + threads - 1) / threads, 1, 1 };
	int iter;

	// Initialize POLV
	TEST_REQUIRE_SUCCESS(test, polvInit());
	state->initialized = 1;
	state->data = polvAlloc((size_t) n * sizeof(float), polvMemHostCoherent);
	state->shape = polvAlloc(sizeof(uint32_t), polvMemHostCoherent);
	TEST_REQUIRE(test, state->data && state->shape);
	data = polvGetHostPointer(state->data);
	shape = polvGetHostPointer(state->shape);
	TEST_REQUIRE(test, data && shape);
	for (i = 0; i < n; i++)
		data[i] = 0.0f;
	*shape = n;

	args[0] = state->data;
	args[1] = state->shape;

	// Launch kernel `NITERS` times
	for (iter = 0; iter < NITERS; iter++)
		TEST_REQUIRE_SUCCESS(test, polvKernelLaunch(TEST_SHADER("increment"), args, 2, grid, group));

	for (i = 0; i < n; i++)
		TEST_REQUIRE(test, data[i] == (float) NITERS);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *ptrs[] = { state->data, state->shape };
	if (state->initialized)
	{
		test_free_polv(ptrs, 2);
		polvFinalize();
	}
}


int main(void)
{
	State state = { 0 };
	return test_run("lifecycle/repeated_dispatch", run, cleanup, &state);
}
