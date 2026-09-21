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
 * Vector addition (1D)
 */
#include <stdint.h>
#include <stdlib.h>
#include "test.h"

typedef struct {
	int initialized;
	float *a;
	float *b;
	float *c;
	float *expected;
	void *da;
	void *db;
	void *dc;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const size_t n = 1000;
	const uint32_t threads = 128;
	const size_t bytes = n * sizeof(float);
	void *args[3];
	POLVDim group = { threads, 1, 1 };
	POLVDim grid = { (uint32_t) ((n + threads - 1) / threads), 1, 1 };

	// Initialize host
	state->a = malloc(bytes);
	state->b = malloc(bytes);
	state->c = malloc(bytes);
	state->expected = malloc(bytes);
	TEST_REQUIRE(test, state->a && state->b && state->c && state->expected);

	test_fill_float(state->a, n, 0.25f, 1.0f);
	test_fill_float(state->b, n, -0.5f, 3.0f);
	for (size_t i = 0; i < n; i++)
		state->expected[i] = state->a[i] + state->b[i];

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;
	state->da = polvAlloc(bytes, polvMemDeviceLocal);
	state->db = polvAlloc(bytes, polvMemDeviceLocal);
	state->dc = polvAlloc(bytes, polvMemDeviceLocal);
	TEST_REQUIRE(test, state->da && state->db && state->dc);

	// HOST a, b -> DEVICE a, b
	TEST_REQUIRE_RESULT(test, polvMemcpy(state->a, 0, state->da, 0, bytes, polvHostToDevice),
	                          POLV_SUCCESS);
	TEST_REQUIRE_RESULT(test, polvMemcpy(state->b, 0, state->db, 0, bytes, polvHostToDevice),
	                          POLV_SUCCESS);

	args[0] = state->da;
	args[1] = state->db;
	args[2] = state->dc;

	// Kernel launch
	TEST_REQUIRE_RESULT(test, polvKernelLaunch(TEST_SHADER("vecadd"), args, 3, grid, group),
	                          POLV_SUCCESS);

	// DEVICE c -> HOST c
	TEST_REQUIRE_RESULT(test, polvMemcpy(state->dc, 0, state->c, 0, bytes, polvDeviceToHost),
	                          POLV_SUCCESS);
	TEST_REQUIRE(test, TEST_COMPARE_FLOAT(test, state->c, state->expected, n, 1.0e-6f));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->da, state->db, state->dc };
	void *host_ptrs[] = { state->a, state->b, state->c, state->expected };

	if (state->initialized)
	{
		test_free_polv(polv_ptrs, 3);
		polvFinalize();
	}
	test_free_host(host_ptrs, 4);
}


int main(void)
{
	State state = { 0 };

	return test_run("kernels/vecadd", run, cleanup, &state);
}
