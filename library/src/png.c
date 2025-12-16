/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2025 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl43.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include <colopresso.h>

#include "internal/log.h"
#include "internal/png.h"

typedef struct {
  const uint8_t *data;
  size_t size;
  size_t pos;
} png_memory_reader_t;

static void png_read_from_memory(png_structp png_ptr, png_bytep data, png_size_t length) {
  png_memory_reader_t *reader;

  reader = (png_memory_reader_t *)png_get_io_ptr(png_ptr);
  if (reader->pos + length > reader->size) {
    cpres_log(CPRES_LOG_LEVEL_ERROR, "Attempted to read past end of PNG data");
    png_error(png_ptr, "Read past end of data");
    return;
  }

  memcpy(data, reader->data + reader->pos, length);

  reader->pos += length;
}

static inline cpres_error_t read_png_common(png_structp png, png_infop info, uint8_t **rgba_data, png_uint_32 *width, png_uint_32 *height) {
  png_byte color_type, bit_depth;
  png_bytep *row_pointers;
  png_uint_32 y;
  size_t row_bytes, total_size;

  png_read_info(png, info);

  *width = png_get_image_width(png, info);
  *height = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth = png_get_bit_depth(png, info);

  if (*width <= 0 || *height <= 0 || *width > 65536 || *height > 65536) {
    return CPRES_ERROR_INVALID_PNG;
  }

  if (bit_depth == 16) {
    png_set_strip_16(png);
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png);
  }

  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand_gray_1_2_4_to_8(png);
  }

  if (png_get_valid(png, info, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png);
  }

  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
  }

  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png);
  }

  png_read_update_info(png, info);

  row_bytes = png_get_rowbytes(png, info);

  if (*height > 0 && row_bytes > SIZE_MAX / (*height)) {
    cpres_log(CPRES_LOG_LEVEL_ERROR, "Integer overflow detected: image too large");
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  total_size = row_bytes * (*height);

  *rgba_data = (uint8_t *)malloc(total_size);
  if (!*rgba_data) {
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * (*height));
  if (!row_pointers) {
    free(*rgba_data);
    *rgba_data = NULL;
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  for (y = 0; y < *height; y++) {
    row_pointers[y] = *rgba_data + y * row_bytes;
  }

  png_read_image(png, row_pointers);

  free(row_pointers);

  return CPRES_OK;
}

extern cpres_error_t cpres_png_decode_from_memory(const uint8_t *png_data, size_t png_size, uint8_t **rgba_data, png_uint_32 *width, png_uint_32 *height) {
  png_structp png;
  png_infop info;
  png_memory_reader_t reader = {0};
  cpres_error_t result;

  if (!png_data || png_size < 8) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  if (png_sig_cmp(png_data, 0, 8) != 0) {
    return CPRES_ERROR_INVALID_PNG;
  }

  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, NULL, NULL);
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    return CPRES_ERROR_INVALID_PNG;
  }

  reader.data = png_data;
  reader.size = png_size;
  reader.pos = 0;
  png_set_read_fn(png, &reader, png_read_from_memory);

  result = read_png_common(png, info, rgba_data, width, height);

  png_destroy_read_struct(&png, &info, NULL);

  return result;
}

#if COLOPRESSO_WITH_FILE_OPS
extern cpres_error_t cpres_png_decode_from_file(const char *filename, uint8_t **rgba_data, png_uint_32 *width, png_uint_32 *height) {
  FILE *fp;
  png_structp png;
  png_infop info;
  cpres_error_t result;

  fp = fopen(filename, "rb");
  if (!fp) {
    return CPRES_ERROR_FILE_NOT_FOUND;
  }

  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    fclose(fp);
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  info = png_create_info_struct(png);
  if (!info) {
    png_destroy_read_struct(&png, NULL, NULL);
    fclose(fp);
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return CPRES_ERROR_INVALID_PNG;
  }

  png_init_io(png, fp);

  result = read_png_common(png, info, rgba_data, width, height);

  png_destroy_read_struct(&png, &info, NULL);
  fclose(fp);

  return result;
}

#endif /* COLOPRESSO_WITH_FILE_OPS */
