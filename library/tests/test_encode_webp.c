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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <colopresso.h>

#include <unity.h>

#include "../src/internal/webp.h"
#include "test.h"

static cpres_config_t g_config;

void setUp(void) {
  cpres_config_init_defaults(&g_config);
  g_config.webp_method = 0;
  g_config.webp_pass = 1;
  g_config.webp_segments = 1;
  g_config.webp_filter_strength = 20;
  cpres_set_log_callback(test_default_log_callback);
}

void tearDown(void) {
  cpres_set_log_callback(NULL);
  release_cached_example_png();
}

#ifndef COLOPRESSO_DISABLE_FILE_OPS
void test_webp_file_lossless_mode(void) {
  FILE *fp = NULL;
  char input_path[512], output_path[512];
  cpres_error_t error = CPRES_OK;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);
  snprintf(output_path, sizeof(output_path), "example_webp_lossless.webp");

  g_config.webp_lossless = true;
  error = cpres_encode_webp_file(input_path, output_path, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_path, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  if (fp != NULL) {
    fclose(fp);
  }
}

void test_webp_file_quality_variations(void) {
  FILE *fp = NULL;
  char input_path[512], output_low[512], output_high[512];
  cpres_error_t error = CPRES_OK;
  long size_low = 0, size_high = 0;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);
  snprintf(output_low, sizeof(output_low), "example_webp_quality_low.webp");
  snprintf(output_high, sizeof(output_high), "example_webp_quality_high.webp");

  g_config.webp_quality = 30.0f;
  error = cpres_encode_webp_file(input_path, output_low, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.webp_quality = 90.0f;
  error = cpres_encode_webp_file(input_path, output_high, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_low, "rb");
  if (fp != NULL) {
    fseek(fp, 0, SEEK_END);
    size_low = ftell(fp);
    fclose(fp);
  }

  fp = fopen(output_high, "rb");
  if (fp != NULL) {
    fseek(fp, 0, SEEK_END);
    size_high = ftell(fp);
    fclose(fp);
  }

  TEST_ASSERT_GREATER_THAN(0, size_low);
  TEST_ASSERT_GREATER_THAN(0, size_high);
}

void test_webp_file_with_nonexistent_input(void) {
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_webp_file("/nonexistent/file.png", "output.webp", &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_FILE_NOT_FOUND, error);
}

void test_webp_file_with_null_config(void) {
  char input_path[512];
  cpres_error_t error = CPRES_OK;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);

  error = cpres_encode_webp_file(input_path, "output.webp", NULL);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_webp_file_with_null_input_path(void) {
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_webp_file(NULL, "output.webp", &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_webp_file_with_null_output_path(void) {
  char input_path[512];
  cpres_error_t error = CPRES_OK;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);

  error = cpres_encode_webp_file(input_path, NULL, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_webp_file_with_valid_png(void) {
  FILE *fp = NULL;
  char input_path[512], output_path[512];
  cpres_error_t error = CPRES_OK;

  snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);
  snprintf(output_path, sizeof(output_path), "example_webp.webp");
  cpres_set_log_callback(test_default_log_callback);
  error = cpres_encode_webp_file(input_path, output_path, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_path, "rb");

  TEST_ASSERT_NOT_NULL(fp);

  if (fp != NULL) {
    fclose(fp);
  }
}

void test_webp_file_with_rgba64_png(void) {
  FILE *fp = NULL;
  char input_path[512], output_path[512];
  cpres_error_t error = CPRES_OK;

  if (!format_test_asset_path(input_path, sizeof(input_path), "16bit.png")) {
    TEST_FAIL_MESSAGE("failed to format RGBA64 WebP input path");
  }
  if (snprintf(output_path, sizeof(output_path), "example_webp_rgba64.webp") < 0) {
    TEST_FAIL_MESSAGE("failed to format RGBA64 WebP output path");
  }

  remove(output_path);

  error = cpres_encode_webp_file(input_path, output_path, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_path, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  if (fp != NULL) {
    fclose(fp);
  }

  remove(output_path);
}
#endif

void test_webp_free_with_null(void) { cpres_free(NULL); }

void test_webp_free_with_valid_pointer(void) {
  uint8_t *data = (uint8_t *)malloc(100);

  TEST_ASSERT_NOT_NULL(data);

  cpres_free(data);
}

void test_webp_last_error(void) {
  uint8_t rgba[4 * 2 * 2], *out_data = NULL;
  size_t out_size = 0;
  cpres_config_t cfg;
  cpres_error_t error = CPRES_OK;

  memset(rgba, 128, sizeof(rgba));
  cpres_config_init_defaults(&cfg);
  cfg.webp_quality = 999.0f;
  cfg.webp_alpha_quality = -123;
  cfg.webp_method = 99;
  cfg.webp_lossless = 1;
  error = webp_encode_rgba_to_memory(rgba, 2, 2, &out_data, &out_size, &cfg);
  if (error == CPRES_OK) {
    cpres_free(out_data);
  }

  webp_set_last_error(4321);

  TEST_ASSERT_EQUAL(4321, webp_get_last_error());
}

void test_webp_memory_alpha_compression_filtering_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_ac_on = NULL, *webp_ac_off = NULL;
  size_t png_size = 0, size_on = 0, size_off = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp alpha compression/filtering test");

  g_config.webp_alpha_compression = true;
  g_config.webp_alpha_filtering = 1;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_ac_on, &size_on, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.webp_alpha_compression = false;
  g_config.webp_alpha_filtering = 0;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_ac_off, &size_off, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_on);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_off);

  cpres_free(webp_ac_on);
  cpres_free(webp_ac_off);
}

