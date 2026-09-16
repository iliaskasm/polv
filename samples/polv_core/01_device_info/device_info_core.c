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
#include <polv_core.h>
#include "../common.h"

#define TEST_CLEANUP() do { \
	polvCoreFinalize(); \
} while(0)

static const char *device_type_name(VkPhysicalDeviceType type)
{
	switch (type)
	{
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: 
			return "integrated GPU";
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: 
			return "discrete GPU";
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: 
			return "virtual GPU";
		case VK_PHYSICAL_DEVICE_TYPE_CPU: 
			return "CPU";
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		default: 
			return "other";
	}
}

int main(void)
{
	int ndev, i;

	/* POLV Core initialization */
	TEST_POLVC_CHECK(polvCoreInit(), "polvCoreInit failed");

	ndev = polvCoreGetNumDevices();
	printf("POLV Core devices: %d\n", ndev);

	for (i = 0; i < ndev; ++i)
	{
		POLVCoreDevice *dev;
		VkPhysicalDeviceProperties properties;
		VkPhysicalDeviceFeatures features;
		VkPhysicalDeviceMemoryProperties memory_properties;

		TEST_CHECK((dev = polvCoreGetDevice(i)), "polvCoreGetDevice(%d) failed", i);

		TEST_POLVC_CHECK(polvCoreDeviceGetProperties(dev, &properties), 
			"polvCoreDeviceGetProperties failed");
		TEST_POLVC_CHECK(polvCoreDeviceGetFeatures(dev, &features), 
			"polvCoreDeviceGetFeatures failed");
		TEST_POLVC_CHECK(polvCoreDeviceGetMemoryProperties(dev, &memory_properties), 
			"polvCoreDeviceGetMemoryProperties failed");

		printf("\nDevice %d\n", polvCoreDeviceGetId(dev));
		printf("  name: %s\n", properties.deviceName);
		printf("  type: %s\n", device_type_name(properties.deviceType));
		printf("  vendor/device ID: 0x%04" PRIx32 "/0x%04" PRIx32 "\n",
		       properties.vendorID, properties.deviceID);
		printf("  Vulkan API: %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
		       VK_VERSION_MAJOR(properties.apiVersion),
		       VK_VERSION_MINOR(properties.apiVersion),
		       VK_VERSION_PATCH(properties.apiVersion));
		printf("  driver version (raw): %" PRIu32 "\n", properties.driverVersion);
		printf("  device-local memory: %.2f GiB\n",
		       (double) polvCoreDeviceGetLocalMemorySize(dev) /
		       (1024.0 * 1024.0 * 1024.0));
		printf("  memory heaps/types: %" PRIu32 "/%" PRIu32 "\n",
		       memory_properties.memoryHeapCount,
		       memory_properties.memoryTypeCount);
		printf("  compute queue family: %" PRIu32 "\n",
		       polvCoreDeviceGetComputeQueueFamily(dev));
		printf("  max invocations/group: %" PRIu32 "\n",
		       properties.limits.maxComputeWorkGroupInvocations);
		printf("  shaderFloat64: %s\n", features.shaderFloat64 ? "yes" : "no");
	}

	TEST_SUCCESS(); // cleanup and return
}
