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
 * POLV Core host-to-host memory copy
 */
#include <stdint.h>
#include "test.h"


typedef struct {
	int initialized;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = opaque;
	uint32_t src[32];
	uint32_t dst[32] = { 0 };
	const size_t src_offset = 3 * sizeof(uint32_t);
	const size_t dst_offset = 5 * sizeof(uint32_t);
	const size_t count = 16;
	const size_t bytes = count * sizeof(uint32_t);
	size_t i;

	// Initialize POLV Core
	TEST_REQUIRE_SUCCESS(test, polvCoreInit());
	state->initialized = 1;

	// Initialize source data
	for (i = 0; i < 32; i++)
		src[i] = 0x13579bdfu ^ (uint32_t) i;

	// HOST src -> HOST dst using non-zero offsets
	TEST_REQUIRE_SUCCESS(test, polvCoreMemoryCopyH2H(src, src_offset, dst, dst_offset, bytes));

	// Verify the copied range
	for (i = 0; i < count; i++)
		TEST_REQUIRE(test, dst[5 + i] == src[3 + i]);

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

	return test_run("memory/h2h", run, cleanup, &state);
}
