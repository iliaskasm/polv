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
#include <inttypes.h>
#include <stdio.h>
#include <polv.h>
#include "../common.h"

#define TEST_CLEANUP() do { \
	polvFinalize(); \
} while (0)

int main(void)
{
	int ndev, i;

	/* Initialize POLV and get number of devices */
	TEST_POLV_CHECK(polvInit(), "polvInit failed");

	ndev = polvGetNumDevices();
	printf("POLV devices: %d\n", ndev);

	for (i = 0; i < ndev; ++i)
	{
		POLVDeviceInfo info;

		TEST_POLV_CHECK(polvGetDeviceInfo(i, &info),
			"polvGetDeviceInfo failed");

		printf("\nDevice %d\n", info.id);
		printf("  name: %s\n", info.name);
		printf("  type: %s\n", polvDeviceTypeName(info.type));
		printf("  vendor/device ID: 0x%04" PRIx32 "/0x%04" PRIx32 "\n",
		       info.vendor_id, info.device_id);
		printf("  Vulkan API: %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
		       info.api_version_major,
		       info.api_version_minor,
		       info.api_version_patch);
		printf("  driver version (raw): %" PRIu32 "\n", info.driver_version);
		printf("  device-local memory: %.2f GiB\n",
		       (double) info.device_local_memory_bytes / (1024.0 * 1024.0 * 1024.0));
		printf("  compute queue family: %" PRIu32 "\n", info.compute_queue_family);
		printf("  max work-group count: %" PRIu32 " x %" PRIu32 " x %" PRIu32 "\n",
		       info.max_compute_work_group_count[0],
		       info.max_compute_work_group_count[1],
		       info.max_compute_work_group_count[2]);
		printf("  max work-group size: %" PRIu32 " x %" PRIu32 " x %" PRIu32 "\n",
		       info.max_compute_work_group_size[0],
		       info.max_compute_work_group_size[1],
		       info.max_compute_work_group_size[2]);
		printf("  max invocations/group: %" PRIu32 "\n",
		       info.max_compute_work_group_invocations);
		printf("  max storage-buffer range: %" PRIu32 " bytes\n",
		       info.max_storage_buffer_range);
	}

	TEST_SUCCESS(); // cleanup and return
}
