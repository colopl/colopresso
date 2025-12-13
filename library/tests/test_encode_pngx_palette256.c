/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2025 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include <colopresso.h>

#include <unity.h>

#include "../src/internal/png.h"
#include "../src/internal/pngx.h"

#include "test.h"

typedef struct {
  uint8_t *data;
  size_t size;
  size_t capacity;
} test_png_mem_buffer_t;

static cpres_config_t g_config;
static const uint8_t g_sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

static void png_flush_cb(png_structp png_ptr) { (void)png_ptr; }

static void png_write_cb(png_structp png_ptr, png_bytep data, png_size_t length) {
  size_t required, new_capacity;
  uint8_t *new_data;
  test_png_mem_buffer_t *buf;

  buf = (test_png_mem_buffer_t *)png_get_io_ptr(png_ptr);
  if (!buf || !data || length == 0) {
    return;
  }

  if (buf->size > SIZE_MAX - (size_t)length) {
    png_error(png_ptr, "write overflow");
    return;
  }

  required = buf->size + (size_t)length;
  if (required > buf->capacity) {
    new_capacity = (buf->capacity == 0) ? 1024 : buf->capacity;
    while (new_capacity < required) {
      if (new_capacity > SIZE_MAX / 2) {
        new_capacity = required;
        break;
      }
      new_capacity *= 2;
    }

    new_data = (uint8_t *)realloc(buf->data, new_capacity);
    if (!new_data) {
      png_error(png_ptr, "out of memory");
      return;
    }

    buf->data = new_data;
    buf->capacity = new_capacity;
  }

  memcpy(buf->data + buf->size, data, (size_t)length);
  buf->size += (size_t)length;
}

static inline int32_t parse_png_palette_count(const uint8_t *png, size_t size) {
  size_t off = 8;
  uint32_t len = 0, clen = 0;
  uint8_t color_type = 0, *ctype = NULL;
  int32_t palette_entries = 0, saw_plte = 0, is_indexed = 0;

  if (!png || size < 8) {
    return -1;
  }

  if (memcmp(png, g_sig, 8) != 0) {
    return -1;
  }

  if (off + 25 <= size) {
    len = (png[off] << 24) | (png[off + 1] << 16) | (png[off + 2] << 8) | png[off + 3];
    if (len == 13 && memcmp(png + off + 4, "IHDR", 4) == 0) {
      color_type = png[off + 4 + 8 + 4];
      if (color_type == 3) {
        is_indexed = 1;
      }
    }
  }

  while (off + 8 <= size) {
    clen = (png[off] << 24) | (png[off + 1] << 16) | (png[off + 2] << 8) | (png[off + 3]);
    ctype = (uint8_t *)(png + off + 4);
    off += 8;
    if (off + clen + 4 > size) {
      break;
    }
    if (memcmp(ctype, "PLTE", 4) == 0) {
      saw_plte = 1;
      palette_entries = (int)(clen / 3);
    }
    off += clen + 4;
    if (memcmp(ctype, "IDAT", 4) == 0) {
      break;
    }
  }

  if (!is_indexed || !saw_plte) {
    return -1;
  }

  return palette_entries;
}

static inline void fill_rgba_solid(uint8_t *rgba, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  uint32_t x, y;
  size_t base;

  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      base = (((size_t)y * (size_t)width) + (size_t)x) * 4;
      rgba[base + 0] = r;
      rgba[base + 1] = g;
      rgba[base + 2] = b;
      rgba[base + 3] = a;
    }
  }
}

static inline bool create_png_rgba_memory(const uint8_t *rgba, uint32_t width, uint32_t height, uint8_t **out_png, size_t *out_size) {
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row_pointers;
  uint32_t y;
  size_t rowbytes, base;
  test_png_mem_buffer_t buf;

  if (out_png) {
    *out_png = NULL;
  }
  if (out_size) {
    *out_size = 0;
  }

  if (!rgba || width == 0 || height == 0 || !out_png || !out_size) {
    return false;
  }

  if ((size_t)width > SIZE_MAX / 4 || (size_t)height > SIZE_MAX / (size_t)width) {
    return false;
  }

  rowbytes = (size_t)width * 4;

  memset(&buf, 0, sizeof(buf));
  row_pointers = NULL;

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    return false;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, NULL);
    return false;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(buf.data);
    free(row_pointers);
    return false;
  }

  row_pointers = (png_bytep *)calloc(height, sizeof(png_bytep));
  if (!row_pointers) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return false;
  }

  for (y = 0; y < height; ++y) {
    base = (size_t)y * rowbytes;
    row_pointers[y] = (png_bytep)(rgba + base);
  }

  png_set_write_fn(png_ptr, &buf, png_write_cb, png_flush_cb);

  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr, info_ptr);
  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);
  free(row_pointers);

  if (!buf.data || buf.size == 0) {
    free(buf.data);
    return false;
  }

  *out_png = buf.data;
  *out_size = buf.size;

  return true;
}

