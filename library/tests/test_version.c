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

#include <colopresso.h>

#include <string.h>

#include <unity.h>

void setUp(void) {}

void tearDown(void) {}

void test_version_get_buildtime(void) {
  uint32_t buildtime = 0;

  buildtime = cpres_get_buildtime();
  (void)buildtime;
}

void test_version_get_libavif_version(void) {
  uint32_t version = 0;

  version = cpres_get_libavif_version();
  TEST_ASSERT_GREATER_THAN(0, version);
}

void test_version_get_libpng_version(void) {
  uint32_t version = 0;

  version = cpres_get_libpng_version();
  TEST_ASSERT_GREATER_THAN(0, version);
}

void test_version_get_libwebp_version(void) {
  uint32_t version = 0;

  version = cpres_get_libwebp_version();
  TEST_ASSERT_GREATER_THAN(0, version);
}

void test_version_get_pngx_libimagequant_version(void) {
  uint32_t version = 0;

  version = cpres_get_pngx_libimagequant_version();
  TEST_ASSERT_GREATER_THAN(0, version);
}

void test_version_get_pngx_oxipng_version(void) {
  uint32_t version = 0;

  version = cpres_get_pngx_oxipng_version();
  TEST_ASSERT_GREATER_THAN(0, version);
}

void test_version_get_version(void) {
  uint32_t version = 0;

  version = cpres_get_version();
  TEST_ASSERT_EQUAL_UINT32(COLOPRESSO_VERSION, version);
}

void test_version_get_compiler_version_string(void) {
  const char *version = NULL;

  version = cpres_get_compiler_version_string();
  TEST_ASSERT_NOT_NULL(version);
  TEST_ASSERT_GREATER_THAN(0, strlen(version));
}

void test_version_get_rust_version_string(void) {
  const char *version = NULL;

  version = cpres_get_rust_version_string();
  TEST_ASSERT_NOT_NULL(version);
  TEST_ASSERT_GREATER_THAN(0, strlen(version));
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_version_get_buildtime);
  RUN_TEST(test_version_get_libavif_version);
  RUN_TEST(test_version_get_libpng_version);
  RUN_TEST(test_version_get_libwebp_version);
  RUN_TEST(test_version_get_pngx_libimagequant_version);
  RUN_TEST(test_version_get_pngx_oxipng_version);
  RUN_TEST(test_version_get_version);
  RUN_TEST(test_version_get_compiler_version_string);
  RUN_TEST(test_version_get_rust_version_string);

  return UNITY_END();
}
