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
 * POLV Core API headers (internal)
 */

#ifndef POLV_CORE_INTERNAL_H
#define POLV_CORE_INTERNAL_H

#include "polv_core.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define POLV_SHADER_CACHE_SIZE 256

typedef struct POLVCoreShader_         POLVCoreShader;
typedef struct POLVCorePhysicalDevice_ POLVCorePhysicalDevice;

/*
 * Memory object.
 *
 * Memory is owned by the context in which it was allocated.
 */
struct POLVCoreMemory_
{
	POLVCoreContext        *owner;
	VkBuffer                buffer;
	VkDeviceMemory          mem;
	VkDeviceSize            size;
	VkDeviceSize            allocation_size;
	VkMemoryPropertyFlags   memory_properties;
	void                   *host_ptr;
	POLVCoreMemory         *prev;
	POLVCoreMemory         *next;
};


/*
 * Shader module.
 *
 * VkShaderModule objects belong to a VkDevice, so shaders are
 * associated with a POLVCoreDevice rather than a context.
 */
struct POLVCoreShader_
{
	POLVCoreDevice *owner;
	char           *filename;
	char           *cache_key;
	VkShaderModule  compute_shader_module;
};


/*
 * Kernel object.
 *
 * A kernel is created in the current context and remains associated
 * with that context regardless of subsequent current-context changes.
 */
struct POLVCorePipeline_
{
	uint32_t                  group_x;
	uint32_t                  group_y;
	uint32_t                  group_z;
	VkPipeline                compute_pipeline;
	struct POLVCorePipeline_ *next;
};

struct POLVCoreKernel_
{
	struct POLVCoreContext_  *owner;
	struct POLVCoreShader_   *shader;
	int                       nargs;
	VkDescriptorSetLayout     descriptor_set_layout;
	VkPipelineLayout          pipeline_layout;
	VkDescriptorPool          descriptor_pool;
	VkDescriptorSet           descriptor_set;
	struct POLVCorePipeline_ *pipelines;
	struct POLVCoreKernel_   *prev;
	struct POLVCoreKernel_   *next;
};

/*
 * Physical-device information.
 */
struct POLVCorePhysicalDevice_
{
	VkPhysicalDevice                 physical_device;
	VkPhysicalDeviceProperties       properties;
	VkPhysicalDeviceFeatures         features;
	VkPhysicalDeviceMemoryProperties memory_properties;
	uint32_t                         queue_family;
};


/*
 * Execution context.
 */
struct POLVCoreContext_
{
	POLVCoreDevice  *p_device;
	VkQueue          compute_queue;
	VkCommandPool    command_pool;
	VkCommandBuffer  command_buffer;
	uint32_t         compute_queue_family;
	POLVCoreMemory  *allocations;
	POLVCoreKernel  *kernels;
	POLVCoreMemory  *upload_staging;
	POLVCoreMemory  *download_staging;
	POLVCoreContext *prev;
	POLVCoreContext *next;
};


/*
 * Logical POLV device.
 */
struct POLVCoreDevice_
{
	int                     id;
	int                     global_id;
	VkDevice                device;
	POLVCorePhysicalDevice  pdev;
	int                     nshaders;
	POLVCoreShader          shader_cache[POLV_SHADER_CACHE_SIZE];
	POLVCoreContext        *contexts;
	uint64_t                num_launched_kernels;
};


#ifdef __cplusplus
}
#endif

#endif /* POLV_CORE_INTERNAL_H */