void setUp(void) {
  cpres_config_init_defaults(&g_config);
  g_config.pngx_level = 1;
}

void tearDown(void) { release_cached_example_png(); }

void test_pngx_palette256_color_count_verification(void) {
  const uint8_t *png_data = NULL;
  uint8_t *pngx_data = NULL, bit_depth = 0, color_type = 0;
  size_t png_size = 0, pngx_size = 0;
  int max_colors = 128, palette_count = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for palette256 color count test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = max_colors;
  g_config.pngx_lossy_quality_min = 80;
  g_config.pngx_lossy_quality_max = 95;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  bit_depth = pngx_data[24];
  color_type = pngx_data[25];
  TEST_ASSERT_EQUAL_UINT8(8, bit_depth);
  TEST_ASSERT_EQUAL_UINT8(3, color_type);

  palette_count = parse_png_palette_count(pngx_data, pngx_size);
  if (palette_count > 0) {
    TEST_ASSERT_LESS_OR_EQUAL_INT(max_colors, palette_count);
  }

  cpres_free(pngx_data);
}

void test_pngx_palette256_lossy_parameter_clamp_and_palette_reduction(void) {
  const uint8_t *png_data = NULL;
  uint8_t *pngx_data = NULL;
  int32_t orig_palette = 0, opt_palette = 0;
  size_t png_size = 0, pngx_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for palette256 lossy clamp test");

  orig_palette = parse_png_palette_count(png_data, png_size);

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = 999;
  g_config.pngx_lossy_quality_min = 120;
  g_config.pngx_lossy_quality_max = -5;
  g_config.pngx_lossy_speed = 42;
  g_config.pngx_lossy_dither_level = 5.0f;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  opt_palette = parse_png_palette_count(pngx_data, pngx_size);
  if (orig_palette > 0 && opt_palette > 0) {
    TEST_ASSERT_TRUE(opt_palette <= orig_palette);
  }

  cpres_free(pngx_data);
}

