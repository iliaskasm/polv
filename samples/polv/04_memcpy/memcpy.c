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
#include <polv.h>
#include "../common.h"

#define TEST_CLEANUP() do { \
	polvFree(d_src); polvFree(d_dst); \
	free(src); free(dst); \
	polvFinalize(); \
} while (0)

int main(void)
{
	const size_t n = 1024;
	const size_t bytes = n * sizeof(float);
	float *src = NULL, *dst = NULL;
	void *d_src = NULL, *d_dst = NULL;
	size_t i;

	/* Host memory allocation/initialization */
	src = (float *) smalloc(bytes);
	dst = (float *) smalloc(bytes);

	for (i = 0; i < n; ++i)
	{
		src[i] = (float)(i * 3);
		dst[i] = 0.0f;
	}

	/* POLV initialization */
	TEST_POLV_CHECK(polvInit(),	"polvInit failed");
	TEST_POLV_CHECK(polvSetDevice(0), "polvSetDevice failed");

	/* Device memory allocation */
	d_src = polvAlloc(bytes, polvMemDeviceLocal);
	d_dst = polvAlloc(bytes, polvMemDeviceLocal);
	TEST_CHECK(d_src && d_dst, "device memory allocation failed");

	/* Host-to-device transfer */
	TEST_POLV_CHECK(polvMemcpy(src, 0, d_src, 0, bytes, polvHostToDevice),
		"H2D failed");

	/* Device-to-device transfer */
	TEST_POLV_CHECK(polvMemcpy(d_src, 0, d_dst, 0, bytes, polvDeviceToDevice),
		"D2D failed");

	/* Device-to-host transfer */
	TEST_POLV_CHECK(polvMemcpy(d_dst, 0, dst, 0, bytes, polvDeviceToHost),
		"D2H failed");

	/* Verification */
	for (i = 0; i < n; ++i)
		if (dst[i] != src[i])
			TEST_FAIL("mismatch at %zu: %f != %f",
			          i, (double) dst[i], (double) src[i]);

	printf("OK\n");
	TEST_SUCCESS(); // cleanup and return
}