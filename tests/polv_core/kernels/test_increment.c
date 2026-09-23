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
 * POLV Core kernel dispatch tests
 */
#include <stdint.h>
#include "test.h"

typedef struct {
	int initialized;
	POLVCoreContext *context;
	POLVCoreKernel *kernel;
	POLVCoreMemory *data;
	POLVCoreMemory *shape;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const uint32_t n = 1000, threads = 64;
	const uint32_t grid_x = (n + threads - 1) / threads;
	POLVCoreDevice *device;
	uint32_t *shape;
	float *data;
	void *args[2];
	uint32_t i;

	// Initialize POLV Core
	TEST_REQUIRE_SUCCESS(test, polvCoreInit());
	state->initialized = 1;

	// Get device and create context
	TEST_REQUIRE(test, polvCoreGetNumDevices() > 0);
	device = polvCoreGetDevice(0);
	TEST_REQUIRE(test, device != NULL);
	TEST_REQUIRE_SUCCESS(test, polvCoreContextCreate(&state->context, device));
	TEST_REQUIRE_SUCCESS(test, polvCoreContextSetCurrent(state->context));

	// Allocate host-coherent input and shape buffers
	state->data = polvCoreMemoryAllocHostCoherent(n * sizeof(float));
	state->shape = polvCoreMemoryAllocHostCoherent(sizeof(uint32_t));
	TEST_REQUIRE(test, state->data != NULL);
	TEST_REQUIRE(test, state->shape != NULL);

	data = polvCoreMemoryGetHostPointer(state->data);
	shape = polvCoreMemoryGetHostPointer(state->shape);
	TEST_REQUIRE(test, data != NULL);
	TEST_REQUIRE(test, shape != NULL);

	for (i = 0; i < n; i++)
		data[i] = (float) i;
	*shape = n;

	// Create and launch increment kernel
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelCreate(&state->kernel, TEST_SHADER("increment"), 2));
	args[0] = state->data;
	args[1] = state->shape;
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelLaunch(state->kernel, args, grid_x, 1, 1, threads, 1, 1));

	// Verify host-coherent results
	for (i = 0; i < n; i++)
		TEST_REQUIRE(test, data[i] == (float) i + 1.0f);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->data, state->shape };

	if (state->initialized)
	{
		if (state->kernel)
			polvCoreKernelDestroy(&state->kernel);

		test_free_polv(polv_ptrs, 2);
		if (state->context)
			polvCoreContextDestroy(&state->context);
		polvCoreFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("kernels/increment", run, cleanup, &state);
}
