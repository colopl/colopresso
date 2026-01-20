/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2025-2026 COLOPL, Inc.
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
#include "../src/internal/pngx.h"
#include "../src/internal/pngx_common.h"

#include "test.h"

static cpres_config_t g_config;

static void assert_channel_levels_within_bits(const uint8_t *rgba, size_t pixel_count, uint8_t bits_per_channel) {
  uint32_t channel;
  uint8_t used_bits = 0;
  bool computed = false;

  if (!rgba || pixel_count == 0 || bits_per_channel >= 8) {
    return;
  }

  for (channel = 0; channel < 4; ++channel) {
    computed = bits_required_for_channel(rgba, pixel_count, (uint8_t)channel, &used_bits);
    TEST_ASSERT_TRUE_MESSAGE(computed, (channel == 3) ? "failed to compute alpha bits" : "failed to compute color bits");
    TEST_ASSERT_TRUE_MESSAGE(used_bits <= bits_per_channel, (channel == 3) ? "alpha channel unique count" : "color channel unique count");
  }
}

static bool run_bitdepth_quantization_only(const uint8_t *png_data, size_t png_size, int lossy_type, float dither_level, uint8_t **out_data, size_t *out_size) {
  pngx_options_t opts;
  cpres_config_t config;
  int quant_quality = -1;

  if (!png_data || png_size == 0 || !out_data || !out_size) {
    return false;
  }

  config = g_config;
  config.pngx_lossy_enable = true;
  config.pngx_lossy_type = lossy_type;
  config.pngx_lossy_dither_level = dither_level;

  pngx_fill_pngx_options(&opts, &config);

  return pngx_run_quantization(png_data, png_size, &opts, out_data, out_size, &quant_quality);
}

void setUp(void) {
  cpres_config_init_defaults(&g_config);
  g_config.pngx_level = 1;
}

void tearDown(void) { release_cached_example_png(); }

void test_pngx_limited_rgba4444_auto_dither_estimation(void) {
  const uint8_t rgba[16] = {
      0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255,
  };
  float dither = 0.0f;

  dither = estimate_bitdepth_dither_level_limited4444(rgba, 2, 2);

  TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.05f, dither);
}

void test_pngx_limited_rgba4444_bitdepth_reduction(void) {
  size_t png_size = 0, pngx_size = 0;
  uint8_t *png_data = NULL, *pngx_data = NULL, *rgba = NULL;
  png_uint_32 width = 0, height = 0;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("128x128.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "128x128.png not found for Limited RGBA4444 test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444;
  g_config.pngx_lossy_dither_level = 1.0f;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);
  TEST_ASSERT_LESS_THAN_size_t(png_size, pngx_size);
  TEST_ASSERT_EQUAL_UINT8(8, pngx_data[24]);
  TEST_ASSERT_EQUAL_UINT8(6, pngx_data[25]);

  error = png_decode_from_memory(pngx_data, pngx_size, &rgba, &width, &height);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(rgba);
  assert_channel_levels_within_bits(rgba, (size_t)width * (size_t)height, 4);

  free(rgba);
  cpres_free(pngx_data);
  free(png_data);
}

void test_pngx_limited_rgba4444_color_usage_limits(void) {
  size_t png_size = 0, pngx_size = 0, unique_colors = 0;
  uint8_t *png_data = NULL, *pngx_data = NULL, *rgba = NULL;
  png_uint_32 width = 0, height = 0;
  bool quant_success = false, counted = false;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("128x128.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "128x128.png not found");

  quant_success = run_bitdepth_quantization_only(png_data, png_size, CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444, 1.0f, &pngx_data, &pngx_size);
  TEST_ASSERT_TRUE_MESSAGE(quant_success, "bitdepth quantization failed");
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  error = png_decode_from_memory(pngx_data, pngx_size, &rgba, &width, &height);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(rgba);
  TEST_ASSERT_GREATER_THAN(0, width);
  TEST_ASSERT_GREATER_THAN(0, height);

  assert_channel_levels_within_bits(rgba, (size_t)width * (size_t)height, 4);

  counted = count_unique_rgba_colors(rgba, (size_t)width * (size_t)height, &unique_colors);
  TEST_ASSERT_TRUE_MESSAGE(counted, "failed to count unique colors");
  TEST_ASSERT_TRUE(unique_colors > 0);
  TEST_ASSERT_LESS_OR_EQUAL_UINT32(65536, (uint32_t)unique_colors);

  free(rgba);
  cpres_free(pngx_data);
  free(png_data);
}

void test_pngx_limited_rgba4444_with_rgba64_png(void) {
  size_t png_size = 0, pngx_size = 0;
  uint8_t *png_data = NULL, *pngx_data = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("16bit.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "16bit.png not found for Limited RGBA4444 RGBA64 test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444;
  g_config.pngx_lossy_dither_level = 0.75f;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  cpres_free(pngx_data);
  free(png_data);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_pngx_limited_rgba4444_auto_dither_estimation);
  RUN_TEST(test_pngx_limited_rgba4444_bitdepth_reduction);
  RUN_TEST(test_pngx_limited_rgba4444_color_usage_limits);
  RUN_TEST(test_pngx_limited_rgba4444_with_rgba64_png);

  return UNITY_END();
}
