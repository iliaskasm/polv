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
	polvCoreMemoryFree(d_x); \
	polvCoreMemoryFree(d_y); \
	polvCoreMemoryFree(d_out); \
	polvCoreMemoryFree(d_alpha); \
	free(x); \
	free(y); \
	free(out); \
	polvCoreKernelDestroy(&kernel); \
	polvCoreContextDestroy(&context); \
	polvCoreFinalize(); \
} while(0);

int main(void)
{
	const size_t n = 4096, threads = 128;
	const size_t bytes = n * sizeof(float);
	const float alpha = 2.5f;
	POLVCoreDevice *device = NULL;
	POLVCoreContext *context = NULL;
	POLVCoreKernel *kernel = NULL;
	POLVCoreMemory *d_x = NULL, *d_y = NULL, *d_alpha = NULL, *d_out = NULL;
	float *x = NULL, *y = NULL, *out = NULL;
	size_t i;

	/* Host memory allocation/initialization */
	x = (float *) smalloc(bytes);
	y = (float *) smalloc(bytes);
	out = (float *) smalloc(bytes);

	for (i = 0; i < n; ++i)
	{
		x[i] = (float) i * 0.25f;
		y[i] = (float) i * 0.5f;
		out[i] = 0.0f;
	}

	/* POLV Core initialization */
	TEST_POLVC_CHECK(polvCoreInit(), "polvCoreInitFailed");
	TEST_CHECK((device = polvCoreGetDevice(0)), "no device 0");
	TEST_POLVC_CHECK(polvCoreContextCreate(&context, device),
		"polvCoreContextCreate failed");

	/* Device/host-coherent memory allocation */
	d_x = polvCoreMemoryAllocDeviceLocal(bytes);
	d_y = polvCoreMemoryAllocDeviceLocal(bytes);
	d_out = polvCoreMemoryAllocDeviceLocal(bytes);
	d_alpha = polvCoreMemoryAllocHostCoherent(sizeof(alpha));
	TEST_CHECK(d_x && d_y && d_out && d_alpha, 
		"device/host-coherent memory allocation failed");

	/* Obtain host pointer from host-coherent memory */
	float *alpha_ptr = (float *) polvCoreMemoryGetHostPointer(d_alpha);
	TEST_CHECK(alpha_ptr, "failed to map alpha buffer");
	*alpha_ptr = alpha;

	/* Host-to-device transfers (x, y) */
	TEST_POLVC_CHECK(polvCoreMemoryCopyH2D(x, 0, d_x, 0, bytes), 
		"H2D(x) failed");
	TEST_POLVC_CHECK(polvCoreMemoryCopyH2D(y, 0, d_y, 0, bytes), 
		"H2D(y) failed");

	/* Kernel creation and launch */
	TEST_POLVC_CHECK(polvCoreKernelCreate(&kernel, "saxpy.spv", 4),
		"kernel creation failed");
	void *args[] = { d_x, d_y, d_alpha, d_out };
	TEST_POLVC_CHECK(polvCoreKernelLaunch(kernel, args,
		(uint32_t)((n + threads - 1) / threads), 1, 1,
		threads, 1, 1), "kernel launch failed");

	/* Device-to-host transfer (out) */
	TEST_POLVC_CHECK(polvCoreMemoryCopyD2H(d_out, 0, out, 0, bytes), 
		"D2H(out) failed");

	/* Verification */
	for (i = 0; i < n; ++i)
	{
		float expected = alpha * x[i] + y[i];
		float delta = out[i] - expected;
		if (delta < -ERROR_THRESHOLD || delta > ERROR_THRESHOLD)
			TEST_FAIL("wrong result at %zu: %f expected %f",
			          i, (double) out[i], (double) expected);
	}

	printf("OK\n");
	TEST_SUCCESS(); // cleanup and return
}
