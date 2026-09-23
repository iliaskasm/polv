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
 * POLV Core device-to-host using device local memory
 */
#include <stdint.h>
#include <stdlib.h>
#include "test.h"

typedef struct {
	int initialized;
	POLVCoreContext *context;
	POLVCoreKernel *kernel;
	uint32_t *host_dst;
	POLVCoreMemory *device_src;
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
	const uint32_t threads = 256;
	const uint32_t grid_x = (uint32_t) ((n + threads - 1) / threads);
	POLVCoreDevice *device;
	void *args[1];
	size_t i;

	// Initialize host
	state->host_dst = malloc(bytes);
	TEST_REQUIRE(test, state->host_dst != NULL);

	// Initialize POLV Core
	TEST_REQUIRE_SUCCESS(test, polvCoreInit());
	state->initialized = 1;

	// Get device and create context
	TEST_REQUIRE(test, polvCoreGetNumDevices() > 0);
	device = polvCoreGetDevice(0);
	TEST_REQUIRE(test, device != NULL);
	TEST_REQUIRE_SUCCESS(test, polvCoreContextCreate(&state->context, device));
	TEST_REQUIRE_SUCCESS(test, polvCoreContextSetCurrent(state->context));

	// Allocate device local memory for device_src
	state->device_src = polvCoreMemoryAllocDeviceLocal(bytes);
	TEST_REQUIRE(test, state->device_src != NULL);

	// Create initialization kernel
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelCreate(&state->kernel, TEST_SHADER("init"), 1));

	// Initialization kernel (for device_src)
	args[0] = state->device_src;
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelLaunch(state->kernel, args, grid_x, 1, 1, threads, 1, 1));

	// DEVICE src -> HOST dst
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyD2H(state->device_src, 0, state->host_dst, 0, bytes));

	// Verification
	for (i = 0; i < n; i++)
		TEST_REQUIRE(test, state->host_dst[i] == pattern((uint32_t) i));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *host_ptrs[] = { state->host_dst };
	void *polv_ptrs[] = { state->device_src };

	if (state->initialized)
	{
		if (state->kernel)
			polvCoreKernelDestroy(&state->kernel);
		test_free_polv(polv_ptrs, 1);
		if (state->context)
			polvCoreContextDestroy(&state->context);
		polvCoreFinalize();
	}

	test_free_host(host_ptrs, 1);
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/d2h", run, cleanup, &state);
}
