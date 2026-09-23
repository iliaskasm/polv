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
 * Sobel edge detection on a 24-bit uncompressed BMP image
 *
 * Usage:
 *   ./sobel_core input.bmp output.bmp
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <polv_core.h>
#include "../../utils/bmp.h"

#define DEVICE_ID 0
#define SOBEL_SHADER "sobel.spv"

#define LOCAL_SIZE_X 16u
#define LOCAL_SIZE_Y 16u

typedef struct {
	uint32_t width;
	uint32_t height;
} Shape;

typedef struct {
	int initialized;
	POLVCoreContext *context;
	POLVCoreKernel *kernel;
	POLVCoreMemory *device_input;
	POLVCoreMemory *device_output;
	POLVCoreMemory *shape;
} POLVCoreState;


static int polv_check(POLVCoreResult result, const char *operation)
{
	if (result == POLV_CORE_SUCCESS)
		return 1;

	fprintf(stderr, "%s failed: %s\n", operation, polvCoreStatus(result));

	return 0;
}


static void polv_core_cleanup(POLVCoreState *state)
{
	if (!state->initialized)
		return;

	polvCoreMemoryFree(state->device_input);
	polvCoreMemoryFree(state->device_output);
	polvCoreMemoryFree(state->shape);
	polvCoreKernelDestroy(&state->kernel);
	polvCoreContextDestroy(&state->context);
	polvCoreFinalize();

	state->device_input = NULL;
	state->device_output = NULL;
	state->shape = NULL;
	state->initialized = 0;
}


int main(int argc, char **argv)
{
	Image input = { 0 }, output = { 0 };
	POLVCoreState state = { 0 };
	Shape *shape;
	void *args[3];
	size_t pixel_count, bytes;
	int success = 0;
	POLVCoreDevice *device;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <input.bmp> <output.bmp>\n", argv[0]);
		return EXIT_FAILURE;
	}

	if (!bmp_read(argv[1], &input))
		return EXIT_FAILURE;

	do
	{
		pixel_count = (size_t) input.width * input.height;
		bytes = pixel_count * sizeof(uint32_t);

		output.width = input.width;
		output.height = input.height;
		output.pixels = malloc(bytes);

		if (!output.pixels)
		{
			fprintf(stderr, "Could not allocate output image\n");
			break;
		}

		// Initialize POLV Core
		if (!polv_check(polvCoreInit(), "polvCoreInit"))
			break;

		// Get device
		if ((device = polvCoreGetDevice(DEVICE_ID)) == NULL)
			break;

		if (!polv_check(polvCoreContextCreate(&state.context, device), "polvCoreContextCreate"))
			break;

		state.initialized = 1;
		state.device_input = polvCoreMemoryAllocDeviceLocal(bytes);
		state.device_output = polvCoreMemoryAllocDeviceLocal(bytes);
		state.shape = polvCoreMemoryAllocHostCoherent(sizeof(Shape));

		if (!state.device_input || !state.device_output || !state.shape)
		{
			fprintf(stderr, "POLV Core memory allocation failed\n");
			break;
		}

		shape = polvCoreMemoryGetHostPointer(state.shape);
		if (!shape)
		{
			fprintf(stderr, "Could not map shape buffer\n");
			break;
		}

		shape->width = input.width;
		shape->height = input.height;

		// HOST input -> DEVICE input
		if (!polv_check(polvCoreMemoryCopyH2D(input.pixels, 0, state.device_input, 0, bytes), 
		                "input H2D copy"))
			break;

		// Kernel arguments: input, output, shape
		args[0] = state.device_input;
		args[1] = state.device_output;
		args[2] = state.shape;

		uint32_t group[3] = { LOCAL_SIZE_X, LOCAL_SIZE_Y, 1u };
		uint32_t grid[3] = {
			(input.width + LOCAL_SIZE_X - 1u) / LOCAL_SIZE_X,
			(input.height + LOCAL_SIZE_Y - 1u) / LOCAL_SIZE_Y,
			1u
		};

		if (!polv_check(polvCoreKernelCreate(&state.kernel, SOBEL_SHADER, 3), 
		                "Sobel kernel creation"))
			break;

		if (!polv_check(polvCoreKernelLaunch(state.kernel, args, grid[0], grid[1], grid[2], 
		                                     group[0], group[1], group[2]), 
		                "Sobel kernel launch"))
			break;

		// DEVICE output -> HOST output
		if (!polv_check(polvCoreMemoryCopyD2H(state.device_output, 0, output.pixels, 0, bytes), 
		                "output D2H copy"))
			break;

		if (!bmp_write(argv[2], &output))
			break;

		printf("Sobel filter complete: %s -> %s (%ux%u)\n", argv[1], argv[2], input.width, input.height);
		success = 1;

	} while (0);

	polv_core_cleanup(&state);
	bmp_image_free(&output);
	bmp_image_free(&input);

	return success ? EXIT_SUCCESS : EXIT_FAILURE;
}