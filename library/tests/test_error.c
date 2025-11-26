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

#include <string.h>

#include <colopresso.h>

#include <unity.h>

void setUp(void) {}

void tearDown(void) {}

void test_error_code_values(void) {
  TEST_ASSERT_EQUAL_INT(0, CPRES_OK);
  TEST_ASSERT_EQUAL_INT(1, CPRES_ERROR_FILE_NOT_FOUND);
  TEST_ASSERT_EQUAL_INT(2, CPRES_ERROR_INVALID_PNG);
  TEST_ASSERT_EQUAL_INT(3, CPRES_ERROR_INVALID_FORMAT);
  TEST_ASSERT_EQUAL_INT(4, CPRES_ERROR_OUT_OF_MEMORY);
  TEST_ASSERT_EQUAL_INT(5, CPRES_ERROR_ENCODE_FAILED);
  TEST_ASSERT_EQUAL_INT(6, CPRES_ERROR_DECODE_FAILED);
  TEST_ASSERT_EQUAL_INT(7, CPRES_ERROR_IO);
  TEST_ASSERT_EQUAL_INT(8, CPRES_ERROR_INVALID_PARAMETER);
  TEST_ASSERT_EQUAL_INT(9, CPRES_ERROR_OUTPUT_NOT_SMALLER);
}

#ifndef COLOPRESSO_DISABLE_FILE_OPS
void test_error_encode_file_invalid_parameters(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);

  TEST_ASSERT_EQUAL(CPRES_ERROR_INVALID_PARAMETER, cpres_encode_webp_file(NULL, "output.webp", &config));
  TEST_ASSERT_EQUAL(CPRES_ERROR_INVALID_PARAMETER, cpres_encode_webp_file("input.png", NULL, &config));
  TEST_ASSERT_EQUAL(CPRES_ERROR_INVALID_PARAMETER, cpres_encode_webp_file("input.png", "output.webp", NULL));
}
#endif

void test_error_encode_memory_invalid_parameters(void) {
  cpres_config_t config;
  uint8_t *webp_data = NULL, dummy_png[100] = {0};
  size_t webp_size = 0;

  cpres_config_init_defaults(&config);

  TEST_ASSERT_EQUAL(CPRES_ERROR_INVALID_PARAMETER, cpres_encode_webp_memory(NULL, 100, &webp_data, &webp_size, &config));
  TEST_ASSERT_EQUAL(CPRES_ERROR_INVALID_PARAMETER, cpres_encode_webp_memory(dummy_png, 100, NULL, &webp_size, &config));
  TEST_ASSERT_EQUAL(CPRES_ERROR_INVALID_PARAMETER, cpres_encode_webp_memory(dummy_png, 100, &webp_data, NULL, &config));
  TEST_ASSERT_EQUAL(CPRES_ERROR_INVALID_PARAMETER, cpres_encode_webp_memory(dummy_png, 100, &webp_data, &webp_size, NULL));
  TEST_ASSERT_EQUAL(CPRES_ERROR_INVALID_PARAMETER, cpres_encode_webp_memory(dummy_png, 0, &webp_data, &webp_size, &config));
  TEST_ASSERT_EQUAL(CPRES_ERROR_INVALID_PARAMETER, cpres_encode_webp_memory(dummy_png, 1024 * 1024 * 513, &webp_data, &webp_size, &config));
}

void test_error_free_null(void) { cpres_free(NULL); }

void test_error_string_decode_failed(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_ERROR_DECODE_FAILED);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("Decoding failed", error_str);
}

void test_error_string_encode_failed(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_ERROR_ENCODE_FAILED);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("Encoding failed", error_str);
}

void test_error_string_file_not_found(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_ERROR_FILE_NOT_FOUND);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("File not found", error_str);
}

void test_error_string_invalid_parameter(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_ERROR_INVALID_PARAMETER);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("Invalid parameter", error_str);
}

void test_error_string_invalid_png(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_ERROR_INVALID_PNG);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("Invalid PNG file", error_str);
}

void test_error_string_invalid_webp(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_ERROR_INVALID_FORMAT);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("Invalid WebP file", error_str);
}

void test_error_string_io(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_ERROR_IO);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("I/O error", error_str);
}

void test_error_string_ok(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_OK);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("Success", error_str);
}

void test_error_string_out_of_memory(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_ERROR_OUT_OF_MEMORY);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("Out of memory", error_str);
}

void test_error_string_output_not_smaller(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string(CPRES_ERROR_OUTPUT_NOT_SMALLER);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("Output image would be larger than input", error_str);
}

void test_error_string_unknown(void) {
  const char *error_str = NULL;

  error_str = cpres_error_string((cpres_error_t)9999);

  TEST_ASSERT_NOT_NULL(error_str);
  TEST_ASSERT_EQUAL_STRING("Unknown error", error_str);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_error_code_values);
#ifndef COLOPRESSO_DISABLE_FILE_OPS
  RUN_TEST(test_error_encode_file_invalid_parameters);
#endif
  RUN_TEST(test_error_encode_memory_invalid_parameters);
  RUN_TEST(test_error_free_null);
  RUN_TEST(test_error_string_decode_failed);
  RUN_TEST(test_error_string_encode_failed);
  RUN_TEST(test_error_string_file_not_found);
  RUN_TEST(test_error_string_invalid_parameter);
  RUN_TEST(test_error_string_invalid_png);
  RUN_TEST(test_error_string_invalid_webp);
  RUN_TEST(test_error_string_io);
  RUN_TEST(test_error_string_ok);
  RUN_TEST(test_error_string_out_of_memory);
  RUN_TEST(test_error_string_output_not_smaller);
  RUN_TEST(test_error_string_unknown);

  return UNITY_END();
}
