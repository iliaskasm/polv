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
 * Host-visible memory test
 */
#include "test.h"

typedef struct {
	int initialized;
	void *allocation;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const size_t n = 256;
	float *ptr, *ptr2;
	size_t i;

	// Initialize POLV
	TEST_REQUIRE_SUCCESS(test, polvInit());
	state->initialized = 1;
	
	// Allocate host-visible memory
	state->allocation = polvAlloc(n * sizeof(float), polvMemHostVisible);
	TEST_REQUIRE(test, state->allocation != NULL);

	// Get host pointers
	ptr = polvGetHostPointer(state->allocation);
	ptr2 = polvGetHostPointer(state->allocation);
	TEST_REQUIRE(test, ptr != NULL);
	TEST_REQUIRE(test, ptr2 == ptr);

	// Initialize host-visible memory
	for (i = 0; i < n; i++)
		ptr[i] = (float) i * 0.75f;

	// Verification
	for (i = 0; i < n; i++)
		TEST_REQUIRE(test, ptr2[i] == (float) i * 0.75f);

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;

	if (state->initialized)
	{
		if (state->allocation)
			polvFree(state->allocation);
		polvFinalize();
	}
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/host_visible", run, cleanup, &state);
}
