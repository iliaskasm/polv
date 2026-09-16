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
 * POLV API headers
 */
#ifndef POLV_H
#define POLV_H

#include <stddef.h>
#include <stdint.h>

#include "polv_version.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef POLV_API
#define POLV_API extern
#endif

typedef enum POLVResult_
{
	POLV_SUCCESS = 0,
	POLV_ERROR = -1,
	POLV_ERROR_NOT_INITIALIZED = -2,
	POLV_ERROR_INVALID_ARGUMENT = -3,
	POLV_ERROR_NO_DEVICE = -4,
	POLV_ERROR_VULKAN = -5,
	POLV_ERROR_OUT_OF_MEMORY = -6,
	POLV_ERROR_SHADER = -7,
	POLV_ERROR_UNSUPPORTED = -8,
	POLV_ERROR_DEVICE_ID_OUT_OF_BOUNDS = -9,
	POLV_ERROR_CONTEXT_MISMATCH = -10,
	POLV_ERROR_CONTEXT_NOT_INITIALIZED = -11
} POLVResult;

#define POLV_DEVICE_NAME_SIZE 256

typedef enum POLVDeviceType_
{
	polvDeviceOther = 0,
	polvDeviceIntegratedGPU,
	polvDeviceDiscreteGPU,
	polvDeviceVirtualGPU,
	polvDeviceCPU
} POLVDeviceType;

typedef struct POLVDeviceInfo_
{
	int            id;
	char           name[POLV_DEVICE_NAME_SIZE];
	uint32_t       vendor_id;
	uint32_t       device_id;
	uint32_t       api_version;
	uint32_t       api_version_major;
	uint32_t       api_version_minor;
	uint32_t       api_version_patch;
	uint32_t       driver_version;
	POLVDeviceType type;
	uint64_t       device_local_memory_bytes;
	uint32_t       compute_queue_family;
	uint32_t       max_compute_work_group_count[3];
	uint32_t       max_compute_work_group_size[3];
	uint32_t       max_compute_work_group_invocations;
	uint32_t       max_storage_buffer_range;
} POLVDeviceInfo;

typedef enum POLVMemoryType_
{
	polvMemDeviceLocal = 0,
	polvMemHostVisible = 1,
	polvMemHostCoherent = 2
} POLVMemoryType;

typedef struct POLVDim_
{
	uint32_t x;
	uint32_t y;
	uint32_t z;
} POLVDim;

typedef enum POLVMemcpyDirection_
{
	polvHostToDevice = 0,
	polvDeviceToHost,
	polvDeviceToDevice,
	polvHostToHost
} POLVMemcpyDirection;

/**************************************************************
 *                                                            *
 * INIT/FINALIZE                                              *
 *                                                            *
 **************************************************************/

/**
 * @brief Initializes the POLV API
 * 
 * @return POLV_SUCCESS on success
 */
POLV_API POLVResult polvInit(void);

/**
 * @brief Finalizes the POLV API
 * 
 */
POLV_API void polvFinalize(void);

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
POLV_API int polvGetNumDevices(void);

/**
 * @brief Sets current device ID
 * 
 * @param device_id the device ID
 * @return POLV_SUCCESS on success, or POLV_ERROR_* otherwise
 */
POLV_API POLVResult polvSetDevice(int device_id);

/**
 * @brief Returns the ID of the current Vulkan device
 * 
 * @return the ID of the current Vulkan device 
 */
POLV_API int polvGetCurrentDeviceId(void);

/**
 * @brief Returns information about a Vulkan compute device
 *
 * @param device_id the POLV device ID
 * @param info      (ret) device information
 * @return POLV_SUCCESS on success, or POLV_ERROR_* otherwise
 */
POLV_API POLVResult polvGetDeviceInfo(int device_id, POLVDeviceInfo *info);

/**
 * @brief Returns a human-readable device-type name
 *
 * @param type the POLV device type
 * @return a static string describing the device type
 */
POLV_API const char *polvDeviceTypeName(POLVDeviceType type);

/**************************************************************
 *                                                            *
 * MEMORY                                                     *
 *                                                            *
 **************************************************************/

/**
 * @brief Allocates memory of specific size and type
 * 
 * @param size     the number of bytes to allocate
 * @param map_type the mapping type
 * @return the allocated memory, or NULL if allocation fails
 */
POLV_API void *polvAlloc(size_t size, POLVMemoryType map_type);

/**
 * @brief Frees data allocated in a specific address
 * 
 * @param addr the memory address
 */
POLV_API void polvFree(void *addr);

/**
 * @brief Returns the host pointer for a host-coherent allocation
 *
 * @param addr the POLV allocation
 * @return the host pointer, or NULL if the allocation is not host coherent
 */
POLV_API void *polvGetHostPointer(void *addr);

/**
 *
 * @brief Performs a memory copy in a given direction.
 *
 * @param src        the source memory
 * @param src_offset the offset from the source memory
 * @param dst        the destination memory
 * @param dst_offset the offset from the destination memory
 * @param size       the number of bytes to copy
 * @param direction  the direction of the memory copy
 * @return POLV_SUCCESS on success, or POLV_ERROR_* otherwise
 */
POLV_API POLVResult polvMemcpy(const void *src, size_t src_offset, void *dst, size_t dst_offset,
                               size_t size, POLVMemcpyDirection direction);

/**************************************************************
 *                                                            *
 * KERNELS                                                    *
 *                                                            *
 **************************************************************/

/**
 * @brief Launches a kernel (from disk)
 * 
 * @param shader_filename the kernel filename
 * @param args            the kernel arguments
 * @param nargs           the number of kernel arguments
 * @param grid            the grid dimensions
 * @param group           the group dimensions
 * @return POLV_SUCCESS on success, or POLV_ERROR_* otherwise
 */
POLV_API POLVResult polvKernelLaunch(const char *shader_filename, void **args, int nargs,
                                     POLVDim grid, POLVDim group);

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
POLV_API const char *polvStatus(POLVResult status);

#ifdef __cplusplus
}
#endif

#endif /* POLV_H */