void test_pngx_palette256_protected_colors(void) {
  const uint8_t *png_data = NULL;
  bool found_plte = false, found_this_color = false;
  char chunk_type[5] = {0}, msg[256] = {0};
  uint32_t total_pixels = 0, pixel_index = 0, chunk_len = 0, num_palette_entries = 0, j = 0;
  uint8_t *pngx_data = NULL, *rgba_data = NULL, *palette = NULL, bit_depth = 0, color_type = 0, r = 0, g = 0, b = 0, i = 0;
  int32_t protected_colors_found = 0;
  size_t png_size = 0, pngx_size = 0, pixel_offset = 0, chunk_offset = 0;
  png_uint_32 width = 0, height = 0;
  cpres_error_t decode_error = CPRES_OK, error = CPRES_OK;
  cpres_rgba_color_t protected_colors[16];

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL(png_data);

  decode_error = cpres_png_decode_from_memory(png_data, png_size, &rgba_data, &width, &height);
  TEST_ASSERT_EQUAL(CPRES_OK, decode_error);
  TEST_ASSERT_NOT_NULL(rgba_data);
  TEST_ASSERT_GREATER_THAN(0, width);
  TEST_ASSERT_GREATER_THAN(0, height);

  total_pixels = width * height;

  for (i = 0; i < 16; i++) {
    pixel_index = (total_pixels / 17) * (uint32_t)(i + 1);
    if (pixel_index >= total_pixels) {
      pixel_index = total_pixels - 1;
    }

    pixel_offset = (size_t)pixel_index * 4;
    protected_colors[i].r = rgba_data[pixel_offset];
    protected_colors[i].g = rgba_data[pixel_offset + 1];
    protected_colors[i].b = rgba_data[pixel_offset + 2];
    protected_colors[i].a = rgba_data[pixel_offset + 3];
  }

  free(rgba_data);

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = 32;
  g_config.pngx_protected_colors = protected_colors;
  g_config.pngx_protected_colors_count = 16;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN(0, pngx_size);
  TEST_ASSERT_GREATER_THAN(33, pngx_size);

  bit_depth = pngx_data[24];
  color_type = pngx_data[25];

  TEST_ASSERT_TRUE(bit_depth == 4 || bit_depth == 8);
  TEST_ASSERT_EQUAL_UINT8(3, color_type);

  TEST_ASSERT_EQUAL_MEMORY(g_sig, pngx_data, 8);

  chunk_offset = 8;
  found_plte = false;
  protected_colors_found = 0;

  while (chunk_offset + 8 <= pngx_size) {
    chunk_len = ((uint32_t)pngx_data[chunk_offset] << 24) | ((uint32_t)pngx_data[chunk_offset + 1] << 16) | ((uint32_t)pngx_data[chunk_offset + 2] << 8) | ((uint32_t)pngx_data[chunk_offset + 3]);

    if (chunk_offset + 8 + chunk_len > pngx_size) {
      break;
    }

    memcpy(chunk_type, &pngx_data[chunk_offset + 4], 4);

    if (strcmp(chunk_type, "PLTE") == 0) {
      found_plte = true;
      num_palette_entries = chunk_len / 3;
      palette = &pngx_data[chunk_offset + 8];

      for (i = 0; i < 16; i++) {
        found_this_color = false;
        for (j = 0; j < num_palette_entries; j++) {
          r = palette[j * 3];
          g = palette[j * 3 + 1];
          b = palette[j * 3 + 2];

          if (r == protected_colors[i].r && g == protected_colors[i].g && b == protected_colors[i].b) {
            found_this_color = true;
            break;
          }
        }

        if (found_this_color) {
          protected_colors_found++;
        }
      }

      break;
    }

    chunk_offset += 8 + chunk_len + 4;
  }

  TEST_ASSERT_TRUE_MESSAGE(found_plte, "PLTE chunk not found");

  snprintf(msg, sizeof(msg), "Only %d of 16 protected colors found in palette", protected_colors_found);

  TEST_ASSERT_GREATER_OR_EQUAL_MESSAGE(14, protected_colors_found, msg);

  cpres_free(pngx_data);

  g_config.pngx_protected_colors = NULL;
  g_config.pngx_protected_colors_count = 0;
}

void test_pngx_palette256_postprocess_smoothing_with_low_dither(void) {
  uint32_t width = 32, height = 32, x, y;
  uint8_t *rgba = NULL, *png_data = NULL, *pngx_data = NULL;
  size_t base, png_size = 0, pngx_size = 0;
  bool ok;
  cpres_error_t error;

  rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
  TEST_ASSERT_NOT_NULL(rgba);

  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      base = (((size_t)y * (size_t)width) + (size_t)x) * 4;
      rgba[base + 0] = (uint8_t)((x ^ y) & 0xFFu);
      rgba[base + 1] = (uint8_t)((x * 3u) & 0xFFu);
      rgba[base + 2] = (uint8_t)((y * 5u) & 0xFFu);
      rgba[base + 3] = 255;
    }
  }

  ok = create_png_rgba_memory(rgba, width, height, &png_data, &png_size);
  free(rgba);
  TEST_ASSERT_TRUE_MESSAGE(ok, "failed to create png for smoothing test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = 64;
  g_config.pngx_lossy_speed = 10;
  g_config.pngx_lossy_quality_min = 30;
  g_config.pngx_lossy_quality_max = 60;
  g_config.pngx_lossy_dither_level = 0.0f;
  g_config.pngx_postprocess_smooth_enable = true;
  g_config.pngx_postprocess_smooth_importance_cutoff = 2.0f;
  g_config.pngx_palette256_gradient_profile_enable = false;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_TRUE(error == CPRES_OK || error == CPRES_ERROR_OUTPUT_NOT_SMALLER);
  if (error == CPRES_OK) {
    TEST_ASSERT_NOT_NULL(pngx_data);
    TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);
    cpres_free(pngx_data);
  }

  free(png_data);
}

