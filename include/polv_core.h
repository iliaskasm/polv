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
 * POLV Core API headers
 */
#ifndef POLV_CORE_H
#define POLV_CORE_H

#include <vulkan/vulkan.h>
#include <stddef.h>
#include <stdint.h>

#include "polv_version.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef POLV_CORE_API
#define POLV_CORE_API extern
#endif

typedef enum POLVCoreResult_
{
	POLV_CORE_SUCCESS = 0,
	POLV_CORE_ERROR = -1,
	POLV_CORE_ERROR_NOT_INITIALIZED = -2,
	POLV_CORE_ERROR_INVALID_ARGUMENT = -3,
	POLV_CORE_ERROR_NO_DEVICE = -4,
	POLV_CORE_ERROR_VULKAN = -5,
	POLV_CORE_ERROR_OUT_OF_MEMORY = -6,
	POLV_CORE_ERROR_SHADER = -7,
	POLV_CORE_ERROR_UNSUPPORTED = -8,
	POLV_CORE_ERROR_DEVICE_ID_OUT_OF_BOUNDS = -9,
	POLV_CORE_ERROR_CONTEXT_MISMATCH = -10,
	POLV_CORE_ERROR_CONTEXT_NOT_INITIALIZED = -11
} POLVCoreResult;

typedef struct POLVCoreMemory_  POLVCoreMemory;
typedef struct POLVCoreKernel_  POLVCoreKernel;
typedef struct POLVCoreContext_ POLVCoreContext;
typedef struct POLVCoreDevice_  POLVCoreDevice;


/**************************************************************
 *                                                            *
 * INIT/FINALIZE                                              *
 *                                                            *
 **************************************************************/

/**
 * @brief Initializes the POLV Core API
 * 
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreInit(void);

/**
 * @brief Finalizes the POLV Core API
 * 
 */
POLV_CORE_API void polvCoreFinalize(void);


/**************************************************************
 *                                                            *
 * CONTEXTS                                                   *
 *                                                            *
 **************************************************************/

/**
 * @brief Creates a POLV context on a specific device
 * 
 * @param context (ret) the created context
 * @param dev     the device
 * @return        POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreContextCreate(POLVCoreContext **context, POLVCoreDevice *dev);

/**
 * @brief Destroys a context
 * 
 * @param context the context to destroy
 */
POLV_CORE_API void polvCoreContextDestroy(POLVCoreContext **context);

/**
 * @brief Sets the current POLV context
 * 
 * @param context the context to make current
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreContextSetCurrent(POLVCoreContext *context);

/**
 * @brief Returns the current POLV context
 * 
 * @param context (ret) the current context
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreContextGetCurrent(POLVCoreContext **context);


/**************************************************************
 *                                                            *
 * DEVICE HANDLING                                            *
 *                                                            *
 **************************************************************/

/**
 * @brief Returns the number of Vulkan devices
 * 
 * @return the number of Vulkan devices 
 */
POLV_CORE_API int polvCoreGetNumDevices(void);

/**
 * @brief Returns a pointer to a device, for a given ID
 * 
 * @param id the device ID
 * @return pointer to the device, if the device was found, or NULL otherwise
 */
POLV_CORE_API POLVCoreDevice *polvCoreGetDevice(int device_id);

/**
 * @brief Returns the POLV device ID for a device handle
 *
 * @param dev the device
 * @return the device ID, or -1 if the device is invalid
 */

POLV_CORE_API int polvCoreDeviceGetId(POLVCoreDevice *dev);

/**
 * @brief Returns the cached Vulkan physical-device properties
 *
 * @param dev        the device
 * @param properties (ret) Vulkan physical-device properties
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreDeviceGetProperties(POLVCoreDevice *dev,
                                                         VkPhysicalDeviceProperties *properties);

/**
 * @brief Returns the cached Vulkan physical-device features
 *
 * @param dev      the device
 * @param features (ret) Vulkan physical-device features
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreDeviceGetFeatures(POLVCoreDevice *dev,
                                                       VkPhysicalDeviceFeatures *features);

/**
 * @brief Returns the cached Vulkan physical-device memory properties
 *
 * @param dev               the device
 * @param memory_properties (ret) Vulkan physical-device memory properties
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreDeviceGetMemoryProperties(POLVCoreDevice *dev,
                                                               VkPhysicalDeviceMemoryProperties *memory_properties);

/**
 * @brief Returns the total size of device-local Vulkan memory heaps
 *
 * On unified-memory devices this may represent memory shared with the host; it
 * should not be interpreted as dedicated VRAM on every platform.
 *
 * @param dev the device
 * @return total device-local memory size in bytes, or 0 for an invalid device
 */
POLV_CORE_API uint64_t polvCoreDeviceGetLocalMemorySize(POLVCoreDevice *dev);

/**
 * @brief Returns the compute queue-family index selected by POLV Core
 *
 * @param dev the device
 * @return queue-family index, or UINT32_MAX for an invalid device
 */
POLV_CORE_API uint32_t polvCoreDeviceGetComputeQueueFamily(POLVCoreDevice *dev);


