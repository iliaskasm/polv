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
 * POLV Core memory helpers (internal)
 */
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "runtime.h"
#include "polv_core_internal.h"

/* Scores preferred memory properties.
 * Start from host-cached, proceed to host-coherent and
 * lastly, device-local.
 */
static unsigned get_memory_type_score(VkMemoryPropertyFlags properties,
                                      VkMemoryPropertyFlags preferred)
{
	unsigned score = 0;

	if ((preferred & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) &&
	    (properties & VK_MEMORY_PROPERTY_HOST_CACHED_BIT))
		score += 4;

	if ((preferred & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) &&
	    (properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
		score += 2;

	if ((preferred & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
	    (properties & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
		score += 1;

	return score;
}


/* Finds memory satisfying required flags while preferring additional flags. */
static POLVCoreResult find_memory_type(POLVCoreDevice *dev, uint32_t type_filter,
                                       VkMemoryPropertyFlags required,
                                       VkMemoryPropertyFlags preferred,
                                       uint32_t *type_index,
                                       VkMemoryPropertyFlags *actual_properties)
{
	const VkPhysicalDeviceMemoryProperties *mp;
	VkMemoryPropertyFlags props;
	uint32_t best_index = UINT32_MAX;
	unsigned best_score = 0;
	unsigned score;
	uint32_t i;

	if (!dev || !type_index)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	mp = &dev->pdev.memory_properties;
	for (i = 0; i < mp->memoryTypeCount; ++i)
	{
		if (!(type_filter & (1u << i)))
			continue;

		props = mp->memoryTypes[i].propertyFlags;
		if ((props & required) != required)
			continue;

		score = get_memory_type_score(props, preferred);
		if (best_index == UINT32_MAX || score > best_score)
		{
			best_index = i;
			best_score = score;
		}
	}

	if (best_index == UINT32_MAX)
		return POLV_CORE_ERROR_UNSUPPORTED;

	*type_index = best_index;
	if (actual_properties)
		*actual_properties = mp->memoryTypes[best_index].propertyFlags;

	return POLV_CORE_SUCCESS;
}


/* Adds memory to the allocations list. */
static void allocation_link(POLVCoreContext *context, POLVCoreMemory *ptr)
{
	ptr->prev = NULL;
	ptr->next = context->allocations;
	if (context->allocations)
		context->allocations->prev = ptr;
	context->allocations = ptr;
}


/* Removes memory from the allocations list. */
static void allocation_unlink(POLVCoreMemory *ptr)
{
	POLVCoreContext *context;

	if (!ptr || !ptr->owner)
		return;
	context = ptr->owner;

	if (ptr->prev)
		ptr->prev->next = ptr->next;
	else if (context->allocations == ptr)
		context->allocations = ptr->next;

	if (ptr->next)
		ptr->next->prev = ptr->prev;

	ptr->prev = NULL;
	ptr->next = NULL;
}


/* Allocates memory given a specific size and memory properties.
 * track == 1 adds the allocation to the context list.
 */
POLVCoreMemory *polvc_memory_allocate(POLVCoreContext *context, VkDeviceSize size,
                                      VkMemoryPropertyFlags required,
                                      VkMemoryPropertyFlags preferred,
                                      int track)
{
	VkBufferCreateInfo buffer_ci;
	VkMemoryRequirements req;
	VkMemoryAllocateInfo mem_ai;
	POLVCoreMemory *ptr;
	POLVCoreDevice *dev;
	uint32_t memtype;
	VkMemoryPropertyFlags actual = 0;

	if (!polvc_runtime_state()->initialized || !context 
		|| !context->p_device || size == 0)
		return NULL;

	dev = context->p_device;
	if (dev->device == VK_NULL_HANDLE)
		return NULL;

	ptr = (POLVCoreMemory *) calloc(1, sizeof(*ptr));
	if (!ptr)
		return NULL;

	ptr->owner = context;

	memset(&buffer_ci, 0, sizeof(buffer_ci));
	buffer_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_ci.size = size;
	buffer_ci.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
	                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
	                  VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(dev->device, &buffer_ci, NULL,
	                   &ptr->buffer) != VK_SUCCESS)
		goto FAIL;

	vkGetBufferMemoryRequirements(dev->device, ptr->buffer, &req);

	if (find_memory_type(dev, req.memoryTypeBits, required, preferred,
	                     &memtype, &actual) != POLV_CORE_SUCCESS)
		goto FAIL;

	memset(&mem_ai, 0, sizeof(mem_ai));
	mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mem_ai.allocationSize = req.size;
	mem_ai.memoryTypeIndex = memtype;

	if (vkAllocateMemory(dev->device, &mem_ai, NULL,
	                     &ptr->mem) != VK_SUCCESS)
		goto FAIL;

	if (vkBindBufferMemory(dev->device, ptr->buffer,
	                       ptr->mem, 0) != VK_SUCCESS)
		goto FAIL;

	ptr->size = size;
	ptr->allocation_size = req.size;
	ptr->memory_properties = actual;

	/* Map host-visible memory */
	if (actual & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		if (vkMapMemory(dev->device, ptr->mem, 0, VK_WHOLE_SIZE, 
		                0, &ptr->host_ptr) != VK_SUCCESS)
			goto FAIL;
	}

	if (track)
		allocation_link(context, ptr);

	return ptr;

FAIL:
	if (ptr->host_ptr)
	{
		vkUnmapMemory(dev->device, ptr->mem);
		ptr->host_ptr = NULL;
	}

	if (ptr->buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(dev->device, ptr->buffer, NULL);

	if (ptr->mem != VK_NULL_HANDLE)
		vkFreeMemory(dev->device, ptr->mem, NULL);

	ptr->owner = NULL;
	free(ptr);

	return NULL;
}


/* Allocates host-visible staging memory of at least size bytes. */
POLVCoreResult polvc_memory_alloc_staging(POLVCoreContext *context, POLVCoreMemory **slot,
                                          VkDeviceSize size, VkMemoryPropertyFlags preferred,
                                          POLVCoreMemory **result)
{
	POLVCoreMemory *staging;
	VkResult vr;

	if (!context || !slot || !result || size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	*result = NULL;

	if (*slot && (*slot)->size >= size)
	{
		*result = *slot;
		return POLV_CORE_SUCCESS;
	}

	staging = polvc_memory_allocate(context, size,
	                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
	                                preferred, 0);
	if (!staging)
		return POLV_CORE_ERROR_OUT_OF_MEMORY;

	if (*slot)
	{
		if ((vr = vkQueueWaitIdle(context->compute_queue)) != VK_SUCCESS)
		{
			polvc_memory_destroy(staging, 0);
			return POLV_CORE_ERROR_VULKAN;
		}

		polvc_memory_destroy(*slot, 0);
	}

	*slot = staging;
	*result = staging;
	return POLV_CORE_SUCCESS;
}


/* Destroys memory and removes it from the allocations list if unlink == 1. */
void polvc_memory_destroy(POLVCoreMemory *ptr, int unlink)
{
	POLVCoreContext *context;
	POLVCoreDevice *dev;

	if (!ptr || !ptr->owner)
		return;

	context = ptr->owner;
	dev = context->p_device;

	if (unlink)
		allocation_unlink(ptr);

	if (ptr->host_ptr)
	{
		vkUnmapMemory(dev->device, ptr->mem);
		ptr->host_ptr = NULL;
	}

	if (ptr->buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(dev->device, ptr->buffer, NULL);

	if (ptr->mem != VK_NULL_HANDLE)
		vkFreeMemory(dev->device, ptr->mem, NULL);

	ptr->owner = NULL;
	free(ptr);
}


/* Creates a VkMappedMemoryRange aligned to nonCoherentAtomSize */
static void make_mapped_range(POLVCoreMemory *memory, VkDeviceSize offset,
                              VkDeviceSize size, VkMappedMemoryRange *range)
{
	VkDeviceSize atom, begin, end, remainder;

	atom = memory->owner->p_device->pdev.properties.limits.nonCoherentAtomSize;
	if (atom == 0)
		atom = 1;

	begin = offset - (offset % atom);
	end = offset + size;
	remainder = end % atom;
	if (remainder != 0)
		end += atom - remainder;

	if (end > memory->allocation_size)
		end = memory->allocation_size;

	memset(range, 0, sizeof(*range));
	range->sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	range->memory = memory->mem;
	range->offset = begin;
	range->size = end - begin;
}


/* Performs a write to mapped memory. */
POLVCoreResult polvc_memory_map_write(POLVCoreMemory *dst, size_t offset,
                                      const void *src, size_t size)
{
	POLVCoreDevice *dev;
	VkMappedMemoryRange range;
	VkResult vr;

	if (!dst || !dst->owner || !src || size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (!(dst->memory_properties &
	      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		return POLV_CORE_ERROR_UNSUPPORTED;

	if (!dst->host_ptr)
		return POLV_CORE_ERROR_VULKAN;

	if (offset > dst->size ||
		(VkDeviceSize) size > dst->size - (VkDeviceSize) offset)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = dst->owner->p_device;

	memcpy((char *) dst->host_ptr + offset, src, size);

	if (!(dst->memory_properties &
	      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
	{
		make_mapped_range(dst, (VkDeviceSize) offset,
		                  (VkDeviceSize) size, &range);

		if ((vr = vkFlushMappedMemoryRanges(dev->device, 1, &range)) 
			!= VK_SUCCESS)
			return POLV_CORE_ERROR_VULKAN;
	}

	return POLV_CORE_SUCCESS;
}


/* Performs a read from mapped memory. */
POLVCoreResult polvc_memory_map_read(POLVCoreMemory *src, size_t offset,
                                     void *dst, size_t size)
{
	POLVCoreDevice *dev;
	VkMappedMemoryRange range;
	VkResult vr;

	if (!src || !src->owner || !dst || size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	if (!(src->memory_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
		return POLV_CORE_ERROR_UNSUPPORTED;

	if (!src->host_ptr)
		return POLV_CORE_ERROR_VULKAN;

	if (offset > src->size ||
		(VkDeviceSize) size > src->size - (VkDeviceSize) offset)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = src->owner->p_device;

	if (!(src->memory_properties &
	      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
	{
		make_mapped_range(src, (VkDeviceSize) offset,
		                  (VkDeviceSize) size, &range);

		if ((vr = vkInvalidateMappedMemoryRanges(dev->device, 1, &range)) 
			!= VK_SUCCESS)
			return POLV_CORE_ERROR_VULKAN;
	}

	memcpy(dst, (const char *) src->host_ptr + offset, size);

	return POLV_CORE_SUCCESS;
}


/* Performs a sync buffer-to-buffer copy, in a given context. */
POLVCoreResult polvc_memory_submit_buffer_copy(POLVCoreContext *context,
                                               VkBuffer src, VkBuffer dst,
                                               VkDeviceSize src_offset,
                                               VkDeviceSize dst_offset,
                                               VkDeviceSize size)
{
	VkCommandBufferBeginInfo bi;
	VkBufferCopy region;
	VkSubmitInfo submit;
	VkResult vr;
	VkMemoryBarrier barrier;

	if (!context || !context->p_device || src == VK_NULL_HANDLE ||
	    dst == VK_NULL_HANDLE || size == 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	/* Reuse the context's command buffer for this synchronous copy */
	if ((vr = vkResetCommandBuffer(context->command_buffer, 0)) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	memset(&bi, 0, sizeof(bi));
	bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if ((vr = vkBeginCommandBuffer(context->command_buffer, &bi)) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	memset(&region, 0, sizeof(region));
	region.srcOffset = src_offset;
	region.dstOffset = dst_offset;
	region.size = size;

	vkCmdCopyBuffer(context->command_buffer, src, dst, 1, &region);

	/* Make transfer writes available and visible to subsequent host reads */
	memset(&barrier, 0, sizeof(barrier));
	barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

	vkCmdPipelineBarrier(
		context->command_buffer,
		VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_HOST_BIT,
		0,
		1, &barrier,
		0, NULL,
		0, NULL
	);

	if ((vr = vkEndCommandBuffer(context->command_buffer)) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	memset(&submit, 0, sizeof(submit));
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &context->command_buffer;

	/* Submit the copy and wait so this helper remains synchronous */
	if ((vr = vkQueueSubmit(context->compute_queue, 1, &submit, VK_NULL_HANDLE))
	    != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	if ((vr = vkQueueWaitIdle(context->compute_queue)) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}