void test_webp_memory_alpha_quality_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_alpha_low = NULL, *webp_alpha_high = NULL;
  size_t png_size = 0, size_low = 0, size_high = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp alpha quality variations");

  g_config.webp_alpha_quality = 10;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_alpha_low, &size_low, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.webp_alpha_quality = 90;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_alpha_high, &size_high, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_low);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_high);

  cpres_free(webp_alpha_low);
  cpres_free(webp_alpha_high);
}

void test_webp_memory_emulate_jpeg_size_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_ej_on = NULL, *webp_ej_off = NULL;
  size_t png_size = 0, size_on = 0, size_off = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp emulate jpeg size test");

  g_config.webp_emulate_jpeg_size = true;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_ej_on, &size_on, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.webp_emulate_jpeg_size = false;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_ej_off, &size_off, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_on);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_off);

  cpres_free(webp_ej_on);
  cpres_free(webp_ej_off);
}

void test_webp_memory_lossless_mode(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_data = NULL;
  size_t png_size = 0, webp_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp lossless memory test");

  g_config.webp_lossless = true;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_data, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(webp_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, webp_size);

  cpres_free(webp_data);
}

void test_webp_memory_method_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_data = NULL;
  size_t png_size = 0, webp_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp method variations");

  g_config.webp_method = 0;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_data, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(webp_data);

  cpres_free(webp_data);
  webp_data = NULL;
  g_config.webp_method = 6;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_data, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(webp_data);

  cpres_free(webp_data);
}

void test_webp_memory_near_lossless_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_nn_low = NULL, *webp_nn_high = NULL;
  size_t png_size = 0, size_low = 0, size_high = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp near lossless variations");

  g_config.webp_near_lossless = 10;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_nn_low, &size_low, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.webp_near_lossless = 90;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_nn_high, &size_high, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_low);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_high);

  cpres_free(webp_nn_low);
  cpres_free(webp_nn_high);
}

void test_webp_memory_output_not_smaller_error(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_data = NULL;
  size_t png_size = 0, webp_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = test_get_tiny_png(&png_size);
  TEST_ASSERT_NOT_NULL(png_data);

  g_config.webp_lossless = false;
  g_config.webp_quality = 100.0f;
  g_config.webp_target_size = (int)(png_size * 2);

  error = cpres_encode_webp_memory(png_data, png_size, &webp_data, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_OUTPUT_NOT_SMALLER, error);
  TEST_ASSERT_NULL(webp_data);
  TEST_ASSERT_TRUE_MESSAGE(webp_size >= png_size, "Encoded WebP size should be reported when output is larger than input");
}

void test_webp_memory_partitions_segments_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_seg = NULL, *webp_part = NULL;
  size_t png_size = 0, size_seg = 0, size_part = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp segments/partitions test");

  g_config.webp_segments = 1;
  g_config.webp_partitions = 0;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_seg, &size_seg, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.webp_segments = 4;
  g_config.webp_partitions = 3;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_part, &size_part, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_seg);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_part);

  cpres_free(webp_seg);
  cpres_free(webp_part);
}

void test_webp_memory_quality_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_low = NULL, *webp_high = NULL;
  size_t png_size = 0, size_low = 0, size_high = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp quality variations");

  g_config.webp_quality = 30.0f;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_low, &size_low, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.webp_quality = 90.0f;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_high, &size_high, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_low);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_high);

  cpres_free(webp_low);
  cpres_free(webp_high);
}

void test_webp_memory_target_size_psnr_ignored(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_ts = NULL, *webp_tp = NULL;
  size_t png_size = 0, size_ts = 0, size_tp = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp target size/psnr test");

  g_config.webp_target_size = 5000;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_ts, &size_ts, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.webp_target_size = 0;
  g_config.webp_target_psnr = 42.0f;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_tp, &size_tp, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_ts);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_tp);

  cpres_free(webp_ts);
  cpres_free(webp_tp);
}

