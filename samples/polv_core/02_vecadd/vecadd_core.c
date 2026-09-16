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
#include <stdlib.h>
#include <polv_core.h>
#include "../common.h"

#define TEST_CLEANUP() do {\
	polvCoreMemoryFree(d_a); \
	polvCoreMemoryFree(d_b); \
	polvCoreMemoryFree(d_c); \
	free(a); \
	free(b); \
	free(c); \
	polvCoreKernelDestroy(&kernel); \
	polvCoreContextDestroy(&context); \
} while(0)

int main(void)
{
	const size_t n = 1000, nthreads = 128;
	const size_t bytes = n * sizeof(float);
	POLVCoreKernel *kernel = NULL;
	POLVCoreDevice *device = NULL;
	POLVCoreContext *context = NULL;
	POLVCoreMemory *d_a = NULL, *d_b = NULL, *d_c = NULL; // device
	float *a = NULL, *b = NULL, *c = NULL; // host
	size_t i;

	/* Host memory allocation/initialization */
	a = (float *) smalloc(bytes);
	b = (float *) smalloc(bytes);
	c = (float *) smalloc(bytes);

	for (i = 0; i < n; ++i) 
	{
		a[i] = (float) i;
		b[i] = (float)(2 * i);
		c[i] = 0.0f;
	}

	/* POLV Core initialization */
	TEST_POLVC_CHECK(polvCoreInit(), "polvCoreInit failed");
	TEST_CHECK((device = polvCoreGetDevice(0)), "no device 0");
	TEST_POLVC_CHECK(polvCoreContextCreate(&context, device),
		"polvCoreContextCreate failed");

	/* Device memory allocation */
	d_a = polvCoreMemoryAllocDeviceLocal(bytes);
	d_b = polvCoreMemoryAllocDeviceLocal(bytes);
	d_c = polvCoreMemoryAllocDeviceLocal(bytes);
	TEST_CHECK(d_a && d_b && d_c, "device memory allocation failed");

	/* Host-to-device transfers (a, b) */
	TEST_POLVC_CHECK(polvCoreMemoryCopyH2D(a, 0, d_a, 0, bytes),
		"H2D(a) failed");
	TEST_POLVC_CHECK(polvCoreMemoryCopyH2D(b, 0, d_b, 0, bytes),
		"H2D(b) failed");

	/* Kernel creation and launch */
	void *args[] = { d_a, d_b, d_c };
	TEST_POLVC_CHECK(polvCoreKernelCreate(&kernel, "vecadd.spv", 3),
		"kernel creation failed");
	TEST_POLVC_CHECK(polvCoreKernelLaunch(kernel, args, 
		(uint32_t) ((n + nthreads - 1) / nthreads), 1, 1,
		nthreads, 1, 1), "kernel launch failed");

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
