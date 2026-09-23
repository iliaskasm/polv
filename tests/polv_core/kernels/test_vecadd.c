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
 * POLV Core vector addition kernel tests
 */
#include <stdint.h>
#include <stdlib.h>
#include "test.h"

typedef struct {
	int initialized;
	POLVCoreContext *context;
	POLVCoreKernel *kernel;
	float *a;
	float *b;
	float *c;
	float *expected;
	POLVCoreMemory *da;
	POLVCoreMemory *db;
	POLVCoreMemory *dc;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const uint32_t n = 4096;
	const uint32_t threads = 256;
	const uint32_t grid_x = (n + threads - 1) / threads;
	const size_t bytes = (size_t) n * sizeof(float);
	POLVCoreDevice *device;
	void *args[3];
	uint32_t i;

	// Initialize host
	state->a = malloc(bytes);
	state->b = malloc(bytes);
	state->c = malloc(bytes);
	state->expected = malloc(bytes);
	TEST_REQUIRE(test, state->a && state->b && state->c && state->expected);

	test_fill_float(state->a, n, 0.25f, 1.0f);
	test_fill_float(state->b, n, -0.125f, 3.0f);
	for (i = 0; i < n; i++)
		state->expected[i] = state->a[i] + state->b[i];

	// Initialize POLV Core
	TEST_REQUIRE_SUCCESS(test, polvCoreInit());
	state->initialized = 1;

	// Get device and create context
	TEST_REQUIRE(test, polvCoreGetNumDevices() > 0);
	device = polvCoreGetDevice(0);
	TEST_REQUIRE(test, device != NULL);
	TEST_REQUIRE_SUCCESS(test, polvCoreContextCreate(&state->context, device));
	TEST_REQUIRE_SUCCESS(test, polvCoreContextSetCurrent(state->context));

	// Allocate device local memory for a, b, and c
	state->da = polvCoreMemoryAllocDeviceLocal(bytes);
	state->db = polvCoreMemoryAllocDeviceLocal(bytes);
	state->dc = polvCoreMemoryAllocDeviceLocal(bytes);
	TEST_REQUIRE(test, state->da && state->db && state->dc);

	// HOST a, b -> DEVICE a, b
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyH2D(state->a, 0, state->da, 0, bytes));
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyH2D(state->b, 0, state->db, 0, bytes));

	// Create and launch vector addition kernel
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelCreate(&state->kernel, TEST_SHADER("vecadd"), 3));
	args[0] = state->da;
	args[1] = state->db;
	args[2] = state->dc;
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelLaunch(state->kernel, args, grid_x, 1, 1, threads, 1, 1));

	// DEVICE c -> HOST c
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyD2H(state->dc, 0, state->c, 0, bytes));
	TEST_REQUIRE(test, TEST_COMPARE_FLOAT(test, state->c, state->expected, n, 1.0e-6f));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *host_ptrs[] = { state->a, state->b, state->c, state->expected };
	void *polv_ptrs[] = { state->da, state->db, state->dc };

	if (state->kernel)
		polvCoreKernelDestroy(&state->kernel);

	if (state->initialized)
	{
		test_free_polv(polv_ptrs, 3);
		if (state->context)
			polvCoreContextDestroy(&state->context);
		polvCoreFinalize();
	}

	test_free_host(host_ptrs, 4);
}


int main(void)
{
	State state = { 0 };

	return test_run("kernels/vecadd", run, cleanup, &state);
}
