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
	polvFree(d_x); \
	polvFree(d_y); \
	polvFree(d_alpha); \
	polvFree(d_out); \
	free(x); \
	free(y); \
	free(out); \
	polvFinalize(); \
} while (0)

int main(void)
{
	const size_t n = 4096;
	const uint32_t threads = 128;
	const size_t bytes = n * sizeof(float);
	const float alpha = 2.5f;
	float *x = NULL, *y = NULL, *out = NULL;
	void *d_x = NULL, *d_y = NULL, *d_alpha = NULL, *d_out = NULL;
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

	/* POLV initialization */
	TEST_POLV_CHECK(polvInit(),	"polvInit failed");
	TEST_POLV_CHECK(polvSetDevice(0), "polvSetDevice failed");
	
	/* Memory allocation (device/host-coherent memory) */
	d_x = polvAlloc(bytes, polvMemDeviceLocal);
	d_y = polvAlloc(bytes, polvMemDeviceLocal);
	d_out = polvAlloc(bytes, polvMemDeviceLocal);
	d_alpha = polvAlloc(sizeof(alpha), polvMemHostCoherent);
	TEST_CHECK(d_x && d_y && d_alpha && d_out,
		"memory allocation failed");

	/* Get alpha */
	float *alpha_ptr = (float *) polvGetHostPointer(d_alpha);
	TEST_CHECK(alpha_ptr, "failed to map alpha buffer");
	*alpha_ptr = alpha;

	/* Host-to-device transfers (x, y) */
	TEST_POLV_CHECK(polvMemcpy(x, 0, d_x, 0, bytes, polvHostToDevice),
		"H2D(x) memory copy failed");
	TEST_POLV_CHECK(polvMemcpy(y, 0, d_y, 0, bytes, polvHostToDevice),
		"H2D(y) memory copy failed");

	/* Kernel launch */
	void *args[] = { d_x, d_y, d_alpha, d_out };
	POLVDim group = { threads, 1, 1 };
	POLVDim grid = { (uint32_t) ((n + threads - 1) / threads), 1, 1	};
	TEST_POLV_CHECK(polvKernelLaunch("saxpy.spv", args, 4, grid, group),
		"kernel launch failed");

	/* Device-to-host transfer (out) */
	TEST_POLV_CHECK(polvMemcpy(d_out, 0, out, 0, bytes, polvDeviceToHost),
		"D2H(out) memory copy failed");

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