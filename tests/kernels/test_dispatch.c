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
 * Test kernel launching by executing a copy shader (a = b)
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"

typedef struct {
	int initialized;
	float *src;
	float *out;
	float *zeros;
	void *device_src;
	float *device_dst;
	float *shape;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	static const uint32_t sizes[] = { 
		1, 17, 63, 64, 65, 127, 128, 129, 1000 
	};
	const size_t max_n = 1000, threads = 64;
	const size_t bytes = (size_t) max_n * sizeof(float);
	uint32_t *shape, i;
	void *args[3];
	POLVDim group = { threads, 1, 1 };
	size_t s, num_sizes = sizeof(sizes) / sizeof(sizes[0]);

	// Initialize host
	state->src = malloc(bytes);
	state->out = malloc(bytes);
	state->zeros = calloc(max_n, sizeof(float)); // max_n zeros
	TEST_REQUIRE(test, state->src && state->out && state->zeros);
	test_fill_float(state->src, max_n, 0.125f, 5.0f);

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;
	state->device_src = polvAlloc(bytes, polvMemDeviceLocal);
	state->device_dst = polvAlloc(bytes, polvMemDeviceLocal);
	state->shape = polvAlloc(sizeof(uint32_t), polvMemHostCoherent);
	TEST_REQUIRE(test, state->device_src && state->device_dst && state->shape);

	// HOST src -> DEVICE src
	shape = polvGetHostPointer(state->shape);
	TEST_REQUIRE(test, shape != NULL);
	TEST_REQUIRE_RESULT(test, polvMemcpy(state->src, 0, state->device_src, 0, bytes, polvHostToDevice), 
	                          POLV_SUCCESS);

	args[0] = state->device_src;
	args[1] = state->device_dst;
	args[2] = state->shape;

	// Launch `num_sizes` kernels
	for (s = 0; s < num_sizes; s++)
	{
		uint32_t n = sizes[s];
		POLVDim grid = { (n + threads - 1) / threads, 1, 1 };

		*shape = n;
		memset(state->out, 0, bytes);
		TEST_REQUIRE_RESULT(test, polvMemcpy(state->zeros, 0, state->device_dst, 0, bytes, polvHostToDevice), 
		                          POLV_SUCCESS);
		TEST_REQUIRE_RESULT(test, polvKernelLaunch(TEST_SHADER("copy"), args, 3, grid, group), 
		                          POLV_SUCCESS);
		TEST_REQUIRE_RESULT(test, polvMemcpy(state->device_dst, 0, state->out, 0, bytes, polvDeviceToHost), 
		                          POLV_SUCCESS);
		TEST_REQUIRE(test, TEST_COMPARE_FLOAT(test, state->out, state->src, n, 0.0f));
		for (i = n; i < max_n; i++)
			TEST_REQUIRE(test, state->out[i] == 0.0f);
	}

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->device_src, state->device_dst, state->shape };
	void *host_ptrs[] = { state->src, state->out, state->zeros };

	if (state->initialized) 
	{ 
		test_free_polv(polv_ptrs, 3); 
		polvFinalize(); 
	}
	test_free_host(host_ptrs, 3);
}


int main(void)
{
	State state = { 0 };
	return test_run("kernels/dispatch", run, cleanup, &state);
}
