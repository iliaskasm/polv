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
#include <math.h>
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

static float matmul_reference(const float *a, const float *b,
                              uint32_t row, uint32_t col,
                              uint32_t n, uint32_t k)
{
	float sum = 0.0f;
	uint32_t i;

	for (i = 0; i < k; ++i)
		sum += a[row * k + i] * b[i * n + col];

	return sum;
}

int main(void)
{
	const uint32_t m = 64, n = 64, k = 64;
	const uint32_t group_x = 16, group_y = 16;
	const uint32_t dims[3] = { m, n, k };
	const size_t a_bytes = (size_t) m * k * sizeof(float),
	             b_bytes = (size_t) k * n * sizeof(float),
	             c_bytes = (size_t) m * n * sizeof(float),
	             dims_bytes = sizeof(dims);
	float *a = NULL, *b = NULL, *c = NULL;
	void *d_a = NULL, *d_b = NULL, *d_c = NULL, *d_dims = NULL;
	uint32_t row, col;
	size_t i;

	/* Host memory allocation/initialization */
	a = (float *) smalloc(a_bytes);
	b = (float *) smalloc(b_bytes);
	c = (float *) smalloc(c_bytes);

	for (i = 0; i < (size_t) m * k; ++i)
		a[i] = (float) i;

	for (i = 0; i < (size_t) k * n; ++i)
		b[i] = (float)(i + 1);

	for (i = 0; i < (size_t) m * n; ++i)
		c[i] = 0.0f;

	/* POLV initialization */
	TEST_POLV_CHECK(polvInit(), "polvInit failed");
	TEST_POLV_CHECK(polvSetDevice(0), "polvSetDevice failed");

	/* Device memory allocation */
	d_a = polvAlloc(a_bytes, polvMemDeviceLocal);
	d_b = polvAlloc(b_bytes, polvMemDeviceLocal);
	d_c = polvAlloc(c_bytes, polvMemDeviceLocal);
	d_dims = polvAlloc(dims_bytes, polvMemDeviceLocal);
	TEST_CHECK(d_a && d_b && d_c && d_dims,	"device memory allocation failed");

	/* Host-to-device transfers (a, b, dims) */
	TEST_POLV_CHECK(polvMemcpy(a, 0, d_a, 0, a_bytes, polvHostToDevice),
		"H2D(A) failed");

	TEST_POLV_CHECK(polvMemcpy(b, 0, d_b, 0, b_bytes, polvHostToDevice),
		"H2D(B) failed");

	TEST_POLV_CHECK(polvMemcpy((void *) dims, 0, d_dims, 0, dims_bytes, polvHostToDevice),
		"H2D(dims) failed");

	/* Kernel launch */
	void *args[] = { d_a, d_b, d_c, d_dims };
	POLVDim group = { group_x, group_y, 1 };
	POLVDim grid = { (n + group_x - 1) / group_x, (m + group_y - 1) / group_y, 1 };

	printf("Matrix multiplication: %ux%u * %ux%u\n", m, k, k, n);
	printf("grid  = (%u, %u, %u)\n", grid.x, grid.y, grid.z);
	printf("group = (%u, %u, %u)\n", group.x, group.y, group.z);

	TEST_POLV_CHECK(polvKernelLaunch("matmul.spv", args, 4, grid, group),
		"kernel launch failed");

	/* Device-to-host transfer (c) */
	TEST_POLV_CHECK(polvMemcpy(d_c, 0, c, 0, c_bytes, polvDeviceToHost),
		"D2H(C) failed");

	/* Verification */
	for (row = 0; row < m; ++row)
	{
		for (col = 0; col < n; ++col)
		{
			float expected = matmul_reference(a, b, row, col, n, k);
			float actual = c[row * n + col];
			float delta = fabsf(actual - expected);

			if (delta > ERROR_THRESHOLD)
				TEST_FAIL("wrong result at C[%u][%u]: got %f, expected %f",
				          row, col, (double) actual, (double) expected);
		}
	}

	printf("OK\n");
	TEST_SUCCESS(); // cleanup and return
}