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
#include <stdio.h>
#include <stdint.h>
#include <polv.h>
#include "../common.h"

#define TEST_CLEANUP() do { \
	polvFree(d_a); polvFree(d_b); polvFree(d_c); \
	polvFinalize(); \
} while (0)

int main(void)
{
	const size_t n = 1000;
	const uint32_t nthreads = 128;
	const size_t bytes = n * sizeof(float);
	float *a = NULL, *b = NULL, *c = NULL;
	void *d_a = NULL, *d_b = NULL, *d_c = NULL;
	float delta, expected;
	size_t i;

	/* POLV initialization */
	TEST_POLV_CHECK(polvInit(), "polvInit failed");
	TEST_POLV_CHECK(polvSetDevice(0), "polvSetDevice failed");

	/* Memory allocation (host-coherent memory) */
	d_a = polvAlloc(bytes, polvMemHostCoherent);
	d_b = polvAlloc(bytes, polvMemHostCoherent);
	d_c = polvAlloc(bytes, polvMemHostCoherent);
	TEST_CHECK(d_a && d_b && d_c, "host-coherent allocation failed");

	/* Get host pointers from allocated host-coherent memory */
	a = (float *) polvGetHostPointer(d_a);
	b = (float *) polvGetHostPointer(d_b);
	c = (float *) polvGetHostPointer(d_c);
	TEST_CHECK(a && b && c,	"failed to get host pointers");

	/* Initialization */
	for (i = 0; i < n; ++i)
	{
		a[i] = (float) i;
		b[i] = (float) (2 * i);
		c[i] = 0.0f;
	}

	void *args[] = { d_a, d_b, d_c };
	POLVDim grid = { (uint32_t) ((n + nthreads - 1) / nthreads), 1, 1 };
	POLVDim group = { nthreads, 1, 1 };

	/* Kernel launch */
	TEST_POLV_CHECK(
		polvKernelLaunch("vecadd.spv", args, 3, grid, group),
		"polvKernelLaunch failed"
	);

	/* Verification */
	for (i = 0; i < n; ++i)
	{
		expected = a[i] + b[i];
		delta = c[i] - expected;

		if (delta < -ERROR_THRESHOLD || delta > ERROR_THRESHOLD)
			TEST_FAIL("wrong result at %zu: %f expected %f",
			          i, (double) c[i], (double) expected);
	}

	printf("OK\n");
	TEST_SUCCESS(); // cleanup and return
}