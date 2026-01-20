/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2026 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <colopresso.h>

#include <unity.h>

#include "../src/internal/avif.h"
#include "test.h"

static cpres_config_t g_config;

void setUp(void) {
  cpres_config_init_defaults(&g_config);
  g_config.avif_speed = 10;
}

void tearDown(void) { release_cached_example_png(); }

void test_avif_encode_rgba_with_zero_dimensions(void) {
  uint8_t pixel[4] = {0, 0, 0, 0}, *out_data = NULL;
  size_t out_size = 0;
  cpres_config_t cfg;
  cpres_error_t error = CPRES_OK;

  cpres_config_init_defaults(&cfg);

  error = avif_encode_rgba_to_memory(pixel, 0, 1, &out_data, &out_size, &cfg);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_OUT_OF_MEMORY, error);
  TEST_ASSERT_NULL(out_data);
  TEST_ASSERT_EQUAL_size_t(0, out_size);
}

#ifndef COLOPRESSO_DISABLE_FILE_OPS
void test_avif_file_lossless_mode(void) {
  FILE *fp = NULL;
  char input_path[512], output_path[512];
  cpres_error_t error = CPRES_OK;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);
  snprintf(output_path, sizeof(output_path), "example_avif_lossless.avif");

  g_config.avif_lossless = true;
  error = cpres_encode_avif_file(input_path, output_path, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_path, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  if (fp != NULL) {
    fclose(fp);
  }
}

void test_avif_file_quality_variations(void) {
  FILE *fp = NULL;
  char input_path[512], output_low[512], output_high[512];
  int32_t size_low = 0, size_high = 0;
  cpres_error_t error = CPRES_OK;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);
  snprintf(output_low, sizeof(output_low), "example_avif_quality_low.avif");
  snprintf(output_high, sizeof(output_high), "example_avif_quality_high.avif");

  g_config.avif_quality = 30.0f;
  error = cpres_encode_avif_file(input_path, output_low, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.avif_quality = 90.0f;
  error = cpres_encode_avif_file(input_path, output_high, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_low, "rb");
  if (fp != NULL) {
    fseek(fp, 0, SEEK_END);
    size_low = (int32_t)ftell(fp);
    fclose(fp);
  }

  fp = fopen(output_high, "rb");
  if (fp != NULL) {
    fseek(fp, 0, SEEK_END);
    size_high = (int32_t)ftell(fp);
    fclose(fp);
  }

  TEST_ASSERT_GREATER_THAN(0, size_low);
  TEST_ASSERT_GREATER_THAN(0, size_high);
  TEST_ASSERT_NOT_EQUAL(size_low, size_high);
}

void test_avif_file_with_nonexistent_input(void) {
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_avif_file("/nonexistent/file.png", "output.avif", &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_FILE_NOT_FOUND, error);
}

