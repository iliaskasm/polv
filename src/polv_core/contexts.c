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
 * POLV Core context helpers (internal)
 */
#include <stdlib.h>
#include <string.h>
#include "contexts.h"
#include "memory.h"
#include "kernels.h"
#include "polv_core_internal.h"

static POLVCoreContext *current_context = NULL;

/* Returns current context. */
POLVCoreContext *polvc_contexts_get_current(void)
{
	return current_context;
}


/* Sets current context. */
void polvc_contexts_set_current(POLVCoreContext *context)
{
	current_context = context;
}


/* Creates the Vulkan command pool, for a given context. */
POLVCoreResult polvc_contexts_create_command_pool(POLVCoreContext *context)
{
	VkCommandPoolCreateInfo pi;

	if (!context || !context->p_device || context->p_device->device == VK_NULL_HANDLE)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	memset(&pi, 0, sizeof(pi));
	pi.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pi.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pi.queueFamilyIndex = context->compute_queue_family;

	if (vkCreateCommandPool(context->p_device->device, &pi, NULL,
	                        &context->command_pool) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}


/* Creates the Vulkan command buffer, for a given context. */
POLVCoreResult polvc_contexts_create_command_buffer(POLVCoreContext *context)
{
	VkCommandBufferAllocateInfo ai;

	if (!context || context->command_pool == VK_NULL_HANDLE)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	memset(&ai, 0, sizeof(ai));
	ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	ai.commandPool = context->command_pool;
	ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	ai.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(context->p_device->device, &ai,
	                             &context->command_buffer) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}


/* Adds a context to the global context list. */
void polvc_contexts_link(POLVCoreContext *context)
{
	POLVCoreDevice *dev;

	if (!context || !context->p_device)
		return;

	dev = context->p_device;

	context->prev = NULL;
	context->next = dev->contexts;

	if (dev->contexts)
		dev->contexts->prev = context;

	dev->contexts = context;
}


/* Destroy a specific context and removes it from the list. */
void polvc_contexts_destroy(POLVCoreContext *context)
{
	POLVCoreDevice *dev;
	POLVCoreKernel *kernel,*next_kernel;
	POLVCoreMemory *ptr, *next;

	if (!context || !context->p_device)
		return;

	dev = context->p_device;
	if (dev->device != VK_NULL_HANDLE)
		vkDeviceWaitIdle(dev->device); // wait first

	if (current_context == context)
		current_context = NULL;

	/* (1) Retrieve and destroy kernels */
	kernel = context->kernels;
	while (kernel)
	{
		next_kernel = kernel->next;
		polvc_kernels_destroy(kernel, 0);
		kernel = next_kernel;
	}
	context->kernels = NULL;

	/* (2) Destroy internal staging allocations */
	if (context->upload_staging)
	{
		polvc_memory_destroy(context->upload_staging, 0);
		context->upload_staging = NULL;
	}

	if (context->download_staging)
	{
		polvc_memory_destroy(context->download_staging, 0);
		context->download_staging = NULL;
	}

	/* (3) Retrieve and destroy memory allocations */
	ptr = context->allocations;
	while (ptr)
	{
		next = ptr->next;
		polvc_memory_destroy(ptr, 0);
		ptr = next;
	}
	context->allocations = NULL;

	/* (4) Remove context from the list */
	if (context->prev)
		context->prev->next = context->next;
	else if (dev->contexts == context)
		dev->contexts = context->next;

	if (context->next)
		context->next->prev = context->prev;

	/* (5) Destroy the command pool of the context */
	if (context->command_pool != VK_NULL_HANDLE &&
		dev->device != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(dev->device,
		                     context->command_pool, NULL);
	}
	context->p_device = NULL;
	free(context);
}
