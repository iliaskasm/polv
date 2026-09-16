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
 * POLV API
 */
#include <string.h>
#include "polv.h"
#include "polv_core.h"
#include "contexts.h"
#include "kernels.h"
#include "polv_internal.h"

static int initialized = 0;


/**************************************************************
 *                                                            *
 * INIT/FINALIZE                                              *
 *                                                            *
 **************************************************************/

POLVResult polvInit(void)
{
	POLVCoreResult core_res;
	POLVResult res;

	if (initialized)
		return POLV_SUCCESS;

	if ((core_res = polvCoreInit()) != POLV_CORE_SUCCESS)
		return polv_status_from_core(core_res);

	if ((res = polv_contexts_init()) != POLV_SUCCESS)
	{
		polv_contexts_finalize();
		polvCoreFinalize();
		return res;
	}

	polv_kernels_init();
	initialized = 1;

	return POLV_SUCCESS;
}


void polvFinalize(void)
{
	if (!initialized)
		return;

	polv_kernels_finalize();
	polv_contexts_finalize();
	polvCoreFinalize();

	initialized = 0;
}


/**************************************************************
 *                                                            *
 * DEVICE HANDLING                                            *
 *                                                            *
 **************************************************************/

int polvGetNumDevices(void)
{
	if (!initialized)
		return POLV_ERROR_NOT_INITIALIZED;

	return polvCoreGetNumDevices();
}


POLVResult polvSetDevice(int device_id)
{
	if (!initialized)
		return POLV_ERROR_NOT_INITIALIZED;

	return polv_contexts_set_device(device_id);
}


int polvGetCurrentDeviceId(void)
{
	if (!initialized)
		return POLV_ERROR_NOT_INITIALIZED;

	return polv_contexts_get_current_device_id();
}


POLVResult polvGetDeviceInfo(int device_id, POLVDeviceInfo *info)
{
	size_t len;
	POLVCoreDevice *dev;
	VkPhysicalDeviceProperties properties;
	POLVCoreResult core_res;

	if (!initialized)
		return POLV_ERROR_NOT_INITIALIZED;

	if (!info)
		return POLV_ERROR_INVALID_ARGUMENT;

	dev = polvCoreGetDevice(device_id);
	if (!dev)
		return POLV_ERROR_DEVICE_ID_OUT_OF_BOUNDS;

	if ((core_res = polvCoreDeviceGetProperties(dev, &properties)) != POLV_CORE_SUCCESS)
		return polv_status_from_core(core_res);

	memset(info, 0, sizeof(*info));
	info->id = device_id;
	len = strnlen(properties.deviceName, POLV_DEVICE_NAME_SIZE - 1);
	memcpy(info->name, properties.deviceName, len);
	info->name[len] = '\0';
	info->vendor_id = properties.vendorID;
	info->device_id = properties.deviceID;
	info->api_version = properties.apiVersion;
	info->api_version_major = VK_VERSION_MAJOR(properties.apiVersion);
	info->api_version_minor = VK_VERSION_MINOR(properties.apiVersion);
	info->api_version_patch = VK_VERSION_PATCH(properties.apiVersion);
	info->driver_version = properties.driverVersion;
	info->device_local_memory_bytes = polvCoreDeviceGetLocalMemorySize(dev);
	info->compute_queue_family = polvCoreDeviceGetComputeQueueFamily(dev);
	info->max_compute_work_group_count[0] = properties.limits.maxComputeWorkGroupCount[0];
	info->max_compute_work_group_count[1] = properties.limits.maxComputeWorkGroupCount[1];
	info->max_compute_work_group_count[2] = properties.limits.maxComputeWorkGroupCount[2];
	info->max_compute_work_group_size[0] = properties.limits.maxComputeWorkGroupSize[0];
	info->max_compute_work_group_size[1] = properties.limits.maxComputeWorkGroupSize[1];
	info->max_compute_work_group_size[2] = properties.limits.maxComputeWorkGroupSize[2];
	info->max_compute_work_group_invocations = properties.limits.maxComputeWorkGroupInvocations;
	info->max_storage_buffer_range = properties.limits.maxStorageBufferRange;

	switch (properties.deviceType)
	{
		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			info->type = polvDeviceIntegratedGPU;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			info->type = polvDeviceDiscreteGPU;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			info->type = polvDeviceVirtualGPU;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			info->type = polvDeviceCPU;
			break;
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
		default:
			info->type = polvDeviceOther;
			break;
	}

	return POLV_SUCCESS;
}


const char *polvDeviceTypeName(POLVDeviceType type)
{
	switch (type)
	{
		case polvDeviceIntegratedGPU:
			return "integrated GPU";
		case polvDeviceDiscreteGPU:
			return "discrete GPU";
		case polvDeviceVirtualGPU:
			return "virtual GPU";
		case polvDeviceCPU:
			return "CPU";
		case polvDeviceOther:
		default:
			return "other";
	}
}


