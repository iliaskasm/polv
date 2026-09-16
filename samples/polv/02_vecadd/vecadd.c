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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <polv.h>
#include "../common.h"

#define TEST_CLEANUP() do { \
	polvFree(d_a); \
	polvFree(d_b); \
	polvFree(d_c); \
	free(a); free(b); free(c); \
	polvFinalize(); \
} while (0)

int main(void)
{
	const size_t n = 1024, nthreads = 128;
	const size_t bytes = n * sizeof(float);
	float *a = NULL, *b = NULL, *c = NULL;
	void *d_a = NULL, *d_b = NULL, *d_c = NULL;
	float expected, delta;
	size_t i;

	/* Host memory allocation/initialization */
	a = (float *) smalloc(bytes);
	b = (float *) smalloc(bytes);
	c = (float *) smalloc(bytes);
	
	for (i = 0; i < n; ++i)
	{
		a[i] = (float) i;
		b[i] = (float) i;
		c[i] = 0.0f;
	}

	/* POLV initialization */
	TEST_POLV_CHECK(polvInit(),	"polvInit failed");
	TEST_POLV_CHECK(polvSetDevice(0), "polvSetDevice failed");

	/* Device memory allocation */
	d_a = polvAlloc(bytes, polvMemDeviceLocal);
	d_b = polvAlloc(bytes, polvMemDeviceLocal);
	d_c = polvAlloc(bytes, polvMemDeviceLocal);
	TEST_CHECK(d_a && d_b && d_c, "device memory allocation failed");

	/* Host-to-device transfers (a, b) */
	TEST_POLV_CHECK(polvMemcpy(a, 0, d_a, 0, bytes, polvHostToDevice),
		"H2D(a) failed");
	TEST_POLV_CHECK(polvMemcpy(b, 0, d_b, 0, bytes, polvHostToDevice),
		"H2D(b) failed");

	/* Kernel launch */
	void *args[] = { d_a, d_b, d_c };
	POLVDim grid = { (uint32_t) ((n + nthreads - 1) / nthreads), 1, 1 };
	POLVDim group = { nthreads, 1, 1 };
	TEST_POLV_CHECK(polvKernelLaunch("vecadd.spv", args, 3, grid, group),
		"dispatch failed");

	/* Device-to-host transfer (c) */
	TEST_POLV_CHECK(polvMemcpy(d_c, 0, c, 0, bytes, polvDeviceToHost),
		"D2H(c) failed");

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