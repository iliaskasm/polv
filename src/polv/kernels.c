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
	char                        *filename;
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
		free(entry);

		entry = next;
	}

	kernels = NULL;
}


/* Retrieves or creates a kernel for a specific context, filename and arguments. */
POLVResult polv_kernels_get_or_create(POLVCoreContext *context, const char *filename,
                                      int nargs, POLVCoreKernel **kernel)
{
	polv_kernels_entry *entry;
	POLVCoreResult res;

	if (!context || !filename || !kernel || nargs <= 0)
		return POLV_ERROR_INVALID_ARGUMENT;

	*kernel = NULL;

	/* (1) Search the kernel pool */
	for (entry = kernels; entry; entry = entry->next)
	{
		if (entry->context == context &&
		    entry->nargs == nargs &&
		    strcmp(entry->filename, filename) == 0)
		{
			*kernel = entry->kernel;
			return POLV_SUCCESS;
		}
	}

	// Kernel not found, set the current context to the given one.
	if ((res = polvCoreContextSetCurrent(context)) != POLV_CORE_SUCCESS)
		return polv_status_from_core(res);

	/* (2) Create the kernel */
	if ((res = polvCoreKernelCreate(kernel, filename, nargs)) != POLV_CORE_SUCCESS)
		return polv_status_from_core(res);

	entry = (polv_kernels_entry *) calloc(1, sizeof(*entry));
	if (!entry)
	{
		polvCoreKernelDestroy(kernel);
		return POLV_ERROR_OUT_OF_MEMORY;
	}

	entry->filename = strdup(filename);
	if (!entry->filename)
	{
		polvCoreKernelDestroy(kernel);
		free(entry);
		return POLV_ERROR_OUT_OF_MEMORY;
	}

	entry->context = context;
	entry->nargs = nargs;
	entry->kernel = *kernel;
	entry->next = kernels;
	kernels = entry;

	return POLV_SUCCESS;
}