void test_webp_memory_use_sharp_yuv_variations(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_sharp_on = NULL, *webp_sharp_off = NULL;
  size_t png_size = 0, size_on = 0, size_off = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp sharp yuv test");

  g_config.webp_use_sharp_yuv = true;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_sharp_on, &size_on, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  g_config.webp_use_sharp_yuv = false;
  error = cpres_encode_webp_memory(png_data, png_size, &webp_sharp_off, &size_off, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_on);
  TEST_ASSERT_GREATER_THAN_size_t(0, size_off);

  cpres_free(webp_sharp_on);
  cpres_free(webp_sharp_off);
}

void test_webp_memory_with_null_config(void) {
  uint8_t png_data[100], *webp_data = NULL;
  size_t webp_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_webp_memory(png_data, sizeof(png_data), &webp_data, &webp_size, NULL);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_webp_memory_with_null_data_ptr(void) {
  uint8_t png_data[100];
  size_t webp_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_webp_memory(png_data, sizeof(png_data), NULL, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_webp_memory_with_null_png_data(void) {
  uint8_t *webp_data = NULL;
  size_t webp_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_webp_memory(NULL, 100, &webp_data, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_webp_memory_with_null_size_ptr(void) {
  uint8_t png_data[100], *webp_data = NULL;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_webp_memory(png_data, sizeof(png_data), &webp_data, NULL, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_webp_memory_with_oversized_input(void) {
  uint8_t png_data[100], *webp_data = NULL;
  size_t webp_size = 0;
  cpres_error_t error = CPRES_OK;

  memset(png_data, 0, sizeof(png_data));

  error = cpres_encode_webp_memory(png_data, (size_t)1024 * 1024 * 512 + 1, &webp_data, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_webp_memory_with_valid_png(void) {
  const uint8_t *png_data = NULL;
  uint8_t *webp_data = NULL;
  size_t png_size = 0, webp_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = get_cached_example_png(&png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example.png not found for webp memory test");
  error = cpres_encode_webp_memory(png_data, png_size, &webp_data, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(webp_data);

  cpres_free(webp_data);

  TEST_ASSERT_GREATER_THAN_size_t(0, webp_size);
}

void test_webp_memory_with_rgba64_png(void) {
  uint8_t *png_data = NULL, *webp_data = NULL;
  size_t png_size = 0, webp_size = 0;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("16bit.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "16bit.png not found for WebP memory test");

  error = cpres_encode_webp_memory(png_data, png_size, &webp_data, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(webp_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, webp_size);

  cpres_free(webp_data);
  free(png_data);
}

void test_webp_memory_with_zero_size(void) {
  uint8_t png_data[100], *webp_data = NULL;
  size_t webp_size = 0;
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_webp_memory(png_data, 0, &webp_data, &webp_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

int main(void) {
  UNITY_BEGIN();

#ifndef COLOPRESSO_DISABLE_FILE_OPS
  RUN_TEST(test_webp_file_lossless_mode);
  RUN_TEST(test_webp_file_quality_variations);
  RUN_TEST(test_webp_file_with_nonexistent_input);
  RUN_TEST(test_webp_file_with_null_config);
  RUN_TEST(test_webp_file_with_null_input_path);
  RUN_TEST(test_webp_file_with_null_output_path);
  RUN_TEST(test_webp_file_with_valid_png);
  RUN_TEST(test_webp_file_with_rgba64_png);
#endif
  RUN_TEST(test_webp_free_with_null);
  RUN_TEST(test_webp_free_with_valid_pointer);
  RUN_TEST(test_webp_last_error);
  RUN_TEST(test_webp_memory_alpha_compression_filtering_variations);
  RUN_TEST(test_webp_memory_alpha_quality_variations);
  RUN_TEST(test_webp_memory_emulate_jpeg_size_variations);
  RUN_TEST(test_webp_memory_lossless_mode);
  RUN_TEST(test_webp_memory_method_variations);
  RUN_TEST(test_webp_memory_near_lossless_variations);
  RUN_TEST(test_webp_memory_output_not_smaller_error);
  RUN_TEST(test_webp_memory_partitions_segments_variations);
  RUN_TEST(test_webp_memory_quality_variations);
  RUN_TEST(test_webp_memory_target_size_psnr_ignored);
  RUN_TEST(test_webp_memory_use_sharp_yuv_variations);
  RUN_TEST(test_webp_memory_with_null_config);
  RUN_TEST(test_webp_memory_with_null_data_ptr);
  RUN_TEST(test_webp_memory_with_null_png_data);
  RUN_TEST(test_webp_memory_with_null_size_ptr);
  RUN_TEST(test_webp_memory_with_oversized_input);
  RUN_TEST(test_webp_memory_with_valid_png);
  RUN_TEST(test_webp_memory_with_rgba64_png);
  RUN_TEST(test_webp_memory_with_zero_size);

  return UNITY_END();
}
