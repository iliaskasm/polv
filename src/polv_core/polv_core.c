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
 * POLV Core API
 */
#include <stdlib.h>
#include <string.h>

#include "polv_core_internal.h"
#include "polv_core.h"
#include "runtime.h"
#include "contexts.h"
#include "memory.h"
#include "kernels.h"

/**************************************************************
 *                                                            *
 * INIT/FINALIZE                                              *
 *                                                            *
 **************************************************************/

POLVCoreResult polvCoreInit(void)
{
	POLVCoreRuntimeState *runtime;
	VkApplicationInfo appinfo;
	VkInstanceCreateInfo instance_ci;
	VkPhysicalDevice *raw = NULL;
	POLVCoreDevice *devices = NULL;
	uint32_t count = 0;
	uint32_t i;
	uint32_t queue_family;
	POLVCoreDevice *dev;
	int created = 0;
	POLVCoreResult result = POLV_CORE_ERROR_VULKAN;
	int j;

	runtime = polvc_runtime_state();

	if (runtime->initialized)
		return POLV_CORE_SUCCESS;

	memset(&appinfo, 0, sizeof(appinfo));
	appinfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appinfo.pApplicationName = NULL;
	appinfo.applicationVersion = 0;
	appinfo.pEngineName = "POLV";
	appinfo.engineVersion = VK_MAKE_API_VERSION(
		0, POLV_VERSION_MAJOR, POLV_VERSION_MINOR, POLV_VERSION_PATCH
	);
	appinfo.apiVersion = VK_API_VERSION_1_0;

	memset(&instance_ci, 0, sizeof(instance_ci));
	instance_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_ci.pApplicationInfo = &appinfo;

	if (vkCreateInstance(&instance_ci, NULL, &runtime->instance) != VK_SUCCESS)
		goto FAIL;

	if (vkEnumeratePhysicalDevices(runtime->instance, &count, NULL) != VK_SUCCESS)
		goto FAIL;
		
	if (count == 0)
	{
		result = POLV_CORE_ERROR_NO_DEVICE;
		goto FAIL;
	}

	raw = (VkPhysicalDevice *) malloc((size_t) count * sizeof(*raw));
	devices = (POLVCoreDevice *) calloc((size_t) count, sizeof(*devices));
	if (!raw || !devices)
	{
		result = POLV_CORE_ERROR_OUT_OF_MEMORY;
		goto FAIL;
	}

	if (vkEnumeratePhysicalDevices(runtime->instance, &count, raw) != VK_SUCCESS)
		goto FAIL;

	for (i = 0; i < count; ++i)
	{
		queue_family = polvc_runtime_find_compute_queue_family(raw[i]);
		if (queue_family == UINT32_MAX)
			continue;

		dev = &devices[created];
		dev->id = created;
		dev->global_id = (int) i;
		dev->pdev.physical_device = raw[i];
		dev->pdev.queue_family = queue_family;
		vkGetPhysicalDeviceProperties(raw[i], &dev->pdev.properties);
		vkGetPhysicalDeviceFeatures(raw[i], &dev->pdev.features);
		vkGetPhysicalDeviceMemoryProperties(raw[i], &dev->pdev.memory_properties);

		if (polvc_runtime_create_logical_device(dev) != POLV_CORE_SUCCESS)
		{
			memset(dev, 0, sizeof(*dev));
			continue;
		}
		++created;
	}

	if (created == 0)
	{
		result = POLV_CORE_ERROR_NO_DEVICE;
		goto FAIL;
	}

	free(raw);
	raw = NULL;
	runtime->vk_devs = devices;
	devices = NULL;
	runtime->available_devs = created;
	runtime->initialized = 1;
	return POLV_CORE_SUCCESS;

FAIL:
	if (devices)
	{
		for (j = 0; j < created; ++j)
			polvc_runtime_finalize_device(&devices[j]);
	}
	free(devices);
	free(raw);
	if (runtime->instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(runtime->instance, NULL);
		runtime->instance = VK_NULL_HANDLE;
	}
	runtime->available_devs = 0;
	runtime->initialized = 0;
	return result;
}


