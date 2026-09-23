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
 * POLV Core status string tests
 */
#include <stddef.h>
#include "test.h"

typedef struct {
	int ignore;
} State;


static int run(TestContext *test, void *opaque)
{
	State *state = (State *) opaque;

	const POLVCoreResult statuses[] = {
		POLV_CORE_SUCCESS,
		POLV_CORE_ERROR,
		POLV_CORE_ERROR_NOT_INITIALIZED,
		POLV_CORE_ERROR_INVALID_ARGUMENT,
		POLV_CORE_ERROR_NO_DEVICE,
		POLV_CORE_ERROR_VULKAN,
		POLV_CORE_ERROR_OUT_OF_MEMORY,
		POLV_CORE_ERROR_SHADER,
		POLV_CORE_ERROR_UNSUPPORTED,
		POLV_CORE_ERROR_DEVICE_ID_OUT_OF_BOUNDS,
		POLV_CORE_ERROR_CONTEXT_MISMATCH,
		POLV_CORE_ERROR_CONTEXT_NOT_INITIALIZED
	};
	const size_t count = sizeof(statuses) / sizeof(statuses[0]);
	size_t i;

	// Verify that every public status code has a non-empty message
	for (i = 0; i < count; i++)
	{
		const char *message = polvCoreStatus(statuses[i]);

		TEST_REQUIRE(test, message != NULL);
		TEST_REQUIRE(test, message[0] != '\0');
	}

	state->ignore = 0x999;
	return 1;
}


int main(void)
{
	State state = { 0 };
	
	return test_run("api/status", run, NULL, &state);
}
