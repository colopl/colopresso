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

#include "test.h"

static cpres_config_t g_config;

void setUp(void) {
  cpres_config_init_defaults(&g_config);
  g_config.pngx_level = 1;
}

void tearDown(void) { release_cached_example_png(); }

void test_pngx_memory_lossy_dither_variations(void) {
  const uint8_t *png_data = NULL;
  size_t png_size = 0, size_out = 0;
  uint8_t *pngx_out = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_tiny_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for PNGX dither variations");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_dither_level = 0.5f;
  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_out, &size_out, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_out);

  cpres_free(pngx_out);
}

void test_pngx_memory_lossy_max_colors_variations(void) {
  const uint8_t *png_data = NULL;
  size_t png_size = 0, size_out = 0;
  uint8_t *pngx_out = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_tiny_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for PNGX max colors variations");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_max_colors = 64;
  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_out, &size_out, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_out);

  cpres_free(pngx_out);
}

void test_pngx_memory_lossy_mode(void) {
  const uint8_t *png_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  uint8_t *pngx_data = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_tiny_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for PNGX lossy memory test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_max_colors = 128;
  g_config.pngx_lossy_quality_min = 60;
  g_config.pngx_lossy_quality_max = 80;

  cpres_set_log_callback(test_default_log_callback);
  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  cpres_free(pngx_data);
}

void test_pngx_memory_lossy_quality_swap(void) {
  const uint8_t *png_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  uint8_t *pngx_out = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_tiny_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for PNGX quality swap test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_quality_min = 80;
  g_config.pngx_lossy_quality_max = 40;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_out, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_out);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  cpres_free(pngx_out);
}

void test_pngx_memory_lossy_quality_variations(void) {
  const uint8_t *png_data = NULL;
  size_t png_size = 0, size_out = 0;
  uint8_t *pngx_out = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_tiny_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for PNGX lossy quality variations");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_quality_min = 50;
  g_config.pngx_lossy_quality_max = 65;
  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_out, &size_out, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_out);

  cpres_free(pngx_out);
}

void test_pngx_memory_lossy_speed_variations(void) {
  const uint8_t *png_data = NULL;
  size_t png_size = 0, size_out = 0;
  uint8_t *pngx_out = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_tiny_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for PNGX speed variations");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_speed = 10;
  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_out, &size_out, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_out);

  cpres_free(pngx_out);
}

void test_pngx_memory_optimization_level_variations(void) {
  const uint8_t *png_data = NULL;
  size_t png_size = 0, size_out = 0;
  uint8_t *pngx_out = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_tiny_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for PNGX optimization level variations");

  g_config.pngx_level = 0;
  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_out, &size_out, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_out);

  cpres_free(pngx_out);
}

void test_pngx_memory_output_not_smaller_error(void) {
  const uint8_t *png_data = NULL;
  uint8_t *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = test_get_tiny_png(&png_size);
  TEST_ASSERT_NOT_NULL(png_data);

  g_config.pngx_lossy_enable = false;
  g_config.pngx_level = 6;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_OUTPUT_NOT_SMALLER, error);
  TEST_ASSERT_NULL(pngx_data);
  TEST_ASSERT_TRUE_MESSAGE(pngx_size >= png_size, "Encoded PNGX size should be reported when output is larger than input");
}

void test_pngx_memory_with_null_config(void) {
  uint8_t png_data[10], *pngx_data = NULL;
  size_t pngx_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_pngx_memory(png_data, sizeof(png_data), &pngx_data, &pngx_size, NULL);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_pngx_memory_with_null_data_ptr(void) {
  uint8_t png_data[10];
  size_t pngx_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_pngx_memory(png_data, sizeof(png_data), NULL, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_pngx_memory_with_null_png_data(void) {
  uint8_t *pngx_data = NULL;
  size_t pngx_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_pngx_memory(NULL, 100, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_pngx_memory_with_null_size_ptr(void) {
  uint8_t png_data[10], *pngx_data = NULL;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_pngx_memory(png_data, sizeof(png_data), &pngx_data, NULL, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_pngx_memory_with_threads_setting(void) {
  const uint8_t *png_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  uint8_t *pngx_out = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_tiny_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for PNGX threads test");

  g_config.pngx_threads = 2;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_out, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_out);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  cpres_free(pngx_out);
}

void test_pngx_memory_with_valid_png(void) {
  const uint8_t *png_data = NULL;
  uint8_t *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_tiny_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for PNGX memory test");

  cpres_set_log_callback(test_default_log_callback);
  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  cpres_free(pngx_data);
}

void test_pngx_memory_with_rgba64_png(void) {
  uint8_t *png_data = NULL, *pngx_data = NULL;
  size_t png_size = 0, pngx_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("16bit.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "16bit.png not found for PNGX memory test");

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  cpres_free(pngx_data);
  free(png_data);
}

void test_pngx_memory_with_zero_size(void) {
  uint8_t png_data[10], *pngx_data = NULL;
  size_t pngx_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_pngx_memory(png_data, 0, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_pngx_memory_lossy_dither_variations);
  RUN_TEST(test_pngx_memory_lossy_max_colors_variations);
  RUN_TEST(test_pngx_memory_lossy_mode);
  RUN_TEST(test_pngx_memory_lossy_quality_swap);
  RUN_TEST(test_pngx_memory_lossy_quality_variations);
  RUN_TEST(test_pngx_memory_lossy_speed_variations);
  RUN_TEST(test_pngx_memory_optimization_level_variations);
  RUN_TEST(test_pngx_memory_output_not_smaller_error);
  RUN_TEST(test_pngx_memory_with_null_config);
  RUN_TEST(test_pngx_memory_with_null_data_ptr);
  RUN_TEST(test_pngx_memory_with_null_png_data);
  RUN_TEST(test_pngx_memory_with_null_size_ptr);
  RUN_TEST(test_pngx_memory_with_threads_setting);
  RUN_TEST(test_pngx_memory_with_valid_png);
  RUN_TEST(test_pngx_memory_with_rgba64_png);
  RUN_TEST(test_pngx_memory_with_zero_size);

  return UNITY_END();
}
