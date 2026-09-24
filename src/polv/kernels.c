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
 * POLV kernel management (internal)
 */
#include <stdlib.h>
#include <string.h>
#include "kernels.h"
#include "polv_internal.h"

typedef struct polv_kernels_entry_
{
	POLVCoreContext             *context;
	ShaderType                   shadertype;
	char                        *filename;
	char                        *shader;
	int                          nargs;
	POLVCoreKernel              *kernel;
	struct polv_kernels_entry_  *next;
} polv_kernels_entry;

static polv_kernels_entry *kernels = NULL;


/* Initializes kernel bookkeeping. */
void polv_kernels_init(void)
{
	kernels = NULL;
}


/* Finalizes kernel bookkeeping. */
void polv_kernels_finalize(void)
{
	polv_kernels_entry *entry, *next;

	entry = kernels;

	while (entry)
	{
		next = entry->next;

		polvCoreKernelDestroy(&entry->kernel);
		free(entry->filename);
		free(entry->shader);
		free(entry);

		entry = next;
	}

	kernels = NULL;
}


/* 
 * Retrieves or creates a kernel for a specific context, shader
 * and arguments.
 * 
 * The filename parameter is used when shadertype != SHADER_STRING.
 * shader parameter is used when shadertype == SHADER_STRING.
 * 
 * Important detail: this cache relies solely on the shader filename 
 * when the input is a GLSL/binary file. This means that if the file 
 * is updated during runtime, POLV will still see the old file.
 * 
 * This is not the case for string inputs, as the string is compared
 * on the fly.
 */
POLVResult polv_kernels_get_or_create(POLVCoreContext *context, const char *filename,
                                      const char *shader, int nargs, POLVCoreKernel **kernel, 
                                      ShaderType shadertype)
{
	polv_kernels_entry *entry;
	POLVCoreResult res;

	if (!context || !kernel || nargs <= 0)
		return POLV_ERROR_INVALID_ARGUMENT;

	switch (shadertype)
	{
		case SHADER_SPV:
		case SHADER_SOURCE:
			if (!filename)
				return POLV_ERROR_INVALID_ARGUMENT;
			break;

		case SHADER_STRING:
			if (!shader)
				return POLV_ERROR_INVALID_ARGUMENT;
			break;

		default:
			return POLV_ERROR_INVALID_ARGUMENT;
	}

	*kernel = NULL;

	/* (1) Search the kernel pool */
	for (entry = kernels; entry; entry = entry->next)
	{
		if (entry->context != context ||
			entry->nargs != nargs ||
			entry->shadertype != shadertype)
			continue;

		if (shadertype == SHADER_STRING)
		{
			// Cache hit
			if (entry->shader && shader && 
				strcmp(entry->shader, shader) == 0)
			{
				*kernel = entry->kernel;
				return POLV_SUCCESS;
			}
		}
		else
		{
			// Cache hit
			if (entry->filename && filename &&
				strcmp(entry->filename, filename) == 0)
			{
				*kernel = entry->kernel;
				return POLV_SUCCESS;
			}
		}
	}

	// Cache miss; set the current context to the given one.
	if ((res = polvCoreContextSetCurrent(context)) != POLV_CORE_SUCCESS)
		return polv_status_from_core(res);

	/* (2) Create the kernel */
	switch (shadertype)
	{
		case SHADER_SPV:
			res = polvCoreKernelCreate(kernel, filename, nargs);
			break;
		case SHADER_SOURCE:
			res = polvCoreKernelCreateFromGLSL(kernel, filename, nargs);
			break;
		case SHADER_STRING:
			res = polvCoreKernelCreateFromString(kernel, shader, nargs);
			break;
		default:
			return POLV_ERROR_INVALID_ARGUMENT;
	}

	if (res != POLV_CORE_SUCCESS)
		return polv_status_from_core(res);

	entry = (polv_kernels_entry *) calloc(1, sizeof(*entry));
	if (!entry)
	{
		polvCoreKernelDestroy(kernel);
		return POLV_ERROR_OUT_OF_MEMORY;
	}

	entry->shadertype = shadertype;

	// Store shader contents if shader is passed as a string
	if (shadertype == SHADER_STRING)
	{
		entry->shader = strdup(shader);
		if (!entry->shader)
		{
			polvCoreKernelDestroy(kernel);
			free(entry);
			return POLV_ERROR_OUT_OF_MEMORY;
		}
	}
	else // Otherwise, store the filename
	{
		entry->filename = strdup(filename);
		if (!entry->filename)
		{
			polvCoreKernelDestroy(kernel);
			free(entry);
			return POLV_ERROR_OUT_OF_MEMORY;
		}
	}

	entry->context = context;
	entry->nargs = nargs;
	entry->kernel = *kernel;
	entry->next = kernels;
	kernels = entry;

	return POLV_SUCCESS;
}
