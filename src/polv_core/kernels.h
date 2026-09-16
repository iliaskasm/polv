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
 * POLV Core kernel helpers
 */
#ifndef POLV_CORE_KERNELS_H
#define POLV_CORE_KERNELS_H

#include "polv_core.h"

#ifdef __cplusplus
extern "C"
{
#endif

int  polvc_kernels_shader_new(POLVCoreDevice *dev, const char *shader_filename);
void polvc_kernels_shader_destroy(POLVCoreDevice *dev, int shader_id);
void polvc_kernels_link(POLVCoreContext *context, POLVCoreKernel *kernel);
void polvc_kernels_destroy(POLVCoreKernel *kernel, int unlink);
POLVCoreResult polvc_kernels_create_descriptor_set_layout(POLVCoreKernel *kernel);
POLVCoreResult polvc_kernels_create_pipeline_layout(POLVCoreKernel *kernel);
POLVCoreResult polvc_kernels_create_descriptor_pool(POLVCoreKernel *kernel);
POLVCoreResult polvc_kernels_allocate_descriptor_set(POLVCoreKernel *kernel);
POLVCoreResult polvc_kernels_get_pipeline(POLVCoreKernel *kernel,
                                              uint32_t group_x,
                                              uint32_t group_y,
                                              uint32_t group_z,
                                              VkPipeline *compute_pipeline);

#ifdef __cplusplus
}
#endif

#endif /* POLV_CORE_KERNELS_H */
