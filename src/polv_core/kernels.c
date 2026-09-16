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
 * POLV Core kernel helpers (internal)
 */
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "kernels.h"
#include "polv_core_internal.h"

static char *dup_string(const char *s)
{
	size_t n;
	char *copy;

	if (!s)
		return NULL;
	n = strlen(s) + 1;
	copy = (char *) malloc(n);
	if (copy)
		memcpy(copy, s, n);
	return copy;
}

/* Reads a file and dumps it to a 32-bit unsigned int. */
static POLVCoreResult read_file(const char *path, uint32_t **out, size_t *out_size)
{
	FILE *fp;
	long n;

	if (!path || !out || !out_size)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	*out = NULL;
	*out_size = 0;

	fp = fopen(path, "rb");
	if (!fp)
		return POLV_CORE_ERROR_SHADER;

	if (fseek(fp, 0, SEEK_END) != 0)
	{
		fclose(fp);
		return POLV_CORE_ERROR_SHADER;
	}

	n = ftell(fp);
	if (n <= 0 || ((size_t) n % sizeof(uint32_t)) != 0)
	{
		fclose(fp);
		return POLV_CORE_ERROR_SHADER;
	}

	if (fseek(fp, 0, SEEK_SET) != 0)
	{
		fclose(fp);
		return POLV_CORE_ERROR_SHADER;
	}

	*out = (uint32_t *) malloc((size_t) n);
	if (!*out)
	{
		fclose(fp);
		return POLV_CORE_ERROR_OUT_OF_MEMORY;
	}

	if (fread(*out, 1, (size_t) n, fp) != (size_t) n)
	{
		free(*out);
		*out = NULL;
		fclose(fp);
		return POLV_CORE_ERROR_SHADER;
	}

	fclose(fp);
	*out_size = (size_t) n;
	return POLV_CORE_SUCCESS;
}


/* Makes a shader cache key, for a given filename.
 * Key is in the following form:
 *
 *   <path>|dev=<storage_device>|ino=<inode>|size=<filesize>|mtime=<modification_time>
 */
static char *make_shader_cache_key(const char *filename)
{
	char resolved[PATH_MAX];
	struct stat st;
	char *key;
	int n;

	if (!filename || realpath(filename, resolved) == NULL)
		return NULL;
	if (stat(resolved, &st) != 0)
		return NULL;

	n = snprintf(NULL, 0, "%s|dev=%ld|ino=%ld|size=%ld|mtime=%ld",
	                      resolved, (long) st.st_dev, (long) st.st_ino,
	                      (long) st.st_size, (long) st.st_mtime);
	if (n < 0)
		return NULL;

	key = (char *) malloc((size_t) n + 1);
	if (!key)
		return NULL;

	snprintf(key, (size_t) n + 1, "%s|dev=%ld|ino=%ld|size=%ld|mtime=%ld",
	                             resolved, (long) st.st_dev, (long) st.st_ino,
	                             (long) st.st_size, (long) st.st_mtime);
	return key;
}


/* Destroys a shader module. */
static void destroy_shader_module(POLVCoreDevice *dev, POLVCoreShader *s)
{
	if (!dev || !s || dev->device == VK_NULL_HANDLE)
		return;

	if (s->compute_shader_module != VK_NULL_HANDLE)
		vkDestroyShaderModule(dev->device, s->compute_shader_module, NULL);

	s->compute_shader_module = VK_NULL_HANDLE;
}


/* Destroys a shader given its ID. */
void polvc_kernels_shader_destroy(POLVCoreDevice *dev, int shader_id)
{
	POLVCoreShader *s;

	if (!dev || shader_id < 0 || shader_id >= POLV_SHADER_CACHE_SIZE)
		return;

	s = &dev->shader_cache[shader_id];
	destroy_shader_module(dev, s);
	free(s->filename);
	free(s->cache_key);
	memset(s, 0, sizeof(*s));
}


/* Creates a shader given its filename (*.spv). */
int polvc_kernels_shader_new(POLVCoreDevice *dev, const char *shader_filename)
{
	VkShaderModuleCreateInfo ci;
	uint32_t *code;
	size_t code_size;
	char *new_key;
	POLVCoreShader *s;
	int id;
	POLVCoreResult res;

	if (!dev || !shader_filename)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	/* (1) Create the key */
	new_key = make_shader_cache_key(shader_filename);
	if (!new_key)
		return POLV_CORE_ERROR_SHADER;

	/* (2) Search the cache for the key; if found, return shader ID immediately */
	for (id = 0; id < dev->nshaders; ++id)
	{
		if (dev->shader_cache[id].cache_key &&
			strcmp(dev->shader_cache[id].cache_key, new_key) == 0)
		{
			free(new_key);
			return id;
		}
	}

	if (dev->nshaders >= POLV_SHADER_CACHE_SIZE)
	{
		free(new_key);
		return POLV_CORE_ERROR_SHADER;
	}

	/* (3) Shader was not cached; read it from disk */
	code = NULL;
	code_size = 0;
	if ((res = read_file(shader_filename, &code, &code_size)) 
		!= POLV_CORE_SUCCESS)
	{
		free(new_key);
		return res;
	}

	/* (4) Create a new shader and cache it */
	id = dev->nshaders;
	s = &dev->shader_cache[id];
	memset(s, 0, sizeof(*s));
	
	s->owner = dev;
	s->filename = dup_string(shader_filename);
	s->cache_key = new_key;

	if (!s->filename)
	{
		free(code);
		polvc_kernels_shader_destroy(dev, id);
		return POLV_CORE_ERROR_OUT_OF_MEMORY;
	}

	/* (5) Create the shader module */
	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	ci.codeSize = code_size;
	ci.pCode = code;

	if (vkCreateShaderModule(dev->device, &ci, NULL,
	                         &s->compute_shader_module) != VK_SUCCESS)
	{
		free(code);
		polvc_kernels_shader_destroy(dev, id);
		return POLV_CORE_ERROR_SHADER;
	}

	free(code);
	++dev->nshaders;
	return id;
}


