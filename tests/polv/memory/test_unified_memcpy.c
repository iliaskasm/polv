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
 * Host-to-device, device-to-host and device-to-device copies using
 * host-coherent memory
 */
#include <stdlib.h>
#include "test.h"

typedef struct {
	int initialized;
	float *host_src;
	float *host_dst;
	float *mapped_src;
	float *mapped_dst;
	void *device_src;
	void *device_dst;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	const size_t n = 2048;
	const size_t bytes = n * sizeof(float);

	// Initialize host memory
	state->host_src = malloc(bytes);
	state->host_dst = malloc(bytes);
	TEST_REQUIRE(test, state->host_src != NULL);
	TEST_REQUIRE(test, state->host_dst != NULL);

	// Initialize POLV
	TEST_REQUIRE_SUCCESS(test, polvInit());
	state->initialized = 1;

	// Allocate host-coherent device memory
	state->device_src = polvAlloc(bytes, polvMemHostCoherent);
	state->device_dst = polvAlloc(bytes, polvMemHostCoherent);
	TEST_REQUIRE(test, state->device_src != NULL);
	TEST_REQUIRE(test, state->device_dst != NULL);

	// Get mapped host pointers
	state->mapped_src = polvGetHostPointer(state->device_src);
	state->mapped_dst = polvGetHostPointer(state->device_dst);
	TEST_REQUIRE(test, state->mapped_src != NULL);
	TEST_REQUIRE(test, state->mapped_dst != NULL);

	// Host to device
	test_fill_float(state->host_src, n, 0.25f, 1.0f);
	test_fill_float(state->mapped_dst, n, 0.0f, 0.0f);

	if (TEST_EXPECT_RESULT(test, polvMemcpy(state->host_src, 0, state->device_dst, 0, bytes, polvHostToDevice),
	                             POLV_SUCCESS))
		TEST_EXPECT(test, TEST_COMPARE_FLOAT(test, state->mapped_dst, state->host_src, n, 0.0f));

	// Device to host
	test_fill_float(state->mapped_src, n, 0.5f, -3.0f);
	test_fill_float(state->host_dst, n, 0.0f, 0.0f);

	if (TEST_EXPECT_RESULT(test, polvMemcpy(state->device_src, 0, state->host_dst, 0, bytes, polvDeviceToHost),
	                             POLV_SUCCESS))
		TEST_EXPECT(test, TEST_COMPARE_FLOAT(test, state->host_dst, state->mapped_src, n, 0.0f));

	// Device to device
	test_fill_float(state->mapped_src, n, 1.25f, 7.0f);
	test_fill_float(state->mapped_dst, n, 0.0f, 0.0f);

	if (TEST_EXPECT_RESULT(test, polvMemcpy(state->device_src, 0, state->device_dst, 0, bytes, polvDeviceToDevice),
	                             POLV_SUCCESS))
		TEST_EXPECT(test, TEST_COMPARE_FLOAT(test, state->mapped_dst, state->mapped_src, n, 0.0f));

	return 1;
}


static void cleanup(void *opaque)
{
	State *state = opaque;
	void *host_ptrs[] = { state->host_src, state->host_dst };
	void *polv_ptrs[] = { state->device_src, state->device_dst };

	if (state->initialized)
	{
		test_free_polv(polv_ptrs, 2);
		polvFinalize();
	}

	test_free_host(host_ptrs, 2);
}


int main(void)
{
	State state = { 0 };

	return test_run("memory/unified_memcpy", run, cleanup, &state);
}