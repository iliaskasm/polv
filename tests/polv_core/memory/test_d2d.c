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
 * POLV Core device-to-device using device local memory
 */
#include <stdint.h>
#include "test.h"

typedef struct {
	int initialized;
	POLVCoreContext *context;
	POLVCoreKernel *init_kernel;
	POLVCoreKernel *compare_kernel;
	POLVCoreMemory *device_src;
	POLVCoreMemory *device_dst;
	POLVCoreMemory *device_result;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const size_t n = 2048;
	const size_t bytes = n * sizeof(uint32_t);
	const uint32_t threads = 256;
	const uint32_t grid_x = (uint32_t) ((n + threads - 1) / threads);
	POLVCoreDevice *device;
	void *init_args[1], *compare_args[3];
	uint32_t result = 0;

	// Initialize POLV Core
	TEST_REQUIRE_SUCCESS(test, polvCoreInit());
	state->initialized = 1;

	// Get device and create context
	TEST_REQUIRE(test, polvCoreGetNumDevices() > 0);
	device = polvCoreGetDevice(0);
	TEST_REQUIRE(test, device != NULL);
	TEST_REQUIRE_SUCCESS(test, polvCoreContextCreate(&state->context, device));
	TEST_REQUIRE_SUCCESS(test, polvCoreContextSetCurrent(state->context));

	// Allocate device local memory for DEVICE src, dst and result
	state->device_src = polvCoreMemoryAllocDeviceLocal(bytes);
	state->device_dst = polvCoreMemoryAllocDeviceLocal(bytes);
	state->device_result = polvCoreMemoryAllocDeviceLocal(sizeof(uint32_t));
	TEST_REQUIRE(test, state->device_src != NULL);
	TEST_REQUIRE(test, state->device_dst != NULL);
	TEST_REQUIRE(test, state->device_result != NULL);

	// Create initialization and comparison kernels
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelCreate(&state->init_kernel, TEST_SHADER("init"), 1));
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelCreate(&state->compare_kernel, TEST_SHADER("compare_src_dst"), 3));

	// Initialization kernel (for device_src)
	init_args[0] = state->device_src;
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelLaunch(state->init_kernel, init_args, grid_x, 1, 1, threads, 1, 1));

	// DEVICE src -> DEVICE dst
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyD2D(state->device_src, 0, state->device_dst, 0, bytes));

	// Compare device_src with device_dst on the device
	compare_args[0] = state->device_src;
	compare_args[1] = state->device_dst;
	compare_args[2] = state->device_result;
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelLaunch(state->compare_kernel, compare_args, 1, 1, 1, 1, 1, 1));

	// DEVICE result -> HOST result
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyD2H(state->device_result, 0, &result, 0, sizeof(result)));
	TEST_REQUIRE(test, result == 1);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->device_src, state->device_dst, state->device_result };

	if (state->initialized)
	{
		if (state->compare_kernel)
			polvCoreKernelDestroy(&state->compare_kernel);
		if (state->init_kernel)
			polvCoreKernelDestroy(&state->init_kernel);
		test_free_polv(polv_ptrs, 3);
		if (state->context)
			polvCoreContextDestroy(&state->context);
		polvCoreFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/d2d", run, cleanup, &state);
}
