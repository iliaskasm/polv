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
	POLVCoreContext *context;
	POLVCoreKernel *kernel;
	POLVCoreMemory *src;
	POLVCoreMemory *dst;
	POLVCoreMemory *shape;
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
	uint32_t group[3] = { threads, 1, 1 },
	         grid[3] = { (n + threads - 1) / threads, 1, 1 };
	POLVCoreDevice *device;

	// Initialize POLV Core
	TEST_REQUIRE_SUCCESS(test, polvCoreInit());
	state->initialized = 1;
	TEST_REQUIRE(test, polvCoreGetNumDevices() > 0);
	device = polvCoreGetDevice(0);
	TEST_REQUIRE(test, device != NULL);
	TEST_REQUIRE_SUCCESS(test, polvCoreContextCreate(&state->context, device));
	TEST_REQUIRE_SUCCESS(test, polvCoreContextSetCurrent(state->context));

	// Allocate host-coherent memory for src, dst and shape
	state->src = polvCoreMemoryAllocHostCoherent(n * sizeof(float));
	state->dst = polvCoreMemoryAllocHostCoherent(n * sizeof(float));
	state->shape = polvCoreMemoryAllocHostCoherent(sizeof(uint32_t));
	TEST_REQUIRE(test, state->src != NULL);
	TEST_REQUIRE(test, state->dst != NULL);
	TEST_REQUIRE(test, state->shape != NULL);

	// Get host pointers for src, dst and shape
	src = polvCoreMemoryGetHostPointer(state->src);
	dst = polvCoreMemoryGetHostPointer(state->dst);
	shape = polvCoreMemoryGetHostPointer(state->shape);
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
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelCreate(&state->kernel, TEST_SHADER("copy"), 3));
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelLaunch(state->kernel, args, grid[0], grid[1], grid[2], 
	                           group[0], group[1], group[2]));
	TEST_REQUIRE(test, TEST_COMPARE_FLOAT(test, dst, src, n, 0.0f));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->src, state->dst, state->shape };

	if (state->initialized)
	{
		if (state->kernel)
			polvCoreKernelDestroy(&state->kernel);
		test_free_polv(polv_ptrs, 3);
		if (state->context)
			polvCoreContextDestroy(&state->context);
		polvCoreFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/unified_copy_kernel", run, cleanup, &state);
}
