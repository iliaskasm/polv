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
 * POLV context management (internal)
 */
#include <stdlib.h>
#include "contexts.h"
#include "polv_internal.h"

typedef struct polv_contexts_entry_
{
	POLVCoreDevice  *dev;
	POLVCoreContext *context;
} polv_contexts_entry;

static polv_contexts_entry *contexts = NULL;
static int num_contexts = 0;
static int selected_device = -1;

/* Returns a specific context given the device it belongs to. */
static polv_contexts_entry *polv_contexts_get_entry(POLVCoreDevice *dev)
{
	int i;

	if (!dev)
		return NULL;

	for (i = 0; i < num_contexts; ++i)
		if (contexts[i].dev == dev)
			return &contexts[i];

	return NULL;
}


/* Creates a context for a specific device and adds it to the pool. */
static POLVResult polv_contexts_create(POLVCoreDevice *dev, POLVCoreContext **context)
{
	polv_contexts_entry *new_contexts;
	POLVCoreContext *ctx;
	POLVCoreResult res;

	if (!dev || !context)
		return POLV_ERROR_INVALID_ARGUMENT;

	ctx = NULL;

	/* Create the context, grow contexts array if necessary */
	if ((res = polvCoreContextCreate(&ctx, dev)) != POLV_CORE_SUCCESS)
		return polv_status_from_core(res);

	new_contexts = (polv_contexts_entry *) realloc(
		contexts, (size_t)(num_contexts + 1) * sizeof(*contexts));
	if (!new_contexts)
	{
		polvCoreContextDestroy(&ctx);
		return POLV_ERROR_OUT_OF_MEMORY;
	}

	/* Add to array */
	contexts = new_contexts;
	contexts[num_contexts].dev = dev;
	contexts[num_contexts].context = ctx;

	num_contexts++;
	*context = ctx;

	return POLV_SUCCESS;
}


/* Initializes context bookkeeping. */
POLVResult polv_contexts_init(void)
{
	contexts = NULL;
	num_contexts = 0;
	selected_device = -1;

	return polv_contexts_set_device(0);
}


/* Finalizes context bookkeeping. */
void polv_contexts_finalize(void)
{
	int i;

	for (i = 0; i < num_contexts; ++i)
		polvCoreContextDestroy(&contexts[i].context);

	free(contexts);

	contexts = NULL;
	num_contexts = 0;
	selected_device = -1;
}


/* Sets the current device (and context) to a specific one, given its ID. */
POLVResult polv_contexts_set_device(int device_id)
{
	polv_contexts_entry *entry;
	POLVCoreDevice *dev;
	POLVCoreContext *context;
	POLVCoreResult core_res;
	POLVResult res;
	int ndev;

	ndev = polvCoreGetNumDevices();

	if (device_id < 0 || device_id >= ndev)
		return POLV_ERROR_DEVICE_ID_OUT_OF_BOUNDS;

	if ((dev = polvCoreGetDevice(device_id)) == NULL)
		return POLV_ERROR_DEVICE_ID_OUT_OF_BOUNDS;

	if ((entry = polv_contexts_get_entry(dev)))
		context = entry->context;
	else
		if ((res = polv_contexts_create(dev, &context)) != POLV_SUCCESS)
			return res;

	if ((core_res = polvCoreContextSetCurrent(context)) != POLV_CORE_SUCCESS)
		return polv_status_from_core(core_res);

	selected_device = device_id;

	return POLV_SUCCESS;
}


/* Returns the current device ID. */
int polv_contexts_get_current_device_id(void)
{
	return selected_device;
}


/* Returns the current context (retrieved from the current device). */
POLVCoreContext *polv_contexts_get_current_context(void)
{
	polv_contexts_entry *entry;
	POLVCoreDevice *dev;

	if (selected_device < 0)
		return NULL;

	if ((dev = polvCoreGetDevice(selected_device)) == NULL)
		return NULL;

	if ((entry = polv_contexts_get_entry(dev)) == NULL)
		return NULL;

	return entry->context;
}