/* Adds a kernel to the list. */
void polvc_kernels_link(POLVCoreContext *context, POLVCoreKernel *kernel)
{
	kernel->prev = NULL;
	kernel->next = context->kernels;

	if (context->kernels)
		context->kernels->prev = kernel;

	context->kernels = kernel;
}


/* Removes a kernel from the list. */
static void kernel_unlink(POLVCoreKernel *kernel)
{
	POLVCoreContext *context;

	if (!kernel || !kernel->owner)
		return;

	context = kernel->owner;

	if (kernel->prev)
		kernel->prev->next = kernel->next;
	else if (context->kernels == kernel)
		context->kernels = kernel->next;

	if (kernel->next)
		kernel->next->prev = kernel->prev;

	kernel->prev = NULL;
	kernel->next = NULL;
}


/* Destroys a specific kernel; removes it from the list if unlink == 1. */
void polvc_kernels_destroy(POLVCoreKernel *kernel, int unlink)
{
	POLVCoreContext *context;
	POLVCoreDevice *dev;
	struct POLVCorePipeline_ *pipeline, *next;

	if (!kernel || !kernel->owner)
		return;

	context = kernel->owner;
	dev = context->p_device;

	if (unlink)
		kernel_unlink(kernel);

	/* (1) Destroy all the pipelines */
	pipeline = kernel->pipelines;
	while (pipeline)
	{
		next = pipeline->next;

		if (pipeline->compute_pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(dev->device, pipeline->compute_pipeline, NULL);

		free(pipeline);
		pipeline = next;
	}
	kernel->pipelines = NULL;

	/* (2) Destroy PL, DP and DSL */
	if (kernel->pipeline_layout != VK_NULL_HANDLE)
		vkDestroyPipelineLayout(dev->device, kernel->pipeline_layout, NULL);

	if (kernel->descriptor_pool != VK_NULL_HANDLE)
		vkDestroyDescriptorPool(dev->device, kernel->descriptor_pool, NULL);

	if (kernel->descriptor_set_layout != VK_NULL_HANDLE)
		vkDestroyDescriptorSetLayout(dev->device, kernel->descriptor_set_layout, NULL);

	kernel->owner = NULL;
	kernel->shader = NULL;
	free(kernel);
}


/* Creates a descriptor set layout for a given kernel. */
POLVCoreResult polvc_kernels_create_descriptor_set_layout(POLVCoreKernel *kernel)
{
	POLVCoreDevice *dev;
	VkDescriptorSetLayoutBinding *bindings;
	VkDescriptorSetLayoutCreateInfo ci;
	int i;

	if (!kernel || !kernel->owner || kernel->nargs <= 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	bindings = (VkDescriptorSetLayoutBinding *) 
		calloc((size_t) kernel->nargs, sizeof(*bindings));
	if (!bindings)
		return POLV_CORE_ERROR_OUT_OF_MEMORY;

	for (i = 0; i < kernel->nargs; ++i)
	{
		bindings[i].binding = (uint32_t) i;
		bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bindings[i].descriptorCount = 1;
		bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	}

	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	ci.bindingCount = (uint32_t) kernel->nargs;
	ci.pBindings = bindings;

	if (vkCreateDescriptorSetLayout(dev->device, &ci, NULL,
	                                &kernel->descriptor_set_layout) != VK_SUCCESS)
	{
		free(bindings);
		return POLV_CORE_ERROR_VULKAN;
	}

	free(bindings);
	return POLV_CORE_SUCCESS;
}


/* Creates a pipeline layout for a given kernel. */
POLVCoreResult polvc_kernels_create_pipeline_layout(POLVCoreKernel *kernel)
{
	POLVCoreDevice *dev;
	VkPipelineLayoutCreateInfo ci;

	if (!kernel || !kernel->owner)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	ci.setLayoutCount = 1;
	ci.pSetLayouts = &kernel->descriptor_set_layout;

	if (vkCreatePipelineLayout(dev->device, &ci, NULL,
	                           &kernel->pipeline_layout) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}


/* Creates a descriptor pool for a given kernel. */
POLVCoreResult polvc_kernels_create_descriptor_pool(POLVCoreKernel *kernel)
{
	POLVCoreDevice *dev;
	VkDescriptorPoolSize pool_size;
	VkDescriptorPoolCreateInfo ci;

	if (!kernel || !kernel->owner || kernel->nargs <= 0)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	memset(&pool_size, 0, sizeof(pool_size));
	pool_size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_size.descriptorCount = (uint32_t) kernel->nargs;

	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	ci.maxSets = 1;
	ci.poolSizeCount = 1;
	ci.pPoolSizes = &pool_size;

	if (vkCreateDescriptorPool(dev->device, &ci, NULL,
	                           &kernel->descriptor_pool) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}


/* Allocates a descriptor set for a given kernel. */
POLVCoreResult polvc_kernels_allocate_descriptor_set(POLVCoreKernel *kernel)
{
	POLVCoreDevice *dev;
	VkDescriptorSetAllocateInfo ai;

	if (!kernel || !kernel->owner)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	memset(&ai, 0, sizeof(ai));
	ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	ai.descriptorPool = kernel->descriptor_pool;
	ai.descriptorSetCount = 1;
	ai.pSetLayouts = &kernel->descriptor_set_layout;

	if (vkAllocateDescriptorSets(dev->device, &ai,
	                             &kernel->descriptor_set) != VK_SUCCESS)
		return POLV_CORE_ERROR_VULKAN;

	return POLV_CORE_SUCCESS;
}


/* Creates a compute pipeline for a given kernel and its group dimensions. */
static POLVCoreResult kernel_create_pipeline(POLVCoreKernel *kernel, uint32_t group_x, 
                                             uint32_t group_y, uint32_t group_z,
                                             VkPipeline *compute_pipeline)
{
	POLVCoreDevice *dev;
	struct POLVCorePipeline_ *pipeline;
	VkSpecializationMapEntry spec_entries[3];
	VkSpecializationInfo spec_info;
	VkPipelineShaderStageCreateInfo stage;
	VkComputePipelineCreateInfo ci;
	uint32_t group_size[3];
	int i, nsizes = 3;

	if (!kernel || !kernel->owner || !kernel->shader || !compute_pipeline)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	dev = kernel->owner->p_device;

	pipeline = (struct POLVCorePipeline_ *) calloc(1, sizeof(*pipeline));
	if (!pipeline)
		return POLV_CORE_ERROR_OUT_OF_MEMORY;

	/* local_size_x_id, local_size_y_id and local_size_z_id */
	group_size[0] = group_x;
	group_size[1] = group_y;
	group_size[2] = group_z;

	for (i = 0; i < nsizes; i++)
	{
		spec_entries[i].constantID = i;
		spec_entries[i].offset = i * sizeof(uint32_t);
		spec_entries[i].size = sizeof(uint32_t);
	}

	memset(&spec_info, 0, sizeof(spec_info));
	spec_info.mapEntryCount = (uint32_t) nsizes;
	spec_info.pMapEntries = spec_entries;
	spec_info.dataSize = sizeof(group_size);
	spec_info.pData = group_size;

	memset(&stage, 0, sizeof(stage));
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module = kernel->shader->compute_shader_module;
	stage.pName = "main";
	stage.pSpecializationInfo = &spec_info;

	memset(&ci, 0, sizeof(ci));
	ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	ci.stage = stage;
	ci.layout = kernel->pipeline_layout;

	if (vkCreateComputePipelines(dev->device, VK_NULL_HANDLE, 1, &ci, NULL,
	                             &pipeline->compute_pipeline) != VK_SUCCESS)
	{
		free(pipeline);
		return POLV_CORE_ERROR_VULKAN;
	}

	pipeline->group_x = group_x;
	pipeline->group_y = group_y;
	pipeline->group_z = group_z;
	pipeline->next = kernel->pipelines;
	kernel->pipelines = pipeline;

	*compute_pipeline = pipeline->compute_pipeline;

	return POLV_CORE_SUCCESS;
}


/* Returns the compute pipeline of a given kernel, if its dimensions match,
 * otherwise it creates one.
 */
POLVCoreResult polvc_kernels_get_pipeline(POLVCoreKernel *kernel, uint32_t group_x, 
                                          uint32_t group_y, uint32_t group_z,
                                          VkPipeline *compute_pipeline)
{
	struct POLVCorePipeline_ *pipeline;

	if (!kernel || !compute_pipeline)
		return POLV_CORE_ERROR_INVALID_ARGUMENT;

	for (pipeline = kernel->pipelines; pipeline; pipeline = pipeline->next)
	{
		if (pipeline->group_x == group_x &&
			pipeline->group_y == group_y &&
			pipeline->group_z == group_z)
		{
			*compute_pipeline = pipeline->compute_pipeline;
			return POLV_CORE_SUCCESS;
		}
	}

	/* Pipeline not found, create one */
	return kernel_create_pipeline(kernel, group_x, group_y, group_z,
	                              compute_pipeline);
}
