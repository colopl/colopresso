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

#include <colopresso.h>

#include <unity.h>

#include "../src/internal/png.h"
#include "../src/internal/pngx_common.h"

#include "test.h"

static cpres_config_t g_config;
static const uint8_t g_sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

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

void setUp(void) {
  cpres_config_init_defaults(&g_config);
  g_config.pngx_level = 1;
}

void tearDown(void) { release_cached_example_png(); }

void test_pngx_color_count_verification(void) {
  const uint8_t *png_data = NULL;
  uint8_t *pngx_data = NULL, bit_depth = 0, color_type = 0;
  size_t png_size = 0, pngx_size = 0;
  int max_colors = 128, palette_count = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for color count test");

  g_config.pngx_lossy_enable = true;
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

void test_pngx_handles_rgba64_png_input(void) {
  uint8_t *png_data = NULL, *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("16bit.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "16bit.png not found for PNGX misc test");

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  cpres_free(pngx_data);
  free(png_data);
}

void test_pngx_error_string_for_size_increase(void) {
  const char *message = NULL;

  message = cpres_error_string(CPRES_ERROR_OUTPUT_NOT_SMALLER);

  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_EQUAL_STRING("Output image would be larger than input", message);
}

void test_pngx_free_with_null(void) { cpres_free(NULL); }

void test_pngx_free_with_valid_pointer(void) {
  uint8_t *p = (uint8_t *)malloc(10);

  TEST_ASSERT_NOT_NULL(p);

  cpres_free(p);
}

void test_pngx_last_error(void) {
  pngx_set_last_error(0);
  pngx_set_last_error(5678);
  TEST_ASSERT_EQUAL(5678, pngx_get_last_error());
}

void test_pngx_lossy_parameter_clamp_and_palette_reduction(void) {
  const uint8_t *png_data = NULL;
  uint8_t *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  int32_t orig_palette = 0, opt_palette = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for lossy test");

  orig_palette = parse_png_palette_count(png_data, png_size);

  g_config.pngx_lossy_enable = true;
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

void test_pngx_protected_colors(void) {
  const uint8_t *png_data = NULL;
  bool found_plte = false, found_this_color = false;
  char chunk_type[5] = {0}, msg[256] = {0};
  size_t png_size = 0, pngx_size = 0, pixel_offset = 0, chunk_offset = 0;
  uint8_t *pngx_data = NULL, *rgba_data = NULL, *palette = NULL, bit_depth = 0, color_type = 0, r = 0, g = 0, b = 0, i = 0;
  uint32_t total_pixels = 0, pixel_index = 0, chunk_len = 0, num_palette_entries = 0, j = 0;
  int32_t palette_count = 0, protected_colors_found = 0;
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

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_pngx_color_count_verification);
  RUN_TEST(test_pngx_handles_rgba64_png_input);
  RUN_TEST(test_pngx_error_string_for_size_increase);
  RUN_TEST(test_pngx_free_with_null);
  RUN_TEST(test_pngx_free_with_valid_pointer);
  RUN_TEST(test_pngx_last_error);
  RUN_TEST(test_pngx_lossy_parameter_clamp_and_palette_reduction);
  RUN_TEST(test_pngx_protected_colors);

  return UNITY_END();
}
