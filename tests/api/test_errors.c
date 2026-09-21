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
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	POLVDim one = { 1, 1, 1 };
	float a = 1.0f, b = 0.0f;
	void *dummy_args[1] = { state };
	int devices;

	// Execute POLV commands without prior initialization
	TEST_EXPECT(test, polvGetNumDevices() == POLV_ERROR_NOT_INITIALIZED);
	TEST_EXPECT_RESULT(test, polvSetDevice(0), POLV_ERROR_NOT_INITIALIZED);
	TEST_EXPECT(test, polvGetCurrentDeviceId() == POLV_ERROR_NOT_INITIALIZED);
	TEST_EXPECT(test, polvAlloc(16, polvMemDeviceLocal) == NULL);
	TEST_EXPECT(test, polvGetHostPointer(NULL) == NULL);
	TEST_EXPECT_RESULT(test, polvMemcpy(&a, 0, &b, 0, sizeof(a), polvHostToHost),
	                         POLV_ERROR_NOT_INITIALIZED);

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;

	// Get number of devices
	devices = polvGetNumDevices();
	TEST_REQUIRE(test, devices > 0);

	// TEST 1: Device ID out of bounds (< 0 and > #devices)
	TEST_EXPECT_RESULT(test, polvSetDevice(-1), POLV_ERROR_DEVICE_ID_OUT_OF_BOUNDS);
	TEST_EXPECT_RESULT(test, polvSetDevice(devices), POLV_ERROR_DEVICE_ID_OUT_OF_BOUNDS);

	// TEST 2: Allocate 0 bytes
	TEST_EXPECT(test, polvAlloc(0, polvMemDeviceLocal) == NULL);

	// TEST 3: Memcpy from NULL to b
	TEST_EXPECT_RESULT(test, polvMemcpy(NULL, 0, &b, 0, sizeof(b), polvHostToHost),
	                         POLV_ERROR_INVALID_ARGUMENT);

	// TEST 4: Memcpy from a to NULL
	TEST_EXPECT_RESULT(test, polvMemcpy(&a, 0, NULL, 0, sizeof(a), polvHostToHost),
	                         POLV_ERROR_INVALID_ARGUMENT);

	// TEST 5: Memcpy from a to b, size = 0
	TEST_EXPECT_RESULT(test, polvMemcpy(&a, 0, &b, 0, 0, polvHostToHost),
	                         POLV_ERROR_INVALID_ARGUMENT);

	// TEST 6: Memcpy from a to b, with an unknown direction
	TEST_EXPECT_RESULT(test, polvMemcpy(&a, 0, &b, 0, sizeof(a), (POLVMemcpyDirection) 999),
	                         POLV_ERROR_UNSUPPORTED);

	// TEST 7: Launch NULL kernel
	TEST_EXPECT_RESULT(test, polvKernelLaunch(NULL, dummy_args, 1, one, one),
	                         POLV_ERROR_INVALID_ARGUMENT);

	// TEST 8: Launch unused kernel with NULL arguments
	TEST_EXPECT_RESULT(test, polvKernelLaunch("unused.spv", NULL, 1, one, one),
	                         POLV_ERROR_INVALID_ARGUMENT);

	// TEST 9: Launch unused kernel with narg = 0
	TEST_EXPECT_RESULT(test, polvKernelLaunch("unused.spv", dummy_args, 0, one, one),
	                         POLV_ERROR_INVALID_ARGUMENT);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;

	if (state->initialized)
		polvFinalize();
}


int main(void)
{
	State state = { 0 };

	return test_run("api/errors", run, cleanup, &state);
}