void test_pngx_palette256_postprocess_indices_executes_without_internal_tests(void) {
  png_uint_32 decoded_width = 0, decoded_height = 0;
  uint32_t width = 64, height = 64, x, y, isolated_x = 10, isolated_y = 10;
  uint8_t *rgba = NULL, *png_data = NULL, *out_png = NULL, *decoded = NULL;
  size_t png_size = 0, out_size = 0, base, center, left, right, up, down;
  bool ok, success;
  int quant_quality = -1;
  pngx_options_t opts;
  cpres_error_t decode_error;

  rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
  TEST_ASSERT_NOT_NULL(rgba);

  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      base = (((size_t)y * (size_t)width) + (size_t)x) * 4;

      if (x < width / 2) {
        rgba[base + 0] = 100;
        rgba[base + 1] = 100;
        rgba[base + 2] = 100;
      } else {
        rgba[base + 0] = 110;
        rgba[base + 1] = 100;
        rgba[base + 2] = 100;
      }
      rgba[base + 3] = 255;
    }
  }

  base = (((size_t)isolated_y * (size_t)width) + (size_t)isolated_x) * 4;
  rgba[base + 0] = 110;
  rgba[base + 1] = 100;
  rgba[base + 2] = 100;
  rgba[base + 3] = 255;

  ok = create_png_rgba_memory(rgba, width, height, &png_data, &png_size);
  free(rgba);
  TEST_ASSERT_TRUE_MESSAGE(ok, "failed to create png for postprocess_indices integration test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = 2;
  g_config.pngx_lossy_speed = 1;
  g_config.pngx_lossy_quality_min = 100;
  g_config.pngx_lossy_quality_max = 100;
  g_config.pngx_lossy_dither_level = 0.0f;

  g_config.pngx_postprocess_smooth_enable = true;
  g_config.pngx_postprocess_smooth_importance_cutoff = -1.0f;

  g_config.pngx_palette256_gradient_profile_enable = false;
  g_config.pngx_palette256_alpha_bleed_enable = false;
  g_config.pngx_saliency_map_enable = false;
  g_config.pngx_chroma_anchor_enable = false;
  g_config.pngx_adaptive_dither_enable = false;
  g_config.pngx_gradient_boost_enable = false;
  g_config.pngx_chroma_weight_enable = false;

  memset(&opts, 0, sizeof(opts));
  pngx_fill_pngx_options(&opts, &g_config);

  success = pngx_quantize_palette256(png_data, png_size, &opts, &out_png, &out_size, &quant_quality);
  free(png_data);

  TEST_ASSERT_TRUE_MESSAGE(success, "pngx_quantize_palette256 failed");
  TEST_ASSERT_NOT_NULL(out_png);
  TEST_ASSERT_GREATER_THAN_size_t(0, out_size);

  decode_error = cpres_png_decode_from_memory(out_png, out_size, &decoded, &decoded_width, &decoded_height);
  cpres_free(out_png);
  TEST_ASSERT_EQUAL_INT_MESSAGE(CPRES_OK, decode_error, "failed to decode quantized png");
  TEST_ASSERT_EQUAL_UINT32(width, (uint32_t)decoded_width);
  TEST_ASSERT_EQUAL_UINT32(height, (uint32_t)decoded_height);

  center = (((size_t)isolated_y * (size_t)width) + (size_t)isolated_x) * 4;
  left = (((size_t)isolated_y * (size_t)width) + (size_t)(isolated_x - 1)) * 4;
  right = (((size_t)isolated_y * (size_t)width) + (size_t)(isolated_x + 1)) * 4;
  up = ((((size_t)isolated_y - 1) * (size_t)width) + (size_t)isolated_x) * 4;
  down = ((((size_t)isolated_y + 1) * (size_t)width) + (size_t)isolated_x) * 4;

  TEST_ASSERT_EQUAL_UINT8(decoded[left + 0], decoded[center + 0]);
  TEST_ASSERT_EQUAL_UINT8(decoded[left + 1], decoded[center + 1]);
  TEST_ASSERT_EQUAL_UINT8(decoded[left + 2], decoded[center + 2]);
  TEST_ASSERT_EQUAL_UINT8(decoded[left + 3], decoded[center + 3]);

  TEST_ASSERT_EQUAL_UINT8(decoded[right + 0], decoded[center + 0]);
  TEST_ASSERT_EQUAL_UINT8(decoded[right + 1], decoded[center + 1]);
  TEST_ASSERT_EQUAL_UINT8(decoded[right + 2], decoded[center + 2]);
  TEST_ASSERT_EQUAL_UINT8(decoded[right + 3], decoded[center + 3]);

  TEST_ASSERT_EQUAL_UINT8(decoded[up + 0], decoded[center + 0]);
  TEST_ASSERT_EQUAL_UINT8(decoded[up + 1], decoded[center + 1]);
  TEST_ASSERT_EQUAL_UINT8(decoded[up + 2], decoded[center + 2]);
  TEST_ASSERT_EQUAL_UINT8(decoded[up + 3], decoded[center + 3]);

  TEST_ASSERT_EQUAL_UINT8(decoded[down + 0], decoded[center + 0]);
  TEST_ASSERT_EQUAL_UINT8(decoded[down + 1], decoded[center + 1]);
  TEST_ASSERT_EQUAL_UINT8(decoded[down + 2], decoded[center + 2]);
  TEST_ASSERT_EQUAL_UINT8(decoded[down + 3], decoded[center + 3]);

  cpres_free(decoded);
}