void polvCoreFinalize(void)
{
	POLVCoreRuntimeState *runtime;
	int i;

	runtime = polvc_runtime_state();

	if (runtime->vk_devs)
	{
		for (i = 0; i < runtime->available_devs; ++i)
			polvc_runtime_finalize_device(&runtime->vk_devs[i]);
		free(runtime->vk_devs);
		runtime->vk_devs = NULL;
	}

	if (runtime->instance != VK_NULL_HANDLE)
	{
		vkDestroyInstance(runtime->instance, NULL);
		runtime->instance = VK_NULL_HANDLE;
	}

	runtime->available_devs = 0;
	polvc_contexts_set_current(NULL);
	runtime->initialized = 0;
}


/**************************************************************
 *                                                            *
 * CONTEXTS                                                   *
 *                                                            *
 **************************************************************/

POLVCoreResult polvCoreContextCreate(POLVCoreContext **context, POLVCoreDevice *dev)
{
	POLVCoreContext *ctx;
	POLVCoreResult res;

	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!context || !dev || dev->device == VK_NULL_HANDLE)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	ctx = (POLVCoreContext *) calloc(1, sizeof(*ctx));

	if (!ctx)
		return POLV_CORE_ERROR_OUT_OF_MEMORY;

	ctx->p_device = dev;
	ctx->compute_queue_family = dev->pdev.queue_family;

	vkGetDeviceQueue(dev->device, dev->pdev.queue_family, 0,
	                 &ctx->compute_queue);

	/* No compute queue */
	if (ctx->compute_queue == VK_NULL_HANDLE)
	{
		free(ctx);
		return POLV_CORE_ERROR_VULKAN;
	}

	/* Create command pool */
	if ((res = polvc_contexts_create_command_pool(ctx)) != POLV_CORE_SUCCESS)
		goto FAIL;

	/* Create command buffer */
	if ((res = polvc_contexts_create_command_buffer(ctx)) != POLV_CORE_SUCCESS)
		goto FAIL;

	*context = ctx;

	/* If there is no current context, set it to the one we created;
	 * also add it to the list.
	 */
	if (polvc_contexts_get_current() == NULL)
		polvc_contexts_set_current(ctx);

	polvc_contexts_link(ctx);

	return POLV_CORE_SUCCESS;

FAIL:
	if (ctx->command_pool != VK_NULL_HANDLE)
		vkDestroyCommandPool(dev->device, ctx->command_pool, NULL);

	free(ctx);
	*context = NULL;

	return res;
}


void polvCoreContextDestroy(POLVCoreContext **context)
{
	if (!context || !*context)
		return;

	polvc_contexts_destroy(*context);

	*context = NULL;
}


POLVCoreResult polvCoreContextSetCurrent(POLVCoreContext *context)
{
	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	polvc_contexts_set_current(context);

	return POLV_CORE_SUCCESS;
}


POLVCoreResult polvCoreContextGetCurrent(POLVCoreContext **context)
{
	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!context)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	*context = polvc_contexts_get_current();

	return POLV_CORE_SUCCESS;
}


/**************************************************************
 *                                                            *
 * DEVICE HANDLING                                            *
 *                                                            *
 **************************************************************/

int polvCoreGetNumDevices(void)
{
	POLVCoreRuntimeState *runtime;

	runtime = polvc_runtime_state();
	return runtime->initialized ? runtime->available_devs : 0;
}


POLVCoreDevice *polvCoreGetDevice(int id)
{
	POLVCoreRuntimeState *runtime;

	runtime = polvc_runtime_state();
	if (!runtime->initialized || !runtime->vk_devs ||
	    id < 0 || id >= runtime->available_devs)
		return NULL;
	return &runtime->vk_devs[id];
}


int polvCoreDeviceGetId(POLVCoreDevice *dev)
{
	POLVCoreRuntimeState *runtime;

	runtime = polvc_runtime_state();
	if (!runtime->initialized || !dev)
		return -1;

	return dev->id;
}


POLVCoreResult polvCoreDeviceGetProperties(POLVCoreDevice *dev,
                                           VkPhysicalDeviceProperties *properties)
{
	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!dev || !properties)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	*properties = dev->pdev.properties;
	return POLV_CORE_SUCCESS;
}


