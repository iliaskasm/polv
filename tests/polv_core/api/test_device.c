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
 * POLV Core device tests
 */
#include "test.h"

typedef struct {
	int initialized;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	int devices;
	int i;

	// Initialize POLV Core
	TEST_REQUIRE_SUCCESS(test, polvCoreInit());
	state->initialized = 1;

	// Get number of devices
	devices = polvCoreGetNumDevices();
	TEST_REQUIRE(test, devices > 0);

	// Verify each device handle and ID
	for (i = 0; i < devices; i++)
	{
		POLVCoreDevice *device = polvCoreGetDevice(i);

		TEST_REQUIRE(test, device != NULL);
		TEST_REQUIRE(test, polvCoreDeviceGetId(device) == i);
	}

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;

	if (state->initialized)
		polvCoreFinalize();
}


int main(void)
{
	State state = { 0 };

	return test_run("api/device", run, cleanup, &state);
}
