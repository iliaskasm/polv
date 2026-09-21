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
 * Device-to-device using host-coherent memory
 */
#include "test.h"

typedef struct {
	int initialized;
	float *src;
	float *dst;
	void *device_src;
	void *device_dst;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const size_t n = 2048;
	const size_t bytes = n * sizeof(float);

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;

	// Allocate host-coherent memory for DEVICE src and dst
	state->device_src = polvAlloc(bytes, polvMemHostCoherent);
	state->device_dst = polvAlloc(bytes, polvMemHostCoherent);
	TEST_REQUIRE(test, state->device_src != NULL);
	TEST_REQUIRE(test, state->device_dst != NULL);

	// Get host pointers for DEVICE src and dst
	state->src = polvGetHostPointer(state->device_src);
	state->dst = polvGetHostPointer(state->device_dst);
	TEST_REQUIRE(test, state->src != NULL);
	TEST_REQUIRE(test, state->dst != NULL);

	// Fill host-coherent memory with data
	test_fill_float(state->src, n, 1.25f, 7.0f);
	test_fill_float(state->dst, n, 0.0f, 0.0f);

	// DEVICE src -> DEVICE dst
	TEST_REQUIRE_RESULT(test, polvMemcpy(state->device_src, 0, state->device_dst, 0, bytes, polvDeviceToDevice), 
	                          POLV_SUCCESS);
	TEST_REQUIRE(test, TEST_COMPARE_FLOAT(test, state->dst, state->src, n, 0.0f));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->device_src, state->device_dst };

	if (state->initialized)
	{
		test_free_polv(polv_ptrs, 2);
		polvFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/d2d_unified", run, cleanup, &state);
}