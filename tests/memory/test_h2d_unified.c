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
 * Host-to-device using host-coherent memory
 */
#include <stdlib.h>
#include "test.h"

typedef struct {
	int initialized;
	float *src;
	float *mapped;
	void *device;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const size_t n = 1024;
	const size_t bytes = n * sizeof(float);

	// Initialize host
	state->src = malloc(bytes);
	TEST_REQUIRE(test, state->src != NULL);
	test_fill_float(state->src, n, 0.25f, 1.0f);

	// Initialize POLV
	TEST_REQUIRE_RESULT(test, polvInit(), POLV_SUCCESS);
	state->initialized = 1;

	// Allocate host-coherent memory for DEVICE device
	state->device = polvAlloc(bytes, polvMemHostCoherent);
	TEST_REQUIRE(test, state->device != NULL);

	// HOST src -> DEVICE device
	TEST_REQUIRE_RESULT(test, polvMemcpy(state->src, 0, state->device, 0, bytes, polvHostToDevice),
	                          POLV_SUCCESS);

	// Get host pointer for mapped
	state->mapped = polvGetHostPointer(state->device);
	TEST_REQUIRE(test, state->mapped != NULL);

	// Direct comparison due to host-coherent memory
	TEST_REQUIRE(test, TEST_COMPARE_FLOAT(test, state->mapped, state->src, n, 0.0f));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *polv_ptrs[] = { state->device };
	void *host_ptrs[] = { state->src };

	if (state->initialized)
	{
		test_free_polv(polv_ptrs, 1);
		polvFinalize();
	}

	test_free_host(host_ptrs, 1);
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/h2d_unified", run, cleanup, &state);
}