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
 * POLV Core memory allocation tests
 */
#include "test.h"

typedef struct {
	int initialized;
	POLVCoreContext *context;
	POLVCoreMemory *ptrs[4];
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	POLVCoreDevice *device;
	const VkDeviceSize bytes = 4096;

	// Initialize POLV Core
	TEST_REQUIRE_SUCCESS(test, polvCoreInit());
	state->initialized = 1;

	// Get device and create context
	TEST_REQUIRE(test, polvCoreGetNumDevices() > 0);
	device = polvCoreGetDevice(0);
	TEST_REQUIRE(test, device != NULL);
	TEST_REQUIRE_SUCCESS(test, polvCoreContextCreate(&state->context, device));
	TEST_REQUIRE_SUCCESS(test, polvCoreContextSetCurrent(state->context));

	// Test the available memory allocation types
	state->ptrs[0] = polvCoreMemoryAllocDeviceLocal(bytes);
	state->ptrs[1] = polvCoreMemoryAllocHostVisible(bytes);
	state->ptrs[2] = polvCoreMemoryAllocHostCoherent(bytes);
	state->ptrs[3] = polvCoreMemoryAlloc(bytes);
	TEST_REQUIRE(test, state->ptrs[0] != NULL);
	TEST_REQUIRE(test, state->ptrs[1] != NULL);
	TEST_REQUIRE(test, state->ptrs[2] != NULL);
	TEST_REQUIRE(test, state->ptrs[3] != NULL);

	// Host-coherent and default allocations must expose host pointers
	TEST_REQUIRE(test, polvCoreMemoryGetHostPointer(state->ptrs[2]) != NULL);
	TEST_REQUIRE(test, polvCoreMemoryGetHostPointer(state->ptrs[3]) != NULL);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->ptrs[0], state->ptrs[1], state->ptrs[2], state->ptrs[3] };

	if (state->initialized)
	{
		test_free_polv(polv_ptrs, 4);
		if (state->context)
			polvCoreContextDestroy(&state->context);
		polvCoreFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/alloc", run, cleanup, &state);
}
