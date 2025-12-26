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

void setUp(void) {
  cpres_config_init_defaults(&g_config);
  g_config.pngx_level = 1;
}

void tearDown(void) { release_cached_example_png(); }

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

void test_pngx_estimate_bitdepth_dither_level_with_null(void) {
  float dither = estimate_bitdepth_dither_level(NULL, 0, 0, 8);

  TEST_ASSERT_FLOAT_WITHIN(0.01f, COLOPRESSO_PNGX_DEFAULT_LOSSY_DITHER_LEVEL, dither);
}

void test_pngx_estimate_bitdepth_dither_level_with_zero_dimensions(void) {
  const uint8_t rgba[16] = {0};
  float dither = estimate_bitdepth_dither_level(rgba, 0, 0, 8);

  TEST_ASSERT_FLOAT_WITHIN(0.01f, COLOPRESSO_PNGX_DEFAULT_LOSSY_DITHER_LEVEL, dither);
}

void test_pngx_estimate_bitdepth_dither_level_uniform_black(void) {
  const uint8_t rgba[64] = {
      0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255,
      0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255,
  };
  float dither = estimate_bitdepth_dither_level(rgba, 4, 4, 8);

  TEST_ASSERT_TRUE(dither >= 0.0f && dither <= 1.0f);
}

void test_pngx_estimate_bitdepth_dither_level_gradient(void) {
  uint8_t rgba[256];
  uint32_t i;
  float dither;

  for (i = 0; i < 64; ++i) {
    rgba[i * 4 + 0] = (uint8_t)(i * 4);
    rgba[i * 4 + 1] = (uint8_t)(i * 4);
    rgba[i * 4 + 2] = (uint8_t)(i * 4);
    rgba[i * 4 + 3] = 255;
  }

  dither = estimate_bitdepth_dither_level(rgba, 8, 8, 8);

  TEST_ASSERT_TRUE(dither >= 0.0f && dither <= 1.0f);
}

void test_pngx_estimate_bitdepth_dither_level_low_bits(void) {
  const uint8_t rgba[64] = {
      128, 64, 32, 255, 200, 100, 50, 255, 64, 128, 192, 255, 255, 128, 0, 255, 128, 64, 32, 255, 200, 100, 50, 255, 64, 128, 192, 255, 255, 128, 0, 255,
      128, 64, 32, 255, 200, 100, 50, 255, 64, 128, 192, 255, 255, 128, 0, 255, 128, 64, 32, 255, 200, 100, 50, 255, 64, 128, 192, 255, 255, 128, 0, 255,
  };
  float dither = estimate_bitdepth_dither_level(rgba, 4, 4, 2);

  TEST_ASSERT_TRUE(dither >= 0.0f && dither <= 1.0f);
}

void test_pngx_estimate_bitdepth_dither_level_translucent(void) {
  const uint8_t rgba[64] = {
      255, 0, 0, 128, 0, 255, 0, 64, 0, 0, 255, 32, 255, 255, 0, 96, 255, 0, 0, 128, 0, 255, 0, 64, 0, 0, 255, 32, 255, 255, 0, 96,
      255, 0, 0, 128, 0, 255, 0, 64, 0, 0, 255, 32, 255, 255, 0, 96, 255, 0, 0, 128, 0, 255, 0, 64, 0, 0, 255, 32, 255, 255, 0, 96,
  };
  float dither = estimate_bitdepth_dither_level(rgba, 4, 4, 8);

  TEST_ASSERT_TRUE(dither >= 0.0f && dither <= 1.0f);
}

void test_pngx_estimate_bitdepth_dither_level_single_pixel(void) {
  const uint8_t rgba[4] = {255, 128, 64, 255};
  float dither = estimate_bitdepth_dither_level(rgba, 1, 1, 8);

  TEST_ASSERT_TRUE(dither >= 0.0f && dither <= 1.0f);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_pngx_handles_rgba64_png_input);
  RUN_TEST(test_pngx_error_string_for_size_increase);
  RUN_TEST(test_pngx_free_with_null);
  RUN_TEST(test_pngx_free_with_valid_pointer);
  RUN_TEST(test_pngx_last_error);
  RUN_TEST(test_pngx_estimate_bitdepth_dither_level_with_null);
  RUN_TEST(test_pngx_estimate_bitdepth_dither_level_with_zero_dimensions);
  RUN_TEST(test_pngx_estimate_bitdepth_dither_level_uniform_black);
  RUN_TEST(test_pngx_estimate_bitdepth_dither_level_gradient);
  RUN_TEST(test_pngx_estimate_bitdepth_dither_level_low_bits);
  RUN_TEST(test_pngx_estimate_bitdepth_dither_level_translucent);
  RUN_TEST(test_pngx_estimate_bitdepth_dither_level_single_pixel);

  return UNITY_END();
}
