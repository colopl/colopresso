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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <colopresso.h>

#include <unity.h>

#include "../src/internal/log.h"
#include "test.h"

#ifndef COLOPRESSO_TEST_ASSETS_DIR
#define COLOPRESSO_TEST_ASSETS_DIR "./assets"
#endif

void setUp(void) { cpres_set_log_callback(NULL); }

void tearDown(void) { cpres_set_log_callback(NULL); }

void test_log_callback_and_truncate(void) {
  test_log_capture_t capture;
  char buf[5000];

  cpres_log_init();
  test_log_capture_begin(&capture);
  memset(buf, 'A', sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';
  cpres_log(CPRES_LOG_LEVEL_INFO, "%s", buf);

  TEST_ASSERT_EQUAL(CPRES_LOG_LEVEL_INFO, capture.last_level);
  printf("%s\n", capture.message);
  TEST_ASSERT(strstr(capture.message, "truncated") != NULL);

  test_log_capture_end();
}

void test_log_set_callback_multiple_times(void) {
  test_log_capture_t capture;
  uint8_t *webp_data = NULL, invalid_data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  size_t webp_size = 0;
  cpres_config_t config;

  test_log_capture_begin(&capture);
  cpres_set_log_callback(common_log_capture_callback);
  cpres_config_init_defaults(&config);
  cpres_encode_webp_memory(invalid_data, sizeof(invalid_data), &webp_data, &webp_size, &config);

  TEST_ASSERT_TRUE(capture.called);
  test_log_capture_end();
}

void test_log_set_callback_with_null(void) {
  test_log_capture_t capture;
  uint8_t *webp_data = NULL, invalid_data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  size_t webp_size = 0;
  cpres_config_t config;

  test_log_capture_begin(&capture);
  test_log_capture_end();
  cpres_config_init_defaults(&config);
  cpres_encode_webp_memory(invalid_data, sizeof(invalid_data), &webp_data, &webp_size, &config);

  TEST_ASSERT_FALSE(capture.called);
}

void test_log_set_callback_with_valid_callback(void) {
  test_log_capture_t capture;
  uint8_t *webp_data = NULL, invalid_data[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
  size_t webp_size = 0;
  cpres_config_t config;

  test_log_capture_begin(&capture);
  cpres_config_init_defaults(&config);
  cpres_encode_webp_memory(invalid_data, sizeof(invalid_data), &webp_data, &webp_size, &config);

  TEST_ASSERT_TRUE(capture.called);
  test_log_capture_end();
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_log_callback_and_truncate);
  RUN_TEST(test_log_set_callback_multiple_times);
  RUN_TEST(test_log_set_callback_with_null);
  RUN_TEST(test_log_set_callback_with_valid_callback);

  return UNITY_END();
}