/**************************************************************
 *                                                            *
 * MEMORY                                                     *
 *                                                            *
 **************************************************************/

/**
 * @brief Allocates device-local memory on the current context
 * 
 * @param size the number of bytes to allocate
 * @return device-local memory address
 */
POLV_CORE_API POLVCoreMemory *polvCoreMemoryAllocDeviceLocal(VkDeviceSize size);

/**
 * @brief Allocates host-visible memory on the current context
 * 
 * @param size the number of bytes to allocate
 * @return host-visible memory address
 */
POLV_CORE_API POLVCoreMemory *polvCoreMemoryAllocHostVisible(VkDeviceSize size);

/**
 * @brief Allocates host-coherent memory on the current context
 * 
 * @param size the number of bytes to allocate
 * @return host-coherent memory address
 */
POLV_CORE_API POLVCoreMemory *polvCoreMemoryAllocHostCoherent(VkDeviceSize size);

/**
 * @brief Allocates memory on the current context (DEFAULT: host-coherent)
 * 
 * @param size the number of bytes to allocate
 * @return host-coherent memory address
 */
POLV_CORE_API POLVCoreMemory *polvCoreMemoryAlloc(VkDeviceSize size);

/**
 * @brief Frees a POLV memory allocation
 * 
 * @param addr the address of the memory to be freed
 */
POLV_CORE_API void polvCoreMemoryFree(POLVCoreMemory *addr);

/**
 * @brief Returns the host pointer for a host-coherent allocation
 *
 * @param addr the POLV allocation
 * @return the host pointer, or NULL if the allocation is not host coherent
 */
POLV_CORE_API void *polvCoreMemoryGetHostPointer(POLVCoreMemory *addr);

/**
 * @brief Copies memory from host to device
 * 
 * @param src        the source host address
 * @param src_offset offset to src
 * @param dst        the destination device address
 * @param dst_offset offset to dst
 * @param size       the number of bytes to copy
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreMemoryCopyH2D(const void *src, size_t src_offset,
                                                   POLVCoreMemory *dst, size_t dst_offset,
                                                   size_t size);

/**
 * @brief Copies memory from device to host
 * 
 * @param src        the source device address
 * @param src_offset offset to src
 * @param dst        the destination host address
 * @param dst_offset offset to dst
 * @param size       the number of bytes to copy
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreMemoryCopyD2H(POLVCoreMemory *src, size_t src_offset,
                                                   void *dst, size_t dst_offset,
                                                   size_t size);

/**
 * @brief Copies memory from device to device
 * 
 * @param src        the source device address
 * @param src_offset offset to src
 * @param dst        the destination device address
 * @param dst_offset offset to dst
 * @param size       the number of bytes to copy
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreMemoryCopyD2D(POLVCoreMemory *src, size_t src_offset,
                                                   POLVCoreMemory *dst, size_t dst_offset,
                                                   size_t size);

/**
 * @brief Copies memory from host to host
 * 
 * @param src        the source host memory
 * @param src_offset offset to src
 * @param dst        the destination host memory
 * @param dst_offset offset to dst
 * @param size       the number of bytes to be copied
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreMemoryCopyH2H(const void *src, size_t src_offset,
                                                   void *dst, size_t dst_offset,
                                                   size_t size);


/**************************************************************
 *                                                            *
 * KERNELS                                                    *
 *                                                            *
 **************************************************************/

/**
 * @brief Creates a kernel on the current context
 * 
 * @param kernel          (ret) the created kernel
 * @param shader_filename the shader filename
 * @param nargs           the number of kernel arguments
 * @return                POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreKernelCreate(POLVCoreKernel **kernel, const char *shader_filename, 
                                                  int nargs);

/**
 * @brief Destroys a kernel
 * 
 * @param kernel the kernel to destroy
 */
POLV_CORE_API void polvCoreKernelDestroy(POLVCoreKernel **kernel);

/**
 * @brief Launches a kernel using the specified grid and group dimensions
 * 
 * @param kernel  the kernel
 * @param args    the kernel arguments
 * @param grid_x  the grid x dimension
 * @param grid_y  the grid x dimension
 * @param grid_z  the grid z dimension
 * @param group_x the group x dimensions
 * @param group_y the group y dimensions
 * @param group_z the group z dimensions
 * @return POLV_CORE_SUCCESS on success, or POLV_CORE_ERROR_* otherwise
 */
POLV_CORE_API POLVCoreResult polvCoreKernelLaunch(POLVCoreKernel *kernel, void **args,
                                                  uint32_t grid_x, uint32_t grid_y, uint32_t grid_z,
                                                  uint32_t group_x, uint32_t group_y, uint32_t group_z);


/**************************************************************
 *                                                            *
 * MISCELLANEOUS                                              *
 *                                                            *
 **************************************************************/

/**
 * @brief Returns a message according to the specified status ID
 * 
 * @param status the status ID
 * @return the status message
 */
POLV_CORE_API const char *polvCoreStatus(POLVCoreResult status);

#ifdef __cplusplus
}
#endif

#endif /* POLV_CORE_H */
