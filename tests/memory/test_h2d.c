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
 * Host-to-device using device local memory
 */
#include <stdint.h>
#include <stdlib.h>
#include "test.h"

typedef struct {
	int initialized;
	uint32_t *host_src;
	void *device_dst;
	void *device_result;
} State;


static uint32_t pattern(uint32_t i)
{
	return 0x9e3779b9u * (i + 1u) ^ 0xa5a5a5a5u;
}


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const size_t n = 2048;
	const size_t bytes = n * sizeof(uint32_t);
	void *args[2];
	uint32_t result = 0;
	size_t i;
	POLVDim grid = { 1, 1, 1 },
	        group = { 1, 1, 1 };

	// Initialize host
	state->host_src = malloc(bytes);
	TEST_REQUIRE(test, state->host_src != NULL);
	for (i = 0; i < n; ++i)
		state->host_src[i] = pattern((uint32_t) i);

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;
	
	// Allocate memory for DEVICE dst and result
	state->device_dst = polvAlloc(bytes, polvMemDeviceLocal);
	state->device_result = polvAlloc(sizeof(uint32_t), polvMemDeviceLocal);
	TEST_REQUIRE(test, state->device_dst != NULL);
	TEST_REQUIRE(test, state->device_result != NULL);

	// HOST src -> DEVICE dst
	TEST_REQUIRE_RESULT(test, polvMemcpy(state->host_src, 0, state->device_dst, 0, bytes, polvHostToDevice),
	                          POLV_SUCCESS);

	// Comparison kernel launch (device_dst[i] == pattern(i))
	args[0] = state->device_dst;
	args[1] = state->device_result;
	TEST_REQUIRE_RESULT(test, polvKernelLaunch(TEST_SHADER("compare_pattern"), args, 2, grid, group),
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
	void *polv_ptrs[] = { state->device_dst, state->device_result };
	void *host_ptrs[] = { state->host_src };

	if (state->initialized)
	{
		test_free_polv(polv_ptrs, 2);
		polvFinalize();
	}

	test_free_host(host_ptrs, 1);
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/h2d", run, cleanup, &state);
}