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
 * POLV API internal helpers
 */
#ifndef POLV_INTERNAL_H
#define POLV_INTERNAL_H

#include "polv.h"
#include "polv_core.h"

static inline POLVResult polv_status_from_core(POLVCoreResult status)
{
	switch (status)
	{
		case POLV_CORE_SUCCESS:
			return POLV_SUCCESS;
		case POLV_CORE_ERROR_NOT_INITIALIZED:
			return POLV_ERROR_NOT_INITIALIZED;
		case POLV_CORE_ERROR_INVALID_ARGUMENT:
			return POLV_ERROR_INVALID_ARGUMENT;
		case POLV_CORE_ERROR_NO_DEVICE:
			return POLV_ERROR_NO_DEVICE;
		case POLV_CORE_ERROR_VULKAN:
			return POLV_ERROR_VULKAN;
		case POLV_CORE_ERROR_OUT_OF_MEMORY:
			return POLV_ERROR_OUT_OF_MEMORY;
		case POLV_CORE_ERROR_SHADER:
			return POLV_ERROR_SHADER;
		case POLV_CORE_ERROR_UNSUPPORTED:
			return POLV_ERROR_UNSUPPORTED;
		case POLV_CORE_ERROR_DEVICE_ID_OUT_OF_BOUNDS:
			return POLV_ERROR_DEVICE_ID_OUT_OF_BOUNDS;
		case POLV_CORE_ERROR_CONTEXT_MISMATCH:
			return POLV_ERROR_CONTEXT_MISMATCH;
		case POLV_CORE_ERROR_CONTEXT_NOT_INITIALIZED:
			return POLV_ERROR_CONTEXT_NOT_INITIALIZED;
		case POLV_CORE_ERROR:
		default:
			return POLV_ERROR;
	}
}

#endif /* POLV_INTERNAL_H */
