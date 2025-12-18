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

#ifndef COLOPRESSO_DISABLE_FILE_OPS
void test_pngx_file_lossy_mode(void) {
  FILE *fp = NULL;
  char input_path[512], output_path[512];
  cpres_error_t error = CPRES_OK;

  if (snprintf(input_path, sizeof(input_path), "%s/128x128.png", COLOPRESSO_TEST_ASSETS_DIR) < 0) {
    TEST_FAIL_MESSAGE("failed to format PNG input path");
  }
  if (snprintf(output_path, sizeof(output_path), "example_pngx_lossy.png") < 0) {
    TEST_FAIL_MESSAGE("failed to format PNGX output path");
  }

  remove(output_path);

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_max_colors = 128;

  error = cpres_encode_pngx_file(input_path, output_path, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_path, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  if (fp != NULL) {
    fclose(fp);
  }

  remove(output_path);
}

void test_pngx_file_with_nonexistent_input(void) {
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_pngx_file("/nonexistent/file.png", "output_pngx.png", &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_FILE_NOT_FOUND, error);
}

void test_pngx_file_with_null_config(void) {
  char input_path[512];
  cpres_error_t error = CPRES_OK;

  if (snprintf(input_path, sizeof(input_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR) < 0) {
    TEST_FAIL_MESSAGE("failed to format PNG input path");
  }

  error = cpres_encode_pngx_file(input_path, "output_pngx.png", NULL);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_pngx_file_with_null_input_path(void) {
  cpres_error_t error = CPRES_OK;

  error = cpres_encode_pngx_file(NULL, "output_pngx.png", &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PARAMETER, error);
}

void test_pngx_file_with_valid_png(void) {
  FILE *fp = NULL;
  char input_path[512], output_path[512];
  cpres_error_t error = CPRES_OK;

  if (snprintf(input_path, sizeof(input_path), "%s/128x128.png", COLOPRESSO_TEST_ASSETS_DIR) < 0) {
    TEST_FAIL_MESSAGE("failed to format PNG input path");
  }
  if (snprintf(output_path, sizeof(output_path), "example_pngx.png") < 0) {
    TEST_FAIL_MESSAGE("failed to format PNGX output path");
  }

  remove(output_path);

  cpres_set_log_callback(test_default_log_callback);
  error = cpres_encode_pngx_file(input_path, output_path, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_path, "rb");

  TEST_ASSERT_NOT_NULL(fp);
  if (fp != NULL) {
    fclose(fp);
  }

  remove(output_path);
}

void test_pngx_file_with_rgba64_png(void) {
  FILE *fp = NULL;
  char input_path[512], output_path[512];
  cpres_error_t error = CPRES_OK;

  if (!format_test_asset_path(input_path, sizeof(input_path), "16bit.png")) {
    TEST_FAIL_MESSAGE("failed to format RGBA64 PNG input path");
  }
  if (snprintf(output_path, sizeof(output_path), "example_pngx_rgba64.png") < 0) {
    TEST_FAIL_MESSAGE("failed to format RGBA64 PNGX output path");
  }

  remove(output_path);

  error = cpres_encode_pngx_file(input_path, output_path, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);

  fp = fopen(output_path, "rb");
  TEST_ASSERT_NOT_NULL(fp);
  if (fp != NULL) {
    fclose(fp);
  }

  remove(output_path);
}
#endif

void test_pngx_file_dummy(void) { TEST_ASSERT_TRUE(true); }

int main(void) {
  UNITY_BEGIN();

#ifndef COLOPRESSO_DISABLE_FILE_OPS
  RUN_TEST(test_pngx_file_lossy_mode);
  RUN_TEST(test_pngx_file_with_nonexistent_input);
  RUN_TEST(test_pngx_file_with_null_config);
  RUN_TEST(test_pngx_file_with_null_input_path);
  RUN_TEST(test_pngx_file_with_valid_png);
  RUN_TEST(test_pngx_file_with_rgba64_png);
#endif

  RUN_TEST(test_pngx_file_dummy);

  return UNITY_END();
}