/**************************************************************
 *                                                            *
 * MEMORY                                                     *
 *                                                            *
 **************************************************************/

void *polvAlloc(size_t size, POLVMemoryType map_type)
{
	POLVCoreContext *context;
	POLVCoreResult res;

	if (!initialized || size == 0)
		return NULL;

	if ((context = polv_contexts_get_current_context()) == NULL)
		return NULL;

	if ((res = polvCoreContextSetCurrent(context)) != POLV_CORE_SUCCESS)
		return NULL;

	switch (map_type)
	{
		case polvMemDeviceLocal:
			return polvCoreMemoryAllocDeviceLocal((VkDeviceSize) size);
		case polvMemHostVisible:
			return polvCoreMemoryAllocHostVisible((VkDeviceSize) size);
		case polvMemHostCoherent:
			return polvCoreMemoryAllocHostCoherent((VkDeviceSize) size);
	}

	return NULL;
}


void polvFree(void *addr)
{
	if (!initialized || !addr)
		return;

	polvCoreMemoryFree((POLVCoreMemory *) addr);
}


void *polvGetHostPointer(void *addr)
{
	if (!initialized || !addr)
		return NULL;

	return polvCoreMemoryGetHostPointer((POLVCoreMemory *) addr);
}


POLVResult polvMemcpy(const void *src, size_t src_offset, void *dst, size_t dst_offset,
                      size_t size, POLVMemcpyDirection direction)
{
	POLVCoreResult res;

	if (!initialized)
		return POLV_ERROR_NOT_INITIALIZED;

	if (!src || !dst || size == 0)
		return POLV_ERROR_INVALID_ARGUMENT;

	switch (direction)
	{
		case polvHostToDevice:
			res = polvCoreMemoryCopyH2D(src, src_offset, (POLVCoreMemory *) dst,
			                            dst_offset, size);
			break;
		case polvDeviceToHost:
			res = polvCoreMemoryCopyD2H((POLVCoreMemory *) src, src_offset, dst,
			                            dst_offset, size);
			break;
		case polvDeviceToDevice:
			res = polvCoreMemoryCopyD2D((POLVCoreMemory *) src, src_offset,
			                            (POLVCoreMemory *) dst, dst_offset, size);
			break;
		case polvHostToHost:
			res = polvCoreMemoryCopyH2H(src, src_offset, dst, dst_offset, size);
			break;
		default:
			return POLV_ERROR_UNSUPPORTED;
	}

	return polv_status_from_core(res);
}


/**************************************************************
 *                                                            *
 * KERNELS                                                    *
 *                                                            *
 **************************************************************/

POLVResult polvKernelLaunch(const char *filename, void **args, int nargs,
                            POLVDim grid, POLVDim group)
{
	POLVCoreContext *context;
	POLVCoreKernel *kernel;
	POLVCoreResult core_res;
	POLVResult res;

	if (!initialized)
		return POLV_ERROR_NOT_INITIALIZED;

	if (!filename || !args || nargs <= 0)
		return POLV_ERROR_INVALID_ARGUMENT;

	context = polv_contexts_get_current_context();
	if (!context)
		return POLV_ERROR_CONTEXT_NOT_INITIALIZED;

	if ((res = polv_kernels_get_or_create(context, filename, nargs, &kernel)) 
		!= POLV_SUCCESS)
		return res;

	core_res = polvCoreKernelLaunch(kernel, args,
	                                grid.x, grid.y, grid.z,
	                                group.x, group.y, group.z);

	return polv_status_from_core(core_res);
}


/**************************************************************
 *                                                            *
 * MISCELLANEOUS                                              *
 *                                                            *
 **************************************************************/

const char *polvStatus(POLVResult status)
{
	switch (status)
	{
		case POLV_SUCCESS:
			return "success";
		case POLV_ERROR:
			return "generic error";
		case POLV_ERROR_NOT_INITIALIZED:
			return "POLV is not initialized";
		case POLV_ERROR_INVALID_ARGUMENT:
			return "invalid argument";
		case POLV_ERROR_NO_DEVICE:
			return "no suitable Vulkan compute device";
		case POLV_ERROR_VULKAN:
			return "Vulkan API error";
		case POLV_ERROR_OUT_OF_MEMORY:
			return "out of memory";
		case POLV_ERROR_SHADER:
			return "shader error";
		case POLV_ERROR_UNSUPPORTED:
			return "unsupported operation";
		case POLV_ERROR_DEVICE_ID_OUT_OF_BOUNDS:
			return "device ID out of bounds";
		case POLV_ERROR_CONTEXT_MISMATCH:
			return "object belongs to another context";
		case POLV_ERROR_CONTEXT_NOT_INITIALIZED:
			return "context not initialized";
		default:
			return "unknown POLV error";
	}
}
