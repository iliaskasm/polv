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
	polvFree(d_dims); \
	free(a); \
	free(b); \
	free(c); \
	polvFinalize(); \
} while (0)

int main(void)
{
	const uint32_t rows = 257, cols = 513,
	               group_x = 16, group_y = 16;
	const size_t n = (size_t) rows * (size_t) cols;
	const size_t bytes = n * sizeof(float);
	float *a = NULL, *b = NULL, *c = NULL;
	void *d_a = NULL, *d_b = NULL, *d_c = NULL, *d_dims = NULL;
	size_t i;

	/* Host memory allocation/initialization */
	a = (float *) smalloc(bytes);
	b = (float *) smalloc(bytes);
	c = (float *) smalloc(bytes);

	for (i = 0; i < n; ++i)
	{
		a[i] = (float) i * 0.25f;
		b[i] = (float) i * 0.75f;
		c[i] = 0.0f;
	}

	/* POLV initialization */
	TEST_POLV_CHECK(polvInit(), "polvInit failed");
	TEST_POLV_CHECK(polvSetDevice(0), "polvSetDevice failed");

	/* Device memory allocation */
	d_a = polvAlloc(bytes, polvMemDeviceLocal);
	d_b = polvAlloc(bytes, polvMemDeviceLocal);
	d_c = polvAlloc(bytes, polvMemDeviceLocal);
	d_dims = polvAlloc(2 * sizeof(uint32_t), polvMemHostCoherent);
	TEST_CHECK(d_a && d_b && d_c && d_dims, "memory allocation failed");

	/* Matrix dims */
	uint32_t *dims = (uint32_t *) polvGetHostPointer(d_dims);
	TEST_CHECK(dims, "failed to map dims buffer");

	dims[0] = rows;
	dims[1] = cols;

	/* Host-to-device transfers (a, b) */
	TEST_POLV_CHECK(polvMemcpy(a, 0, d_a, 0, bytes, polvHostToDevice),
		"H2D(a) memory copy failed");
	TEST_POLV_CHECK(polvMemcpy(b, 0, d_b, 0, bytes, polvHostToDevice),
		"H2D(b) memory copy failed");

	/* Kernel launch */
	void *args[] = { d_a, d_b, d_c, d_dims };
	POLVDim group = { group_x, group_y, 1 };
	POLVDim grid = {
		(cols + group_x - 1) / group_x,
		(rows + group_y - 1) / group_y,
		1
	};

	TEST_POLV_CHECK(polvKernelLaunch("matadd.spv", args, 4, grid, group),
		"kernel launch failed");

	/* Device-to-host transfer (c) */
	TEST_POLV_CHECK(polvMemcpy(d_c, 0, c, 0, bytes, polvDeviceToHost),
		"D2H(c) memory copy failed");

	/* Verification */
	for (i = 0; i < n; ++i)
	{
		float expected = a[i] + b[i];
		float delta = c[i] - expected;

		if (delta < -ERROR_THRESHOLD || delta > ERROR_THRESHOLD)
			TEST_FAIL("wrong result at %zu: %f expected %f",
			          i, (double) c[i], (double) expected);
	}

	printf("OK\n");

	TEST_SUCCESS(); // cleanup and return
}