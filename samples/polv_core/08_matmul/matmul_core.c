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
#include <polv_core.h>
#include "../common.h"

#define TEST_CLEANUP() do { \
	polvCoreMemoryFree(d_a); \
	polvCoreMemoryFree(d_b); \
	polvCoreMemoryFree(d_c); \
	polvCoreMemoryFree(d_dims); \
	free(a); \
	free(b); \
	free(c); \
	polvCoreKernelDestroy(&kernel); \
	polvCoreContextDestroy(&context); \
	polvCoreFinalize(); \
} while(0)

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
	const uint32_t m = 64, n = 64, k = 64,
	               group_x = 16, group_y = 16;
	const size_t a_bytes = (size_t) m * k * sizeof(float),
	             b_bytes = (size_t) k * n * sizeof(float),
	             c_bytes = (size_t) m * n * sizeof(float),
	             dims_bytes = 3 * sizeof(uint32_t);

	POLVCoreDevice *device = NULL;
	POLVCoreContext *context = NULL;
	POLVCoreKernel *kernel = NULL;
	POLVCoreMemory *d_a = NULL, *d_b = NULL, *d_c = NULL, *d_dims = NULL;
	float *a = NULL, *b = NULL, *c = NULL;
	uint32_t row, col;
	size_t i;

	/* Host memory allocation/initialization */
	a = (float *) smalloc(a_bytes);
	b = (float *) smalloc(b_bytes);
	c = (float *) smalloc(c_bytes);

	for (i = 0; i < m * k; ++i)
		a[i] = i;

	for (i = 0; i < k * n; ++i)
		b[i] = i + 1;

	for (i = 0; i < m * n; ++i)
		c[i] = 0.0f;

	/* POLV Core initialization */
	TEST_POLVC_CHECK(polvCoreInit(), "polvCoreInit failed");
	TEST_CHECK((device = polvCoreGetDevice(0)), "polvCoreSetDevice failed");
	TEST_POLVC_CHECK(polvCoreContextCreate(&context, device),
		"polvCoreContextCreate failed");

	d_a = polvCoreMemoryAllocDeviceLocal(a_bytes);
	d_b = polvCoreMemoryAllocDeviceLocal(b_bytes);
	d_c = polvCoreMemoryAllocDeviceLocal(c_bytes);
	d_dims = polvCoreMemoryAllocHostCoherent(dims_bytes);
	TEST_CHECK(d_a && d_b && d_c && d_dims, "device memory allocation failed");

	TEST_POLVC_CHECK(polvCoreMemoryCopyH2D(a, 0, d_a, 0, a_bytes),
		"H2D(a) failed");
	TEST_POLVC_CHECK(polvCoreMemoryCopyH2D(b, 0, d_b, 0, b_bytes),
		"H2D(b) failed");

	/* Obtain host pointer from host-coherent memory */
	uint32_t *dims = (uint32_t *) polvCoreMemoryGetHostPointer(d_dims);
	TEST_CHECK(dims, "failed to map dims buffer");
	dims[0] = m;
	dims[1] = n;
	dims[2] = k;

	/* Kernel creation and launch */
	void *args[4] = { d_a, d_b, d_c, d_dims };
	int group[3] = { group_x, group_y, 1 };
	int grid[3] = { (n + group_x - 1) / group_x, (m + group_y - 1) / group_y, 1 };

	printf("Matrix multiplication: %ux%u * %ux%u\n", m, k, k, n);
	printf("grid  = (%u, %u, %u)\n", grid[0], grid[1], grid[2]);
	printf("group = (%u, %u, %u)\n", group[0], group[1], group[2]);

	TEST_POLVC_CHECK(polvCoreKernelCreate(&kernel, "matmul.spv", 4),
		"kernel creation failed");
	TEST_POLVC_CHECK(polvCoreKernelLaunch(kernel, args, grid[0], grid[1], grid[2],
		group[0], group[1], group[2]), 
		"kernel launch failed");
		
	/* Device-to-host transfer (c) */
	TEST_POLVC_CHECK(polvCoreMemoryCopyD2H(d_c, 0, c, 0, c_bytes),
		"D2H(c) failed");

	/* Verification */
	for (row = 0; row < m; ++row)
	{
		for (col = 0; col < n; ++col)
		{
			float expected = matmul_reference(a, b, row, col, n, k);
			float actual = c[row * n + col];
			float delta = fabsf(actual - expected);

			if (delta > ERROR_THRESHOLD)
				TEST_FAIL("wrong result at %zu: %f expected %f",
				          i, (double) actual, (double) expected);
		}
	}

	printf("OK\n");
	TEST_SUCCESS(); // cleanup and return
}