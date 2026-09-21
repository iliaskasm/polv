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
 * Device-to-device using device local memory
 */
#include <stdint.h>
#include "test.h"

typedef struct {
	int initialized;
	void *device_src;
	void *device_dst;
	void *device_result;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;

	const size_t n = 2048;
	const size_t bytes = n * sizeof(uint32_t);
	const uint32_t threads = 256;
	void *init_args[1], *compare_args[3];
	uint32_t result = 0;

	POLVDim init_grid = { (uint32_t) ((n + threads - 1) / threads), 1, 1	},
	        init_group = { threads, 1, 1 },
	        compare_grid = { 1, 1, 1 },
	        compare_group = { 1, 1, 1 };

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;

	// Allocate device local memory for DEVICE src, dst and result
	state->device_src = polvAlloc(bytes, polvMemDeviceLocal);
	state->device_dst = polvAlloc(bytes, polvMemDeviceLocal);
	state->device_result = polvAlloc(sizeof(uint32_t), polvMemDeviceLocal);
	TEST_REQUIRE(test, state->device_src != NULL);
	TEST_REQUIRE(test, state->device_dst != NULL);
	TEST_REQUIRE(test, state->device_result != NULL);

	// Initialization kernel (for device_src)
	init_args[0] = state->device_src;
	TEST_REQUIRE_RESULT(test, polvKernelLaunch(TEST_SHADER("init"), init_args, 1, init_grid, init_group),
	                          POLV_SUCCESS);

	// DEVICE src -> DEVICE dst
	TEST_REQUIRE_RESULT(test, polvMemcpy(state->device_src,	0, state->device_dst, 0, bytes,	polvDeviceToDevice),
	                          POLV_SUCCESS);

	// Compare device_src with device_dst on the *device*
	// result[0] = 1 when all elements are identical, otherwise 0
	compare_args[0] = state->device_src;
	compare_args[1] = state->device_dst;
	compare_args[2] = state->device_result;

	// Comparison kernel launch (device_src[i] == device_dst[i])
	TEST_REQUIRE_RESULT(test, polvKernelLaunch(TEST_SHADER("compare_src_dst"), compare_args, 3, compare_grid, compare_group),
	                          POLV_SUCCESS);

	// DEVICE result -> HOST result
	TEST_REQUIRE_RESULT(test, polvMemcpy(state->device_result, 0, &result, 0, sizeof(result), polvDeviceToHost),
	                          POLV_SUCCESS);
	TEST_REQUIRE(test, result == 1);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->device_src, state->device_dst, state->device_result };

	if (state->initialized)
	{
		test_free_polv(polv_ptrs, 3);
		polvFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/d2d", run, cleanup, &state);
}