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
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <polv.h>
#include "../common.h"

#define DEVICE_ID 0
#define DEFAULT_SIZE_MIB 64u
#define DEFAULT_ITERATIONS 20u
#define WARMUP_ITERATIONS 20u

#define TEST_CLEANUP() do { \
	polvFree(device_src); \
	polvFree(device_dst); \
	free(host_src); \
	free(host_dst); \
	polvFinalize(); \
} while (0)

static double now_seconds(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return -1.0;

	return (double) ts.tv_sec + (double) ts.tv_nsec * 1.0e-9;
}

static int parse_positive_size(const char *text, size_t *value)
{
	char *end = NULL;
	unsigned long long parsed;

	errno = 0;
	parsed = strtoull(text, &end, 10);

	if (errno != 0 || end == text || *end != '\0' || parsed == 0 ||
	    parsed > (unsigned long long)SIZE_MAX)
		return 0;

	*value = (size_t) parsed;
	return 1;
}

static void print_result(const char *label, size_t bytes, size_t iterations,
                         double seconds)
{
	const double total_bytes = (double) bytes * (double) iterations;
	const double gbps = total_bytes / seconds / 1.0e9;
	const double ms_per_copy = seconds * 1.0e3 / (double) iterations;

	printf("%-4s %8.2f GB/s  %8.3f ms/copy\n",
	       label, gbps, ms_per_copy);
}

/******************************************************************
 *                         Host to device                         *
 ******************************************************************/
static POLVResult host_to_device(const void *host_src, void *device_src,
                                 size_t bytes, size_t iterations)
{
	POLVResult err;
	double start, elapsed;
	size_t i;
	
	/* Warmup */
	for (i = 0; i < WARMUP_ITERATIONS; ++i)
	{
		if ((err = polvMemcpy((void *) host_src, 0, device_src, 0, bytes,
		                      polvHostToDevice)) != POLV_SUCCESS)
			return err;
	}

	/* Benchmark */
	start = now_seconds();

	for (i = 0; i < iterations; ++i)
	{
		if ((err = polvMemcpy((void *) host_src, 0, device_src, 0, bytes,
		                      polvHostToDevice)) != POLV_SUCCESS)
			return err;
	}

	elapsed = now_seconds() - start;
	print_result("H2D", bytes, iterations, elapsed);

	return POLV_SUCCESS;
}


/******************************************************************
 *                         Device to host                         *
 ******************************************************************/
static POLVResult device_to_host(void *device_src, void *host_dst,
                                 size_t bytes, size_t iterations)
{
	POLVResult err;
	double start, elapsed;
	size_t i;

	/* Warmup */
	for (i = 0; i < WARMUP_ITERATIONS; ++i)
	{
		if ((err = polvMemcpy(device_src, 0, host_dst, 0, bytes,
		                      polvDeviceToHost)) != POLV_SUCCESS)
			return err;
	}

	/* Benchmark */
	start = now_seconds();

	for (i = 0; i < iterations; ++i)
	{
		if ((err = polvMemcpy(device_src, 0, host_dst, 0, bytes,
		                      polvDeviceToHost)) != POLV_SUCCESS)
			return err;
	}

	elapsed = now_seconds() - start;
	print_result("D2H", bytes, iterations, elapsed);

	return POLV_SUCCESS;
}


/******************************************************************
 *                        Device to device                        *
 ******************************************************************/
static POLVResult device_to_device(void *device_src, void *device_dst,
                                   size_t bytes, size_t iterations)
{
	POLVResult err;
	double start, elapsed;
	size_t i;
	
	/* Warmup */
	for (i = 0; i < WARMUP_ITERATIONS; ++i)
	{
		if ((err = polvMemcpy(device_src, 0, device_dst, 0, bytes,
		                      polvDeviceToDevice)) != POLV_SUCCESS)
			return err;
	}

	/* Benchmark */
	start = now_seconds();

	for (i = 0; i < iterations; ++i)
	{
		if ((err = polvMemcpy(device_src, 0, device_dst, 0, bytes,
		                      polvDeviceToDevice)) != POLV_SUCCESS)
			return err;
	}

	elapsed = now_seconds() - start;
	print_result("D2D", bytes, iterations, elapsed);

	return POLV_SUCCESS;
}


static POLVResult verify(const void *host_src, void *host_dst,
                         void *device_dst, size_t bytes, int *valid)
{
	POLVResult err;
	
	if ((err = polvMemcpy(device_dst, 0, host_dst, 0, bytes, polvDeviceToHost)) 
		!= POLV_SUCCESS)
		return err;

	*valid = memcmp(host_src, host_dst, bytes) == 0;

	return POLV_SUCCESS;
}


int main(int argc, char **argv)
{
	size_t size_mib = DEFAULT_SIZE_MIB,
	       iterations = DEFAULT_ITERATIONS,
	       bytes;
	unsigned char *host_src = NULL, *host_dst = NULL;
	void *device_src = NULL, *device_dst = NULL;
	POLVDeviceInfo info;
	int valid;
	size_t i;

	if (argc > 3 ||
	    (argc >= 2 && !parse_positive_size(argv[1], &size_mib)) ||
	    (argc == 3 && !parse_positive_size(argv[2], &iterations)))
	{
		fprintf(stderr, "usage: %s [size_mib] [iterations]\n", argv[0]);
		return 1;
	}

	if (size_mib > SIZE_MAX / (1024u * 1024u))
	{
		fprintf(stderr, "transfer size is too large\n");
		return 1;
	}

	bytes = size_mib * 1024u * 1024u;

	/* Host memory allocation/initialization */
	host_src = (unsigned char *) smalloc(bytes);
	host_dst = (unsigned char *) smalloc(bytes);

	for (i = 0; i < bytes; ++i)
		host_src[i] = (unsigned char) (i * 131u + 17u);

	memset(host_dst, 0, bytes);

	/* POLV initialization */
	TEST_POLV_CHECK(polvInit(),	"polvInit failed");
	TEST_POLV_CHECK(polvSetDevice(DEVICE_ID), "polvSetDevice failed");

	/* Get device information */
	TEST_POLV_CHECK(polvGetDeviceInfo(DEVICE_ID, &info), "polvGetDeviceInfo(0) failed");

	/* Device memory allocation */
	device_src = polvAlloc(bytes, polvMemDeviceLocal);
	device_dst = polvAlloc(bytes, polvMemDeviceLocal);
	TEST_CHECK(device_src && device_dst, 
		"device allocation failed; try a smaller transfer size");

	printf("Device: %s\n", info.name);
	printf("Transfer: %zu MiB, %zu timed iterations, %u warmup iterations\n",
	       size_mib, iterations, WARMUP_ITERATIONS);
	printf("Bandwidth is end-to-end POLV copy throughput (decimal GB/s).\n\n");

	/* Host to device */
	TEST_POLV_CHECK(host_to_device(host_src, device_src, bytes, iterations),
		"H2D failed");

	/* Device to host */
	TEST_POLV_CHECK(device_to_host(device_src, host_dst, bytes, iterations),
		"D2H failed");

	/* Device to device */
	TEST_POLV_CHECK(device_to_device(device_src, device_dst, bytes, iterations),
		"D2D failed");

	/* Verification */
	TEST_POLV_CHECK(verify(host_src, host_dst, device_dst, bytes, &valid),
		"verification D2H failed");

	TEST_CHECK(valid, "verification failed after D2D copy");

	printf("\nVerification: OK\n");

	TEST_SUCCESS(); // cleanup and return
}