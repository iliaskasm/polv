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
 * Host-coherent memory tests using copy kernel
 */
#include <stdint.h>
#include "test.h"

typedef struct {
	int initialized;
	void *src;
	void *dst;
	void *shape;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const uint32_t n = 1000;
	const uint32_t threads = 128;
	float *src;
	float *dst;
	uint32_t *shape, i;
	void *args[3];
	POLVDim group = { threads, 1, 1 },
	        grid = { (n + threads - 1) / threads, 1, 1 };

	// Initialize POLV
	TEST_REQUIRE_SUCCESS(test, polvInit());
	state->initialized = 1;

	// Allocate host-coherent memory for src, dst and shape
	state->src = polvAlloc(n * sizeof(float), polvMemHostCoherent);
	state->dst = polvAlloc(n * sizeof(float), polvMemHostCoherent);
	state->shape = polvAlloc(sizeof(uint32_t), polvMemHostCoherent);
	TEST_REQUIRE(test, state->src != NULL);
	TEST_REQUIRE(test, state->dst != NULL);
	TEST_REQUIRE(test, state->shape != NULL);

	// Get host pointers for src, dst and shape
	src = polvGetHostPointer(state->src);
	dst = polvGetHostPointer(state->dst);
	shape = polvGetHostPointer(state->shape);
	TEST_REQUIRE(test, src != NULL);
	TEST_REQUIRE(test, dst != NULL);
	TEST_REQUIRE(test, shape != NULL);

	// Fill src with values
	test_fill_float(src, n, 0.125f, 2.0f);
	for (i = 0; i < n; i++)
		dst[i] = 0.0f;

	// Copy kernel launch (dst[i] = src[i])
	*shape = n;
	args[0] = state->src;
	args[1] = state->dst;
	args[2] = state->shape;
	TEST_REQUIRE_SUCCESS(test, polvKernelLaunch(TEST_SHADER("copy"), args, 3, grid, group));
	TEST_REQUIRE(test, TEST_COMPARE_FLOAT(test, dst, src, n, 0.0f));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *ptrs[] = { state->src, state->dst, state->shape };

	if (state->initialized)
	{
		test_free_polv(ptrs, 3);
		polvFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/unified_copy_kernel", run, cleanup, &state);
}