void test_pngx_palette256_alpha_bleed_seedless_transparent_image(void) {
  uint32_t width = 64, height = 64;
  uint8_t *rgba = NULL, *png_data = NULL, *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  bool ok;
  cpres_error_t error;

  rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
  TEST_ASSERT_NOT_NULL(rgba);

  fill_rgba_solid(rgba, width, height, 123, 45, 67, 0);

  ok = create_png_rgba_memory(rgba, width, height, &png_data, &png_size);
  free(rgba);
  TEST_ASSERT_TRUE_MESSAGE(ok, "failed to create png for alpha bleed seedless test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = 16;
  g_config.pngx_lossy_dither_level = 0.0f;
  g_config.pngx_postprocess_smooth_enable = true;
  g_config.pngx_palette256_alpha_bleed_enable = true;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_TRUE(error == CPRES_OK || error == CPRES_ERROR_OUTPUT_NOT_SMALLER);
  if (error == CPRES_OK) {
    cpres_free(pngx_data);
  }

  free(png_data);
}

void test_pngx_palette256_alpha_bleed_applies_to_soft_pixels(void) {
  uint8_t rgba[2 * 4], *png_data = NULL, *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  bool ok;
  cpres_error_t error;

  rgba[0] = 255;
  rgba[1] = 0;
  rgba[2] = 0;
  rgba[3] = 255;

  rgba[4] = 0;
  rgba[5] = 255;
  rgba[6] = 0;
  rgba[7] = 1;

  ok = create_png_rgba_memory(rgba, 2, 1, &png_data, &png_size);
  TEST_ASSERT_TRUE_MESSAGE(ok, "failed to create png for alpha bleed apply test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = 8;
  g_config.pngx_lossy_dither_level = 0.0f;
  g_config.pngx_postprocess_smooth_enable = true;

  g_config.pngx_palette256_alpha_bleed_enable = true;
  g_config.pngx_palette256_alpha_bleed_max_distance = 2;
  g_config.pngx_palette256_alpha_bleed_opaque_threshold = 200;
  g_config.pngx_palette256_alpha_bleed_soft_limit = 10;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_TRUE(error == CPRES_OK || error == CPRES_ERROR_OUTPUT_NOT_SMALLER);
  if (error == CPRES_OK) {
    cpres_free(pngx_data);
  }

  free(png_data);
}

void test_pngx_palette256_gradient_profile_prefer_uniform_path(void) {
  uint32_t width = 64, height = 64, x, y;
  uint8_t *rgba = NULL, *png_data = NULL, *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0, base;
  bool ok;
  cpres_error_t error;

  rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
  TEST_ASSERT_NOT_NULL(rgba);

  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      base = (((size_t)y * (size_t)width) + (size_t)x) * 4;
      rgba[base + 0] = (uint8_t)x;
      rgba[base + 1] = (uint8_t)x;
      rgba[base + 2] = (uint8_t)x;
      rgba[base + 3] = 255;
    }
  }

  ok = create_png_rgba_memory(rgba, width, height, &png_data, &png_size);
  free(rgba);
  TEST_ASSERT_TRUE_MESSAGE(ok, "failed to create png for gradient profile test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = 128;
  g_config.pngx_lossy_dither_level = 0.0f;

  g_config.pngx_palette256_gradient_profile_enable = true;
  g_config.pngx_palette256_profile_opaque_ratio_threshold = 0.0f;
  g_config.pngx_palette256_profile_gradient_mean_max = 1.0f;
  g_config.pngx_palette256_profile_saturation_mean_max = 1.0f;
  g_config.pngx_palette256_gradient_dither_floor = 0.10f;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_TRUE(error == CPRES_OK || error == CPRES_ERROR_OUTPUT_NOT_SMALLER);
  if (error == CPRES_OK) {
    cpres_free(pngx_data);
  }

  free(png_data);
}

void test_pngx_palette256_tune_quant_params_clamps_speed_and_quality(void) {
  uint32_t width = 64, height = 64;
  uint8_t *rgba = NULL, *png_data = NULL, *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  bool ok;
  cpres_error_t error;

  rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
  TEST_ASSERT_NOT_NULL(rgba);

  fill_rgba_solid(rgba, width, height, 10, 20, 30, 255);

  ok = create_png_rgba_memory(rgba, width, height, &png_data, &png_size);
  free(rgba);
  TEST_ASSERT_TRUE_MESSAGE(ok, "failed to create png for tune clamp test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = 32;
  g_config.pngx_lossy_speed = 10;
  g_config.pngx_lossy_quality_min = 0;
  g_config.pngx_lossy_quality_max = 0;
  g_config.pngx_lossy_dither_level = 0.0f;

  g_config.pngx_palette256_gradient_profile_enable = false;

  g_config.pngx_palette256_tune_opaque_ratio_threshold = 0.0f;
  g_config.pngx_palette256_tune_gradient_mean_max = 1.0f;
  g_config.pngx_palette256_tune_saturation_mean_max = 1.0f;
  g_config.pngx_palette256_tune_speed_max = 1;
  g_config.pngx_palette256_tune_quality_min_floor = 90;
  g_config.pngx_palette256_tune_quality_max_target = 80;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_TRUE(error == CPRES_OK || error == CPRES_ERROR_OUTPUT_NOT_SMALLER);
  if (error == CPRES_OK) {
    cpres_free(pngx_data);
  }

  free(png_data);
}

void test_pngx_palette256_profile_defaults_are_accepted_when_negative(void) {
  uint32_t width = 64, height = 64;
  uint8_t *rgba = NULL, *png_data = NULL, *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  bool ok;
  cpres_error_t error;

  rgba = (uint8_t *)malloc((size_t)width * (size_t)height * 4);
  TEST_ASSERT_NOT_NULL(rgba);

  fill_rgba_solid(rgba, width, height, 40, 40, 40, 255);

  ok = create_png_rgba_memory(rgba, width, height, &png_data, &png_size);
  free(rgba);
  TEST_ASSERT_TRUE_MESSAGE(ok, "failed to create png for profile defaults test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
  g_config.pngx_lossy_max_colors = 32;
  g_config.pngx_lossy_dither_level = 0.0f;

  g_config.pngx_palette256_gradient_profile_enable = true;
  g_config.pngx_palette256_profile_opaque_ratio_threshold = -1.0f;
  g_config.pngx_palette256_profile_gradient_mean_max = -1.0f;
  g_config.pngx_palette256_profile_saturation_mean_max = -1.0f;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_TRUE(error == CPRES_OK || error == CPRES_ERROR_OUTPUT_NOT_SMALLER);
  if (error == CPRES_OK) {
    cpres_free(pngx_data);
  }

  free(png_data);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_pngx_palette256_color_count_verification);
  RUN_TEST(test_pngx_palette256_lossy_parameter_clamp_and_palette_reduction);
  RUN_TEST(test_pngx_palette256_protected_colors);
  RUN_TEST(test_pngx_palette256_postprocess_smoothing_with_low_dither);
  RUN_TEST(test_pngx_palette256_postprocess_indices_executes_without_internal_tests);
  RUN_TEST(test_pngx_palette256_alpha_bleed_seedless_transparent_image);
  RUN_TEST(test_pngx_palette256_alpha_bleed_applies_to_soft_pixels);
  RUN_TEST(test_pngx_palette256_gradient_profile_prefer_uniform_path);
  RUN_TEST(test_pngx_palette256_tune_quant_params_clamps_speed_and_quality);
  RUN_TEST(test_pngx_palette256_profile_defaults_are_accepted_when_negative);

  return UNITY_END();
}