POLVCoreResult polvCoreDeviceGetFeatures(POLVCoreDevice *dev,
                                         VkPhysicalDeviceFeatures *features)
{
	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!dev || !features)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	*features = dev->pdev.features;
	return POLV_CORE_SUCCESS;
}


POLVCoreResult polvCoreDeviceGetMemoryProperties(
    POLVCoreDevice *dev, VkPhysicalDeviceMemoryProperties *memory_properties)
{
	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!dev || !memory_properties)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	*memory_properties = dev->pdev.memory_properties;
	return POLV_CORE_SUCCESS;
}


uint64_t polvCoreDeviceGetLocalMemorySize(POLVCoreDevice *dev)
{
	VkPhysicalDeviceMemoryProperties *memory_properties;
	uint64_t total;
	uint32_t i;

	if (!polvc_runtime_state()->initialized || !dev)
		return 0;

	memory_properties = &dev->pdev.memory_properties;
	total = 0;

	for (i = 0; i < memory_properties->memoryHeapCount; ++i)
	{
		if ((memory_properties->memoryHeaps[i].flags &
		     VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
			total += (uint64_t) memory_properties->memoryHeaps[i].size;
	}

	return total;
}


uint32_t polvCoreDeviceGetComputeQueueFamily(POLVCoreDevice *dev)
{
	if (!polvc_runtime_state()->initialized || !dev)
		return UINT32_MAX;

	return dev->pdev.queue_family;
}


/**************************************************************
 *                                                            *
 * MEMORY                                                     *
 *                                                            *
 **************************************************************/

POLVCoreMemory *polvCoreMemoryAllocDeviceLocal(VkDeviceSize size)
{
	POLVCoreContext *context;

	context = polvc_contexts_get_current();
	if (!context)
		return NULL;

	return polvc_memory_allocate(context, size,
	                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
	                             0, 1);
}


POLVCoreMemory *polvCoreMemoryAllocHostVisible(VkDeviceSize size)
{
	POLVCoreContext *context;

	context = polvc_contexts_get_current();
	if (!context)
		return NULL;

	return polvc_memory_allocate(context, size,
	                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	                             VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
	                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                             1);
}


POLVCoreMemory *polvCoreMemoryAllocHostCoherent(VkDeviceSize size)
{
	POLVCoreContext *context;

	context = polvc_contexts_get_current();
	if (!context)
		return NULL;

	return polvc_memory_allocate(context, size,
	                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
	                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                             VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
	                             1);
}


POLVCoreMemory *polvCoreMemoryAlloc(VkDeviceSize size)
{
	return polvCoreMemoryAllocHostCoherent(size);
}


void polvCoreMemoryFree(POLVCoreMemory *addr)
{
	POLVCoreContext *context;
	POLVCoreDevice *dev;

	if (!addr || !addr->owner)
		return;

	context = addr->owner;
	dev = context->p_device;
	if (dev->device != VK_NULL_HANDLE)
		vkDeviceWaitIdle(dev->device);
	polvc_memory_destroy(addr, 1);
}


void *polvCoreMemoryGetHostPointer(POLVCoreMemory *addr)
{
	if (!addr || !addr->owner)
		return NULL;

	if (!(addr->memory_properties &
	      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		return NULL;

	if (!(addr->memory_properties &
	      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		return NULL;

	return addr->host_ptr;
}


POLVCoreResult polvCoreMemoryCopyH2D(const void *src, size_t src_offset,
                                     POLVCoreMemory *dst, size_t dst_offset, size_t size)
{
	const unsigned char *src_addr;
	POLVCoreContext *context;
	POLVCoreMemory *staging;
	POLVCoreResult res;

	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!src || !dst || !dst->owner || size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (dst_offset > dst->size ||
		(VkDeviceSize) size > dst->size - (VkDeviceSize) dst_offset)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	context = dst->owner;
	src_addr = (const unsigned char *) src + src_offset;

	if ((dst->memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
		return polvc_memory_map_write(dst, dst_offset, src_addr, size);

	res = polvc_memory_alloc_staging(context, &context->upload_staging,
	                                 (VkDeviceSize) size,
	                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                                 &staging);
	if (res != POLV_CORE_SUCCESS)
		return res;

	res = polvc_memory_map_write(staging, 0, src_addr, size);
	if (res != POLV_CORE_SUCCESS)
		return res;

	return polvc_memory_submit_buffer_copy(context, staging->buffer, dst->buffer, 0,
	                                       (VkDeviceSize) dst_offset,
	                                       (VkDeviceSize) size);
}


POLVCoreResult polvCoreMemoryCopyD2H(POLVCoreMemory *src, size_t src_offset,
                                     void *dst, size_t dst_offset, size_t size)
{
	unsigned char *dst_addr;
	POLVCoreContext *context;
	POLVCoreMemory *staging;
	POLVCoreResult res;

	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!src || !src->owner || !dst || size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (src_offset > src->size ||
		(VkDeviceSize) size > src->size - (VkDeviceSize) src_offset)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	context = src->owner;
	dst_addr = (unsigned char *) dst + dst_offset;

	if ((src->memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
		return polvc_memory_map_read(src, src_offset, dst_addr, size);

	res = polvc_memory_alloc_staging(context, &context->download_staging,
	                                 (VkDeviceSize) size,
	                                 VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
	                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	                                 &staging);
	if (res != POLV_CORE_SUCCESS)
		return res;

	res = polvc_memory_submit_buffer_copy(context, src->buffer, staging->buffer,
	                                      (VkDeviceSize) src_offset, 0,
	                                      (VkDeviceSize) size);
	if (res != POLV_CORE_SUCCESS)
		return res;

	return polvc_memory_map_read(staging, 0, dst_addr, size);
}


POLVCoreResult polvCoreMemoryCopyD2D(POLVCoreMemory *src, size_t src_offset,
                                     POLVCoreMemory *dst, size_t dst_offset, size_t size)
{
	POLVCoreContext *context;
	VkDeviceSize src_end;
	VkDeviceSize dst_end;

	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!src || !dst || !src->owner || !dst->owner || size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (src->owner != dst->owner)
		return POLV_CORE_ERROR_CONTEXT_MISMATCH;
		
	context = src->owner;
	if (src_offset > src->size ||
		(VkDeviceSize) size > src->size - (VkDeviceSize) src_offset)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (dst_offset > dst->size ||
		(VkDeviceSize) size > dst->size - (VkDeviceSize) dst_offset)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (src == dst)
	{
		src_end = (VkDeviceSize) src_offset + (VkDeviceSize) size;
		dst_end = (VkDeviceSize) dst_offset + (VkDeviceSize) size;

		if ((VkDeviceSize) src_offset < dst_end &&
			(VkDeviceSize) dst_offset < src_end)
			return POLV_CORE_ERROR_INVALID_ARGUMENT;
	}

	return polvc_memory_submit_buffer_copy(context, src->buffer, dst->buffer,
	                                          (VkDeviceSize) src_offset, (VkDeviceSize) dst_offset,
	                                          (VkDeviceSize) size);
}


POLVCoreResult polvCoreMemoryCopyH2H(const void *src, size_t src_offset, void *dst,
                                     size_t dst_offset, size_t size)
{
	const unsigned char *src_addr;
	unsigned char *dst_addr;

	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!src || !dst || size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	src_addr = (const unsigned char *) src + src_offset;
	dst_addr = (unsigned char *) dst + dst_offset;

	memmove(dst_addr, src_addr, size);

	return POLV_CORE_SUCCESS;
}


/**************************************************************
 *                                                            *
 * KERNELS                                                    *
 *                                                            *
 **************************************************************/

POLVCoreResult polvCoreKernelCreate(POLVCoreKernel **kernel,
                                    const char *shader_filename, int nargs)
{
	POLVCoreContext *context;
	POLVCoreDevice *dev;
	POLVCoreKernel *k;
	POLVCoreResult res;
	int shader_id;

	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!kernel || !shader_filename || nargs <= 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	*kernel = NULL;

	context = polvc_contexts_get_current();
	if (!context)
		return POLV_CORE_ERROR_CONTEXT_NOT_INITIALIZED;

	dev = context->p_device;
	if (!dev)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	shader_id = polvc_kernels_shader_new(dev, shader_filename);
	if (shader_id < 0)
		return (POLVCoreResult) shader_id;

	if (shader_id >= dev->nshaders)
		return POLV_CORE_ERROR_SHADER;

	k = (POLVCoreKernel *) calloc(1, sizeof(*k));
	if (!k)
		return POLV_CORE_ERROR_OUT_OF_MEMORY;

	k->owner = context;
	k->shader = &dev->shader_cache[shader_id];
	k->nargs = nargs;

	if ((res = polvc_kernels_create_descriptor_set_layout(k)) != POLV_CORE_SUCCESS)
		goto FAIL;

	if ((res = polvc_kernels_create_pipeline_layout(k)) != POLV_CORE_SUCCESS)
		goto FAIL;

	if ((res = polvc_kernels_create_descriptor_pool(k)) != POLV_CORE_SUCCESS)
		goto FAIL;

	if ((res = polvc_kernels_allocate_descriptor_set(k)) != POLV_CORE_SUCCESS)
		goto FAIL;

	polvc_kernels_link(context, k);
	*kernel = k;

	return POLV_CORE_SUCCESS;

FAIL:
	polvc_kernels_destroy(k, 0);
	return res;
}


void polvCoreKernelDestroy(POLVCoreKernel **kernel)
{
	POLVCoreDevice *dev;

	if (!kernel || !*kernel)
		return;

	dev = (*kernel)->owner ? (*kernel)->owner->p_device : NULL;
	if (dev && dev->device != VK_NULL_HANDLE)
		vkDeviceWaitIdle(dev->device);

	polvc_kernels_destroy(*kernel, 1);
	*kernel = NULL;
}


POLVCoreResult polvCoreKernelLaunch(POLVCoreKernel *kernel, void **args,
                                    uint32_t grid_x, uint32_t grid_y, uint32_t grid_z,
                                    uint32_t group_x, uint32_t group_y, uint32_t group_z)
{
	VkDescriptorBufferInfo *infos;
	VkWriteDescriptorSet *writes;
	VkCommandBufferBeginInfo bi;
	VkSubmitInfo submit;
	VkPipeline compute_pipeline;
	POLVCoreContext *context;
	POLVCoreDevice *dev;
	POLVCoreMemory *p;
	uint64_t group_invocations;
	POLVCoreResult res;
	int i;
	VkResult vr;
	VkMemoryBarrier barrier;

	infos = NULL;
	writes = NULL;
	compute_pipeline = VK_NULL_HANDLE;

	if (!polvc_runtime_state()->initialized)
		return POLV_CORE_ERROR_NOT_INITIALIZED;

	if (!kernel || !kernel->owner || !args ||
		grid_x == 0 || grid_y == 0 || grid_z == 0 ||
		group_x == 0 || group_y == 0 || group_z == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	context = kernel->owner;
	dev = context->p_device;
	if (!dev)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (grid_x > dev->pdev.properties.limits.maxComputeWorkGroupCount[0] ||
		grid_y > dev->pdev.properties.limits.maxComputeWorkGroupCount[1] ||
		grid_z > dev->pdev.properties.limits.maxComputeWorkGroupCount[2])
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (group_x > dev->pdev.properties.limits.maxComputeWorkGroupSize[0] ||
		group_y > dev->pdev.properties.limits.maxComputeWorkGroupSize[1] ||
		group_z > dev->pdev.properties.limits.maxComputeWorkGroupSize[2])
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	group_invocations = (uint64_t) group_x * (uint64_t) group_y * (uint64_t) group_z;

	if (group_invocations >
		(uint64_t) dev->pdev.properties.limits.maxComputeWorkGroupInvocations)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	for (i = 0; i < kernel->nargs; ++i)
	{
		p = (POLVCoreMemory *) args[i];

		if (!p || p->buffer == VK_NULL_HANDLE)
			return POLV_CORE_ERROR_INVALID_ARGUMENT;

		if (p->owner != context)
			return POLV_CORE_ERROR_CONTEXT_MISMATCH;
	}

	res = polvc_kernels_get_pipeline(kernel, group_x, group_y, group_z,
	                                 &compute_pipeline);
	if (res != POLV_CORE_SUCCESS)
		return res;

	infos = (VkDescriptorBufferInfo *) calloc((size_t) kernel->nargs, sizeof(*infos));
	writes = (VkWriteDescriptorSet *) calloc((size_t) kernel->nargs, sizeof(*writes));
	if (!infos || !writes)
	{
		res = POLV_CORE_ERROR_OUT_OF_MEMORY;
		goto FAIL;
	}

	for (i = 0; i < kernel->nargs; ++i)
	{
		p = (POLVCoreMemory *) args[i];

		infos[i].buffer = p->buffer;
		infos[i].offset = 0;
		infos[i].range = p->size;

		writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[i].dstSet = kernel->descriptor_set;
		writes[i].dstBinding = (uint32_t) i;
		writes[i].descriptorCount = 1;
		writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writes[i].pBufferInfo = &infos[i];
	}

	vkUpdateDescriptorSets(dev->device, (uint32_t) kernel->nargs, writes, 0, NULL);

	if ((vr = vkResetCommandBuffer(context->command_buffer, 0)) != VK_SUCCESS)
	{
		res = POLV_CORE_ERROR_VULKAN;
		goto FAIL;
	}

	memset(&bi, 0, sizeof(bi));
	bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if ((vr = vkBeginCommandBuffer(context->command_buffer, &bi)) != VK_SUCCESS)
	{
		res = POLV_CORE_ERROR_VULKAN;
		goto FAIL;
	}

	vkCmdBindPipeline(context->command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
	                  compute_pipeline);

	vkCmdBindDescriptorSets(context->command_buffer,
	                        VK_PIPELINE_BIND_POINT_COMPUTE,
	                        kernel->pipeline_layout, 0, 1,
	                        &kernel->descriptor_set, 0, NULL);

	vkCmdDispatch(context->command_buffer, grid_x, grid_y, grid_z);


	/* Barrier */
	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
	vkCmdPipelineBarrier(
		context->command_buffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_HOST_BIT,
		0, 1, &barrier, 0, NULL, 0, NULL
	);

	if ((vr = vkEndCommandBuffer(context->command_buffer)) != VK_SUCCESS)
	{
		res = POLV_CORE_ERROR_VULKAN;
		goto FAIL;
	}

	memset(&submit, 0, sizeof(submit));
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &context->command_buffer;

	if ((vr = vkQueueSubmit(context->compute_queue, 1, &submit, VK_NULL_HANDLE)) 
		!= VK_SUCCESS)
	{
		res = POLV_CORE_ERROR_VULKAN;
		goto FAIL;
	}

	if ((vr = vkQueueWaitIdle(context->compute_queue)) != VK_SUCCESS) 
	{
		res = POLV_CORE_ERROR_VULKAN;
		goto FAIL;
	}

	++dev->num_launched_kernels;
	res = POLV_CORE_SUCCESS;

FAIL:
	free(infos);
	free(writes);
	return res;
}


/**************************************************************
 *                                                            *
 * MISCELLANEOUS                                              *
 *                                                            *
 **************************************************************/

const char *polvCoreStatus(POLVCoreResult status)
{
	switch (status)
	{
		case POLV_CORE_SUCCESS: 
			return "success";
		case POLV_CORE_ERROR: 
			return "generic error";
		case POLV_CORE_ERROR_NOT_INITIALIZED: 
			return "POLV Core is not initialized";
		case POLV_CORE_ERROR_INVALID_ARGUMENT: 
			return "invalid argument";
		case POLV_CORE_ERROR_NO_DEVICE: 
			return "no suitable Vulkan compute device";
		case POLV_CORE_ERROR_VULKAN: 
			return "Vulkan API error";
		case POLV_CORE_ERROR_OUT_OF_MEMORY: 
			return "out of memory";
		case POLV_CORE_ERROR_SHADER: 
			return "shader error";
		case POLV_CORE_ERROR_UNSUPPORTED: 
			return "unsupported operation";
		case POLV_CORE_ERROR_DEVICE_ID_OUT_OF_BOUNDS: 
			return "device ID out of bounds";
		case POLV_CORE_ERROR_CONTEXT_MISMATCH: 
			return "object belongs to another context";
		case POLV_CORE_ERROR_CONTEXT_NOT_INITIALIZED: 
			return "context not initialized";
		default: 
			return "unknown POLV Core error";
	}
}
