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
 * BMP read/write functionality
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmp.h"

static uint16_t bmp_get_u16_le(const unsigned char *p)
{
	return (uint16_t) p[0] | ((uint16_t) p[1] << 8);
}


static uint32_t bmp_get_u32_le(const unsigned char *p)
{
	return (uint32_t) p[0] | ((uint32_t) p[1] << 8) | ((uint32_t) p[2] << 16) | ((uint32_t) p[3] << 24);
}


static int32_t bmp_get_i32_le(const unsigned char *p)
{
	return (int32_t) bmp_get_u32_le(p);
}


static void bmp_put_u16_le(unsigned char *p, uint16_t value)
{
	p[0] = (unsigned char) (value & 0xffu);
	p[1] = (unsigned char) ((value >> 8) & 0xffu);
}


static void bmp_put_u32_le(unsigned char *p, uint32_t value)
{
	p[0] = (unsigned char) (value & 0xffu);
	p[1] = (unsigned char) ((value >> 8) & 0xffu);
	p[2] = (unsigned char) ((value >> 16) & 0xffu);
	p[3] = (unsigned char) ((value >> 24) & 0xffu);
}


static void bmp_put_i32_le(unsigned char *p, int32_t value)
{
	bmp_put_u32_le(p, (uint32_t) value);
}


static int bmp_read_exact(FILE *file, void *data, size_t size)
{
	return fread(data, 1, size, file) == size;
}


static int bmp_write_exact(FILE *file, const void *data, size_t size)
{
	return fwrite(data, 1, size, file) == size;
}


void bmp_image_free(Image *image)
{
	if (!image)
		return;

	free(image->pixels);
	image->pixels = NULL;
	image->width = 0;
	image->height = 0;
}


int bmp_read(const char *path, Image *image)
{
	unsigned char file_header[14], info_header[40], *row = NULL;
	FILE *file = NULL;
	uint32_t pixel_offset, dib_size, compression, width, height, y;
	uint16_t planes, bits_per_pixel;
	int32_t signed_width, signed_height;
	size_t pixel_count, row_stride;
	int top_down, ok = 0;

	if (!path || !image)
		return 0;

	memset(image, 0, sizeof(*image));

	file = fopen(path, "rb");
	if (!file)
		return 0;

	if (!bmp_read_exact(file, file_header, sizeof(file_header)) || 
		!bmp_read_exact(file, info_header, sizeof(info_header)))
		goto done;

	if (file_header[0] != 'B' || file_header[1] != 'M')
		goto done;

	pixel_offset = bmp_get_u32_le(file_header + 10);
	dib_size = bmp_get_u32_le(info_header);
	signed_width = bmp_get_i32_le(info_header + 4);
	signed_height = bmp_get_i32_le(info_header + 8);
	planes = bmp_get_u16_le(info_header + 12);
	bits_per_pixel = bmp_get_u16_le(info_header + 14);
	compression = bmp_get_u32_le(info_header + 16);

	if (dib_size < 40 || signed_width <= 0 || signed_height == 0)
		goto done;

	if (planes != 1 || bits_per_pixel != 24 || compression != 0)
		goto done;

	width = (uint32_t) signed_width;
	height = signed_height < 0 ? (uint32_t) (-(int64_t) signed_height) : (uint32_t) signed_height;
	top_down = signed_height < 0;

	if ((size_t) width > SIZE_MAX / (size_t) height)
		goto done;

	pixel_count = (size_t) width * height;
	if (pixel_count > SIZE_MAX / sizeof(uint32_t))
		goto done;

	{
		uint64_t stride = (((uint64_t) width * 3u) + 3u) & ~(uint64_t) 3u;

		if (stride > SIZE_MAX)
			goto done;

		row_stride = (size_t) stride;
	}

	if ((uint64_t) pixel_offset < 14u + (uint64_t) dib_size)
		goto done;

#if LONG_MAX < UINT32_MAX
	if (pixel_offset > (uint32_t) LONG_MAX)
		goto done;
#endif

	image->pixels = malloc(pixel_count * sizeof(*image->pixels));
	row = malloc(row_stride);

	if (!image->pixels || !row)
		goto done;

	if (fseek(file, (long) pixel_offset, SEEK_SET) != 0)
		goto done;

	for (y = 0; y < height; y++) {
		uint32_t dst_y = top_down ? y : height - 1u - y, x;

		if (!bmp_read_exact(file, row, row_stride))
			break;

		for (x = 0; x < width; x++) {
			const unsigned char *src = row + (size_t) x * 3u;
			uint32_t b = src[0], g = src[1], r = src[2];

			image->pixels[(size_t) dst_y * width + x] = r | (g << 8) | (b << 16);
		}
	}

	if (y != height)
		goto done;

	image->width = width;
	image->height = height;
	ok = 1;

done:
	free(row);
	fclose(file);

	if (!ok)
		bmp_image_free(image);

	return ok;
}

int bmp_write(const char *path, const Image *image)
{
	unsigned char file_header[14] = { 0 }, info_header[40] = { 0 }, *row = NULL;
	FILE *file = NULL;
	size_t row_stride;
	uint64_t image_size, file_size;
	uint32_t file_y;
	int ok = 0;

	if (!path || !image || !image->pixels || !image->width || !image->height)
		return 0;

	if (image->width > (uint32_t) INT32_MAX || image->height > (uint32_t) INT32_MAX)
		return 0;

	{
		uint64_t stride = (((uint64_t) image->width * 3u) + 3u) & ~(uint64_t) 3u;

		if (stride > SIZE_MAX)
			return 0;

		row_stride = (size_t) stride;
	}

	image_size = (uint64_t) row_stride * image->height;
	file_size = 14u + 40u + image_size;

	if (image_size > UINT32_MAX || file_size > UINT32_MAX)
		return 0;

	row = calloc(1, row_stride);
	if (!row)
		return 0;

	file = fopen(path, "wb");
	if (!file) {
		free(row);
		return 0;
	}

	file_header[0] = 'B';
	file_header[1] = 'M';
	bmp_put_u32_le(file_header + 2, (uint32_t) file_size);
	bmp_put_u32_le(file_header + 10, 54u);

	bmp_put_u32_le(info_header, 40u);
	bmp_put_i32_le(info_header + 4, (int32_t) image->width);
	bmp_put_i32_le(info_header + 8, (int32_t) image->height);
	bmp_put_u16_le(info_header + 12, 1u);
	bmp_put_u16_le(info_header + 14, 24u);
	bmp_put_u32_le(info_header + 16, 0u);
	bmp_put_u32_le(info_header + 20, (uint32_t) image_size);

	if (!bmp_write_exact(file, file_header, sizeof(file_header)) || 
		!bmp_write_exact(file, info_header, sizeof(info_header)))
		goto done;

	for (file_y = 0; file_y < image->height; file_y++) {
		uint32_t src_y = image->height - 1u - file_y, x;

		memset(row, 0, row_stride);

		for (x = 0; x < image->width; x++) {
			uint32_t pixel = image->pixels[(size_t) src_y * image->width + x];
			unsigned char *dst = row + (size_t) x * 3u;

			dst[0] = (unsigned char) ((pixel >> 16) & 0xffu);
			dst[1] = (unsigned char) ((pixel >> 8) & 0xffu);
			dst[2] = (unsigned char) (pixel & 0xffu);
		}

		if (!bmp_write_exact(file, row, row_stride))
			break;
	}

	if (file_y == image->height)
		ok = 1;

done:
	if (fclose(file) != 0)
		ok = 0;

	free(row);
	return ok;
}