void test_avif_file_with_null_config(void) {
  char input_path[512];
  cpres_error_t error = CPRES_OK;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);
  error = cpres_encode_avif_file(input_path, "output.avif", NULL);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_avif_file_with_null_input_path(void) {
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_avif_file(NULL, "output.avif", &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_avif_file_with_null_output_path(void) {
  char input_path[512];
  cpres_error_t error = CPRES_OK;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);
  error = cpres_encode_avif_file(input_path, NULL, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_avif_file_with_valid_png(void) {
  FILE *fp = NULL;
  char input_path[512], output_path[512];
  cpres_error_t error = CPRES_OK;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);
  snprintf(output_path, sizeof(output_path), "example_avif.avif");

  cpres_set_log_callback(test_default_log_callback);

  error = cpres_encode_avif_file(input_path, output_path, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_path, "rb");

  TEST_ASSERT_NOT_NULL(fp);

  if (fp != NULL) {
    fclose(fp);
  }
}

void test_avif_file_with_rgba64_png(void) {
  FILE *fp = NULL;
  char input_path[512], output_path[512];
  cpres_error_t error = CPRES_OK;

  if (!format_test_asset_path(input_path, sizeof(input_path), "16bit.png")) {
    TEST_FAIL_MESSAGE("failed to format RGBA64 AVIF input path");
  }
  if (snprintf(output_path, sizeof(output_path), "example_avif_rgba64.avif") < 0) {
    TEST_FAIL_MESSAGE("failed to format RGBA64 AVIF output path");
  }

  remove(output_path);

  error = cpres_encode_avif_file(input_path, output_path, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_path, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  if (fp != NULL) {
    fclose(fp);
  }

  remove(output_path);
}
#endif

void test_avif_free_with_null(void) { cpres_free(NULL); }

void test_avif_free_with_valid_pointer(void) {
  uint8_t *p = (uint8_t *)malloc(10);

  TEST_ASSERT_NOT_NULL(p);

  cpres_free(p);
}

void test_avif_last_error(void) {
  avif_set_last_error(0);
  avif_set_last_error(1234);
  TEST_ASSERT_EQUAL(1234, avif_get_last_error());
}

void test_avif_memory_alpha_quality_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *avif_alpha_low = NULL, *avif_alpha_high = NULL;
  size_t png_size = 0, size_low = 0, size_high = 0;
  cpres_error_t error = CPRES_OK;
  int32_t q_low = 0, q_high = 0;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for AVIF alpha quality variations");

  g_config.avif_alpha_quality = 10;
  error = cpres_encode_avif_memory(png_data, png_size, &avif_alpha_low, &size_low, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.avif_alpha_quality = 90;
  error = cpres_encode_avif_memory(png_data, png_size, &avif_alpha_high, &size_high, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_low);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_high);
  TEST_ASSERT_NOT_EQUAL(size_low, size_high);

  q_low = ((100 - 10) * 63 + 50) / 100;
  q_high = ((100 - 90) * 63 + 50) / 100;
  TEST_ASSERT_GREATER_THAN(q_high, q_low);

  cpres_free(avif_alpha_low);
  cpres_free(avif_alpha_high);
}

void test_avif_memory_clamps_extreme_values(void) {
  const uint8_t *png_data = NULL;
  uint8_t *avif_data = NULL;
  size_t png_size = 0, avif_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for AVIF clamp test");

  g_config.avif_quality = -10.0f;
  g_config.avif_alpha_quality = 150;
  g_config.avif_speed = -3;
  error = cpres_encode_avif_memory(png_data, png_size, &avif_data, &avif_size, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(avif_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, avif_size);
  cpres_free(avif_data);

  g_config.avif_quality = 150.0f;
  g_config.avif_alpha_quality = -20;
  g_config.avif_speed = 20;
  avif_data = NULL;
  avif_size = 0;
  error = cpres_encode_avif_memory(png_data, png_size, &avif_data, &avif_size, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(avif_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, avif_size);
  cpres_free(avif_data);
}

void test_avif_memory_lossless_mode(void) {
  const uint8_t *png_data = NULL;
  uint8_t *avif_data = NULL;
  size_t png_size = 0, avif_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for AVIF lossless memory test");

  g_config.avif_lossless = true;
  cpres_set_log_callback(test_default_log_callback);
  error = cpres_encode_avif_memory(png_data, png_size, &avif_data, &avif_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(avif_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, avif_size);

  cpres_free(avif_data);
}

void test_avif_memory_output_not_smaller_error(void) {
  const uint8_t *png_data = NULL;
  uint8_t *avif_data = NULL;
  size_t png_size = 0, avif_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = test_get_tiny_png(&png_size);
  TEST_ASSERT_NOT_NULL(png_data);

  g_config.avif_lossless = true;
  g_config.avif_quality = 100.0f;
  g_config.avif_alpha_quality = 100;

  error = cpres_encode_avif_memory(png_data, png_size, &avif_data, &avif_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_OUTPUT_NOT_SMALLER, error);
  TEST_ASSERT_NULL(avif_data);
  TEST_ASSERT_TRUE_MESSAGE(avif_size >= png_size, "Encoded AVIF size should be reported when output is larger than input");
}

void test_avif_memory_quality_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *avif_low = NULL, *avif_high = NULL;
  size_t png_size = 0, size_low = 0, size_high = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for AVIF quality variations");

  g_config.avif_quality = 30.0f;
  error = cpres_encode_avif_memory(png_data, png_size, &avif_low, &size_low, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.avif_quality = 90.0f;
  error = cpres_encode_avif_memory(png_data, png_size, &avif_high, &size_high, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_low);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_high);
  TEST_ASSERT_NOT_EQUAL(size_low, size_high);

  cpres_free(avif_low);
  cpres_free(avif_high);
}

void test_avif_memory_speed_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *avif_slow = NULL, *avif_fast = NULL;
  size_t png_size = 0, size_slow = 0, size_fast = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for AVIF speed variations");

  g_config.avif_speed = 6;
  error = cpres_encode_avif_memory(png_data, png_size, &avif_slow, &size_slow, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.avif_speed = 10;
  error = cpres_encode_avif_memory(png_data, png_size, &avif_fast, &size_fast, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_slow);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_fast);
  TEST_ASSERT_NOT_EQUAL(size_slow, size_fast);

  cpres_free(avif_slow);
  cpres_free(avif_fast);
}

void test_avif_memory_with_null_config(void) {
  uint8_t png_data[10], *avif_data = NULL;
  size_t avif_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_avif_memory(png_data, sizeof(png_data), &avif_data, &avif_size, NULL);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_avif_memory_with_null_data_ptr(void) {
  uint8_t png_data[10];
  size_t avif_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_avif_memory(png_data, sizeof(png_data), NULL, &avif_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_avif_memory_with_null_png_data(void) {
  uint8_t *avif_data = NULL;
  size_t avif_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_avif_memory(NULL, 100, &avif_data, &avif_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_avif_memory_with_null_size_ptr(void) {
  uint8_t png_data[10], *avif_data = NULL;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_avif_memory(png_data, sizeof(png_data), &avif_data, NULL, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_avif_memory_with_oversized_input(void) {
  uint8_t png_data[10], *avif_data = NULL;
  size_t avif_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_avif_memory(png_data, (size_t)1024 * 1024 * 512 + 1, &avif_data, &avif_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_avif_memory_with_valid_png(void) {
  const uint8_t *png_data = NULL;
  uint8_t *avif_data = NULL;
  size_t png_size = 0, avif_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for AVIF memory test");

  cpres_set_log_callback(test_default_log_callback);
  error = cpres_encode_avif_memory(png_data, png_size, &avif_data, &avif_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(avif_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, avif_size);

  cpres_free(avif_data);
}

void test_avif_memory_with_rgba64_png(void) {
  uint8_t *png_data = NULL, *avif_data = NULL;
  size_t png_size = 0, avif_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("16bit.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "16bit.png not found for AVIF memory test");

  error = cpres_encode_avif_memory(png_data, png_size, &avif_data, &avif_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(avif_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, avif_size);

  cpres_free(avif_data);
  free(png_data);
}

void test_avif_memory_with_zero_size(void) {
  uint8_t png_data[10], *avif_data = NULL;
  size_t avif_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_avif_memory(png_data, 0, &avif_data, &avif_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_avif_null_params(void) {
  uint8_t dummy[4] = {0, 0, 0, 0}, *out_data = NULL;
  size_t out_size = 0;
  cpres_config_t cfg;

  cpres_config_init_defaults(&cfg);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, avif_encode_rgba_to_memory(NULL, 1, 1, &out_data, &out_size, &cfg));
  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, avif_encode_rgba_to_memory(dummy, 1, 1, NULL, &out_size, &cfg));
  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, avif_encode_rgba_to_memory(dummy, 1, 1, &out_data, NULL, &cfg));
  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, avif_encode_rgba_to_memory(dummy, 1, 1, &out_data, &out_size, NULL));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_avif_encode_rgba_with_zero_dimensions);
#ifndef COLOPRESSO_DISABLE_FILE_OPS
  RUN_TEST(test_avif_file_lossless_mode);
  RUN_TEST(test_avif_file_quality_variations);
  RUN_TEST(test_avif_file_with_nonexistent_input);
  RUN_TEST(test_avif_file_with_null_config);
  RUN_TEST(test_avif_file_with_null_input_path);
  RUN_TEST(test_avif_file_with_null_output_path);
  RUN_TEST(test_avif_file_with_valid_png);
  RUN_TEST(test_avif_file_with_rgba64_png);
#endif
  RUN_TEST(test_avif_free_with_null);
  RUN_TEST(test_avif_free_with_valid_pointer);
  RUN_TEST(test_avif_last_error);
  RUN_TEST(test_avif_memory_alpha_quality_variations);
  RUN_TEST(test_avif_memory_clamps_extreme_values);
  RUN_TEST(test_avif_memory_lossless_mode);
  RUN_TEST(test_avif_memory_output_not_smaller_error);
  RUN_TEST(test_avif_memory_quality_variations);
  RUN_TEST(test_avif_memory_speed_variations);
  RUN_TEST(test_avif_memory_with_null_config);
  RUN_TEST(test_avif_memory_with_null_data_ptr);
  RUN_TEST(test_avif_memory_with_null_png_data);
  RUN_TEST(test_avif_memory_with_null_size_ptr);
  RUN_TEST(test_avif_memory_with_oversized_input);
  RUN_TEST(test_avif_memory_with_valid_png);
  RUN_TEST(test_avif_memory_with_rgba64_png);
  RUN_TEST(test_avif_memory_with_zero_size);
  RUN_TEST(test_avif_null_params);

  return UNITY_END();
}
