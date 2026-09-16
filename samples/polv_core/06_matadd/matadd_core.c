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
#include <polv_core.h>
#include "../common.h"

#define TEST_CLEANUP() do {\
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
} while(0);

int main(void)
{
	const uint32_t rows = 257, cols = 513,
	               group_x = 16, group_y = 16;
	const size_t n = (size_t) rows * (size_t) cols;
	const size_t bytes = n * sizeof(float);
	POLVCoreDevice *device = NULL;
	POLVCoreContext *context = NULL;
	POLVCoreKernel *kernel = NULL;
	POLVCoreMemory *d_a = NULL, *d_b = NULL, *d_c = NULL, *d_dims = NULL;
	float *a = NULL, *b = NULL, *c = NULL;
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

	/* POLV Core initialization */
	TEST_POLVC_CHECK(polvCoreInit(), "polvCoreInitFailed");
	TEST_CHECK((device = polvCoreGetDevice(0)), "no device 0");
	TEST_POLVC_CHECK(polvCoreContextCreate(&context, device),
		"polvCoreContextCreate failed");

	/* Device/host-coherent memory allocation */
	d_a = polvCoreMemoryAllocDeviceLocal(bytes);
	d_b = polvCoreMemoryAllocDeviceLocal(bytes);
	d_c = polvCoreMemoryAllocDeviceLocal(bytes);
	d_dims = polvCoreMemoryAllocHostCoherent(2 * sizeof(uint32_t));
	TEST_CHECK(d_a && d_b && d_c && d_dims, 
		"device/host-coherent memory allocation failed");

	/* Obtain host pointer from host-coherent memory */
	uint32_t *dims = (uint32_t *) polvCoreMemoryGetHostPointer(d_dims);
	TEST_CHECK(dims, "failed to map dims buffer");
	dims[0] = rows;
	dims[1] = cols;

	/* Host-to-device transfer (a, b) */
	TEST_POLVC_CHECK(polvCoreMemoryCopyH2D(a, 0, d_a, 0, bytes),
		"H2D(a) failed");
	TEST_POLVC_CHECK(polvCoreMemoryCopyH2D(b, 0, d_b, 0, bytes),
		"H2D(b) failed");

	/* Kernel creation and launch */
	TEST_POLVC_CHECK(polvCoreKernelCreate(&kernel, "matadd.spv", 4),
		"kernel creation failed");
	void *args[] = { d_a, d_b, d_c, d_dims };
	TEST_POLVC_CHECK(polvCoreKernelLaunch(kernel, args,
		(cols + group_x - 1) / group_x,
		(rows + group_y - 1) / group_y,
		1,
		group_x, group_y, 1), "kernel launch failed");

	/* Device-to-host transfer (c) */
	TEST_POLVC_CHECK(polvCoreMemoryCopyD2H(d_c, 0, c, 0, bytes),
		"D2H(c) failed");

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
