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
 * Template for new POLV Core tests
 */
#include "../common/test.h"

typedef struct {
	int initialized;
	POLVCoreContext *context;
	POLVCoreMemory *memory;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	POLVCoreDevice *device;

	// Initialize POLV Core
	TEST_REQUIRE_RESULT(test, polvCoreInit(), POLV_CORE_SUCCESS);
	state->initialized = 1;
	TEST_REQUIRE(test, polvCoreGetNumDevices() > 0);

	// Create and set the current context
	device = polvCoreGetDevice(0);
	TEST_REQUIRE(test, device != NULL);
	TEST_REQUIRE_RESULT(test, polvCoreContextCreate(&state->context, device),
	                          POLV_CORE_SUCCESS);
	TEST_REQUIRE_RESULT(test, polvCoreContextSetCurrent(state->context),
	                          POLV_CORE_SUCCESS);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;

	// cleanup: 
	//   use test_free_polv for freeing up POLV memory 
	//   and test_free_host for host memory. Of course,
	//   manual polvCoreMemoryFree() calls can be used.
	if (state->initialized)
	{
		// place cleanup code here, it's more safe
		if (state->context)
			polvCoreContextDestroy(&state->context);
		polvCoreFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("Template/new_core", run, cleanup, &state);
}
