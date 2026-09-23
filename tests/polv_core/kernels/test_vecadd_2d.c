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
 * Vector addition (2D)
 */
#include <stdint.h>
#include <stdlib.h>
#include "test.h"

typedef struct { 
	uint32_t width;
	uint32_t height; 
} Shape;

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
	POLVCoreMemory *shape;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	POLVCoreDevice *device;
	const uint32_t width = 37, height = 19,
	               gx = 8, gy = 8;
	const uint32_t grid_x = (width + gx - 1) / gx,
	               grid_y = (height + gy - 1) / gy;
	const size_t n = (size_t) width * height;
	const size_t bytes = n * sizeof(float);
	Shape *shape;
	void *args[4];
	size_t i;

	// Initialize host
	state->a = malloc(bytes);
	state->b = malloc(bytes);
	state->c = malloc(bytes);
	state->expected = malloc(bytes);
	TEST_REQUIRE(test, state->a && state->b && state->c && state->expected);
	test_fill_float(state->a, n, 0.125f, 1.0f);
	test_fill_float(state->b, n, 0.375f, -2.0f);
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

	// Allocate host-coherent memory for shape
	state->shape = polvCoreMemoryAllocHostCoherent(sizeof(Shape));
	TEST_REQUIRE(test, state->da && state->db && state->dc && state->shape);

	// Get host pointer for shape
	shape = polvCoreMemoryGetHostPointer(state->shape);
	TEST_REQUIRE(test, shape != NULL);

	shape->width = width;
	shape->height = height;

	// HOST a, b -> DEVICE a, b
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyH2D(state->a, 0, state->da, 0, bytes));
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyH2D(state->b, 0, state->db, 0, bytes));

	args[0] = state->da;
	args[1] = state->db;
	args[2] = state->dc;
	args[3] = state->shape;

	// Create and launch kernel
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelCreate(&state->kernel, TEST_SHADER("vecadd_2d"), 4));
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelLaunch(state->kernel, args, grid_x, grid_y, 1,
	                                                gx, gy, 1));

	// DEVICE c -> HOST c
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyD2H(state->dc, 0, state->c, 0, bytes));
	TEST_REQUIRE(test, TEST_COMPARE_FLOAT(test, state->c, state->expected, n, 1.0e-6f));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *host_ptrs[] = { state->a, state->b, state->c, state->expected };
	void *polv_ptrs[] = { state->da, state->db, state->dc, state->shape };

	if (state->initialized)
	{
		if (state->kernel)
			polvCoreKernelDestroy(&state->kernel);
		test_free_polv(polv_ptrs, 4);
		if (state->context)
			polvCoreContextDestroy(&state->context);
		polvCoreFinalize();
	}
	test_free_host(host_ptrs, 4);
}


int main(void)
{
	State state = { 0 };
	return test_run("kernels/vecadd_2d", run, cleanup, &state);
}