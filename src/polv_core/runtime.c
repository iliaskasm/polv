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

/*
 * POLV Core runtime helpers (internal)
 */
#include "runtime.h"
#include "contexts.h"
#include "kernels.h"
#include "polv_core_internal.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static POLVCoreRuntimeState runtime_state = { 0, VK_NULL_HANDLE, NULL, 0 };

/* Returns the runtime state. */
POLVCoreRuntimeState *polvc_runtime_state(void)
{
	return &runtime_state;
}


/* Finds and returns a compute queue family. */
uint32_t polvc_runtime_find_compute_queue_family(VkPhysicalDevice physical)
{
	uint32_t count = 0;
	VkQueueFamilyProperties *families;
	uint32_t result = UINT32_MAX;
	uint32_t i;

	vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, NULL);
	if (count == 0)
		return UINT32_MAX;

	families = (VkQueueFamilyProperties *) malloc((size_t) count * sizeof(*families));
	if (!families)
		return UINT32_MAX;

	vkGetPhysicalDeviceQueueFamilyProperties(physical, &count, families);
	for (i = 0; i < count; ++i)
	{
		if ((families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
		{
			result = i;
			break;
		}
	}

	free(families);
	return result;
}


/* Creates a logical device for dev. */
POLVCoreResult polvc_runtime_create_logical_device(POLVCoreDevice *dev)
{
	float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_ci;
	VkPhysicalDeviceFeatures features;
	VkDeviceCreateInfo device_ci;

	if (!dev || dev->pdev.queue_family == UINT32_MAX)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	memset(&queue_ci, 0, sizeof(queue_ci));
	queue_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_ci.queueFamilyIndex = dev->pdev.queue_family;
	queue_ci.queueCount = 1;
	queue_ci.pQueuePriorities = &queue_priority;

	memset(&features, 0, sizeof(features));

	memset(&device_ci, 0, sizeof(device_ci));
	device_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_ci.queueCreateInfoCount = 1;
	device_ci.pQueueCreateInfos = &queue_ci;
	device_ci.pEnabledFeatures = &features;

	if (vkCreateDevice(dev->pdev.physical_device, &device_ci, NULL, &dev->device) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}


/* Finalizes a device. */
void polvc_runtime_finalize_device(POLVCoreDevice *dev)
{
	POLVCoreContext *context;
	POLVCoreContext *next;
	int i;

	if (!dev)
		return;

	if (dev->device != VK_NULL_HANDLE)
		vkDeviceWaitIdle(dev->device);

	context = dev->contexts;

	while (context)
	{
		next = context->next;
		polvc_contexts_destroy(context);
		context = next;
	}

	dev->contexts = NULL;

	for (i = 0; i < dev->nshaders; ++i)
		polvc_kernels_shader_destroy(dev, i);

	dev->nshaders = 0;

	if (dev->device != VK_NULL_HANDLE)
	{
		vkDestroyDevice(dev->device, NULL);
		dev->device = VK_NULL_HANDLE;
	}
}
