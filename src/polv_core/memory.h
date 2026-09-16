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
 * POLV Core memory helpers
 */
#ifndef POLV_CORE_MEMORY_H
#define POLV_CORE_MEMORY_H

#include "polv_core.h"

#ifdef __cplusplus
extern "C"
{
#endif

POLVCoreMemory *polvc_memory_allocate(POLVCoreContext *context, VkDeviceSize size,
                                      VkMemoryPropertyFlags required,
                                      VkMemoryPropertyFlags preferred,
                                      int track);

POLVCoreResult polvc_memory_alloc_staging(POLVCoreContext *context, POLVCoreMemory **slot,
                                          VkDeviceSize size, VkMemoryPropertyFlags preferred,
                                          POLVCoreMemory **result);

void           polvc_memory_destroy(POLVCoreMemory *ptr, int unlink);

POLVCoreResult polvc_memory_map_write(POLVCoreMemory *dst, size_t offset,
                                      const void *src, size_t size);
POLVCoreResult polvc_memory_map_read(POLVCoreMemory *src, size_t offset,
                                     void *dst, size_t size);

POLVCoreResult polvc_memory_submit_buffer_copy(POLVCoreContext *context,
                                               VkBuffer src, VkBuffer dst,
                                               VkDeviceSize src_offset,
                                               VkDeviceSize dst_offset,
                                               VkDeviceSize size);

#ifdef __cplusplus
}
#endif

#endif /* POLV_CORE_MEMORY_H */
