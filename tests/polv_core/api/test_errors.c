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
 * Edge cases tests
 */
#include <stdint.h>
#include "test.h"

typedef struct {
	int initialized;
	POLVCoreContext *context;
	POLVCoreKernel *kernel;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	POLVCoreDevice *device;

	float a = 1.0f, b = 0.0f;
	void *dummy_args[1] = { state };
	int devices;

	// Execute POLV Core commands without prior initialization
	TEST_EXPECT(test, polvCoreGetNumDevices() == 0);
	TEST_EXPECT(test, polvCoreDeviceGetId(0) == -1);
	TEST_EXPECT(test, polvCoreMemoryAllocDeviceLocal(16) == NULL);
	TEST_EXPECT(test, polvCoreMemoryGetHostPointer(NULL) == NULL);
	TEST_EXPECT_RESULT(test, polvCoreMemoryCopyH2H(&a, 0, &b, 0, sizeof(a)),
	                         POLV_CORE_ERROR_NOT_INITIALIZED);

	// Initialize POLV Core
	TEST_REQUIRE_SUCCESS(test, polvCoreInit());
	state->initialized = 1;

	// Get number of devices
	devices = polvCoreGetNumDevices();
	TEST_REQUIRE(test, devices > 0);
	device = polvCoreGetDevice(0);
	TEST_REQUIRE(test, device != NULL);
	TEST_REQUIRE_SUCCESS(test, polvCoreContextCreate(&state->context, device));
	TEST_REQUIRE_SUCCESS(test, polvCoreContextSetCurrent(state->context));

	// TEST 1: Device ID out of bounds (< 0 and >= #devices)
	TEST_EXPECT(test, polvCoreGetDevice(-1) == NULL);
	TEST_EXPECT(test, polvCoreGetDevice(devices) == NULL);

	// TEST 2: Allocate 0 bytes
	TEST_EXPECT(test, polvCoreMemoryAllocDeviceLocal(0) == NULL);

	// TEST 3: Memcpy from NULL to b
	TEST_EXPECT_RESULT(test, polvCoreMemoryCopyH2H(NULL, 0, &b, 0, sizeof(b)),
	                         POLV_CORE_ERROR_INVALID_ARGUMENT);

	// TEST 4: Memcpy from a to NULL
	TEST_EXPECT_RESULT(test, polvCoreMemoryCopyH2H(&a, 0, NULL, 0, sizeof(a)),
	                         POLV_CORE_ERROR_INVALID_ARGUMENT);

	// TEST 5: Memcpy from a to b, size = 0
	TEST_EXPECT_RESULT(test, polvCoreMemoryCopyH2H(&a, 0, &b, 0, 0),
	                         POLV_CORE_ERROR_INVALID_ARGUMENT);

	// TEST 6: Create kernel with NULL output pointer
	TEST_EXPECT_RESULT(test, polvCoreKernelCreate(NULL, "unused", 1), 
	                         POLV_CORE_ERROR_INVALID_ARGUMENT);

	// TEST 7: Launch NULL kernel
	TEST_EXPECT_RESULT(test, polvCoreKernelLaunch(NULL, dummy_args, 1, 1, 1, 1, 1, 1),
	                         POLV_CORE_ERROR_INVALID_ARGUMENT);

	// TEST 8: Launch init kernel with NULL arguments
	TEST_REQUIRE_SUCCESS(test, polvCoreKernelCreate(&state->kernel, TEST_SHADER("init"), 1));
	TEST_EXPECT_RESULT(test, polvCoreKernelLaunch(state->kernel, NULL, 1, 1, 1, 1, 1, 1),
	                         POLV_CORE_ERROR_INVALID_ARGUMENT);
	polvCoreKernelDestroy(&state->kernel);

	// TEST 9: Create init kernel with narg = 0
	TEST_EXPECT_RESULT(test, polvCoreKernelCreate(&state->kernel, TEST_SHADER("init"), 0), 
	                         POLV_CORE_ERROR_INVALID_ARGUMENT);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;

	if (state->initialized)
	{
		if (state->kernel)
    		polvCoreKernelDestroy(&state->kernel);
		if (state->context)
			polvCoreContextDestroy(&state->context);
		polvCoreFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("api/errors", run, cleanup, &state);
}
