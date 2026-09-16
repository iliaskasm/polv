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
#include <polv_core.h>
#include "../common.h"

#define DEVICE_ID 0
#define DEFAULT_SIZE_MIB 64u
#define DEFAULT_ITERATIONS 20u
#define WARMUP_ITERATIONS 20u

#define TEST_CLEANUP() do { \
	polvCoreMemoryFree(device_src); \
	polvCoreMemoryFree(device_dst); \
	free(host_src); \
	free(host_dst); \
	polvCoreContextDestroy(&context); \
	polvCoreFinalize(); \
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
static POLVCoreResult host_to_device(const void *host_src,
                                     POLVCoreMemory *device_src,
                                     size_t bytes, size_t iterations)
{
	POLVCoreResult err;
	double start, elapsed;
	size_t i;

	/* Warmup */
	for (i = 0; i < WARMUP_ITERATIONS; ++i)
	{
		if ((err = polvCoreMemoryCopyH2D(host_src, 0, device_src, 0, bytes)) 
			!= POLV_CORE_SUCCESS)
			return err;
	}

	/* Benchmark */
	start = now_seconds();

	for (i = 0; i < iterations; ++i)
	{
		if ((err = polvCoreMemoryCopyH2D(host_src, 0, device_src, 0, bytes)) 
			!= POLV_CORE_SUCCESS)
			return err;
	}

	elapsed = now_seconds() - start;
	print_result("H2D", bytes, iterations, elapsed);

	return POLV_CORE_SUCCESS;
}


/******************************************************************
 *                         Device to host                         *
 ******************************************************************/
static POLVCoreResult device_to_host(POLVCoreMemory *device_src,
                                     void *host_dst,
                                     size_t bytes, size_t iterations)
{
	POLVCoreResult err;
	double start, elapsed;
	size_t i;

	/* Warmup */
	for (i = 0; i < WARMUP_ITERATIONS; ++i)
	{
		if ((err = polvCoreMemoryCopyD2H(device_src, 0, host_dst, 0, bytes)) 
			!= POLV_CORE_SUCCESS)
			return err;
	}

	/* Benchmark */
	start = now_seconds();

	for (i = 0; i < iterations; ++i)
	{
		if ((err = polvCoreMemoryCopyD2H(device_src, 0, host_dst, 0, bytes)) 
			!= POLV_CORE_SUCCESS)
			return err;
	}

	elapsed = now_seconds() - start;
	print_result("D2H", bytes, iterations, elapsed);

	return POLV_CORE_SUCCESS;
}


/******************************************************************
 *                        Device to device                        *
 ******************************************************************/
static POLVCoreResult device_to_device(POLVCoreMemory *device_src,
                                       POLVCoreMemory *device_dst,
                                       size_t bytes, size_t iterations)
{
	POLVCoreResult err;
	double start, elapsed;
	size_t i;

	/* Warmup */
	for (i = 0; i < WARMUP_ITERATIONS; ++i)
	{
		if ((err = polvCoreMemoryCopyD2D(device_src, 0, device_dst, 0, bytes)) 
			!= POLV_CORE_SUCCESS)
			return err;
	}

	/* Benchmark */
	start = now_seconds();

	for (i = 0; i < iterations; ++i)
	{
		if ((err = polvCoreMemoryCopyD2D(device_src, 0, device_dst, 0, bytes)) 
			!= POLV_CORE_SUCCESS)
			return err;
	}

	elapsed = now_seconds() - start;
	print_result("D2D", bytes, iterations, elapsed);

	return POLV_CORE_SUCCESS;
}


/************************************ 
 *            Verification          * 
 ************************************/
static POLVCoreResult verify(const void *host_src, void *host_dst,
                             POLVCoreMemory *device_dst, size_t bytes,
                             int *valid)
{
	POLVCoreResult err;

	if ((err = polvCoreMemoryCopyD2H(device_dst, 0, host_dst, 0, bytes)) 
		!= POLV_CORE_SUCCESS)
		return err;

	*valid = memcmp(host_src, host_dst, bytes) == 0;

	return POLV_CORE_SUCCESS;
}


int main(int argc, char **argv)
{
	size_t size_mib = DEFAULT_SIZE_MIB,
	       iterations = DEFAULT_ITERATIONS,
	       bytes;
	unsigned char *host_src = NULL, *host_dst = NULL;
	POLVCoreDevice *device = NULL;
	POLVCoreContext *context = NULL;
	POLVCoreMemory *device_src = NULL, *device_dst = NULL;
	VkPhysicalDeviceProperties properties;
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
		host_src[i] = (unsigned char)(i * 131u + 17u);

	memset(host_dst, 0, bytes);

	/* POLV Core initialization */
	TEST_POLVC_CHECK(polvCoreInit(), "polvCoreInit failed");

	/* Get POLV Core device */
	device = polvCoreGetDevice(DEVICE_ID);
	TEST_CHECK(device, "no device %d", DEVICE_ID);

	/* Get device information */
	TEST_POLVC_CHECK(polvCoreDeviceGetProperties(device, &properties),
		"polvCoreDeviceGetProperties failed");

	/* Create context */
	TEST_POLVC_CHECK(polvCoreContextCreate(&context, device),
		"polvCoreContextCreate failed");

	/* Device allocations */
	device_src = polvCoreMemoryAllocDeviceLocal((VkDeviceSize) bytes);
	device_dst = polvCoreMemoryAllocDeviceLocal((VkDeviceSize) bytes);
	TEST_CHECK(device_src && device_dst,
		"device allocation failed; try a smaller transfer size");

	printf("Device: %s\n", properties.deviceName);
	printf("Transfer: %zu MiB, %zu timed iterations, %u warmup iterations\n",
	       size_mib, iterations, WARMUP_ITERATIONS);
	printf("Bandwidth is end-to-end POLV Core copy throughput "
	       "(decimal GB/s).\n\n");

	/* Host to device */
	TEST_POLVC_CHECK(host_to_device(host_src, device_src, bytes, iterations),
		"H2D failed");

	/* Device to host */
	TEST_POLVC_CHECK(device_to_host(device_src, host_dst, bytes, iterations),
		"D2H failed");

	/* Device to device */
	TEST_POLVC_CHECK(device_to_device(device_src, device_dst, bytes, iterations),
		"D2D failed");

	/* Verification */
	TEST_POLVC_CHECK(verify(host_src, host_dst, device_dst, bytes, &valid),
		"verification D2H failed");

	TEST_CHECK(valid, "verification failed after D2D copy");

	printf("\nVerification: OK\n");

	TEST_SUCCESS(); // cleanup and return
}