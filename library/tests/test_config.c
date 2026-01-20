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

#include <string.h>

#include <colopresso.h>

#include <unity.h>

void setUp(void) {}

void tearDown(void) {}

void test_config_advanced_options(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_use_sharp_yuv = true;
  config.webp_use_delta_palette = true;
  config.webp_exact = true;
  config.webp_low_memory = true;

  TEST_ASSERT_TRUE(config.webp_use_sharp_yuv);
  TEST_ASSERT_TRUE(config.webp_use_delta_palette);
  TEST_ASSERT_TRUE(config.webp_exact);
  TEST_ASSERT_TRUE(config.webp_low_memory);
}

void test_config_alpha_settings(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_alpha_compression = false;
  config.webp_alpha_filtering = 2;
  config.webp_alpha_quality = 50;

  TEST_ASSERT_FALSE(config.webp_alpha_compression);
  TEST_ASSERT_EQUAL_INT(2, config.webp_alpha_filtering);
  TEST_ASSERT_EQUAL_INT(50, config.webp_alpha_quality);
}

void test_config_emulate_jpeg_size(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_emulate_jpeg_size = true;

  TEST_ASSERT_TRUE(config.webp_emulate_jpeg_size);

  config.webp_emulate_jpeg_size = false;

  TEST_ASSERT_FALSE(config.webp_emulate_jpeg_size);
}

void test_config_filter_settings(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_filter_strength = 0;

  TEST_ASSERT_EQUAL_INT(0, config.webp_filter_strength);

  config.webp_filter_strength = 100;

  TEST_ASSERT_EQUAL_INT(100, config.webp_filter_strength);

  config.webp_filter_sharpness = 0;

  TEST_ASSERT_EQUAL_INT(0, config.webp_filter_sharpness);

  config.webp_filter_sharpness = 7;

  TEST_ASSERT_EQUAL_INT(7, config.webp_filter_sharpness);

  config.webp_filter_type = 0;

  TEST_ASSERT_EQUAL_INT(0, config.webp_filter_type);

  config.webp_filter_type = 1;

  TEST_ASSERT_EQUAL_INT(1, config.webp_filter_type);

  config.webp_autofilter = false;

  TEST_ASSERT_FALSE(config.webp_autofilter);
}

void test_config_init(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);

  TEST_ASSERT_EQUAL_FLOAT(COLOPRESSO_WEBP_DEFAULT_QUALITY, config.webp_quality);
  TEST_ASSERT_FALSE(config.webp_lossless);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_METHOD, config.webp_method);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_TARGET_SIZE, config.webp_target_size);
  TEST_ASSERT_EQUAL_FLOAT(COLOPRESSO_WEBP_DEFAULT_TARGET_PSNR, config.webp_target_psnr);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_SEGMENTS, config.webp_segments);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_SNS_STRENGTH, config.webp_sns_strength);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_FILTER_STRENGTH, config.webp_filter_strength);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_FILTER_SHARPNESS, config.webp_filter_sharpness);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_FILTER_TYPE, config.webp_filter_type);
  TEST_ASSERT_TRUE(config.webp_autofilter);
  TEST_ASSERT_TRUE(config.webp_alpha_compression);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_ALPHA_FILTERING, config.webp_alpha_filtering);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_ALPHA_QUALITY, config.webp_alpha_quality);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_PASS, config.webp_pass);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_PREPROCESSING, config.webp_preprocessing);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_PARTITIONS, config.webp_partitions);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_PARTITION_LIMIT, config.webp_partition_limit);
  TEST_ASSERT_FALSE(config.webp_emulate_jpeg_size);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_THREAD_LEVEL, config.webp_thread_level);
  TEST_ASSERT_FALSE(config.webp_low_memory);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_NEAR_LOSSLESS, config.webp_near_lossless);
  TEST_ASSERT_FALSE(config.webp_exact);
  TEST_ASSERT_FALSE(config.webp_use_delta_palette);
  TEST_ASSERT_FALSE(config.webp_use_sharp_yuv);
}

void test_config_init_defaults(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);

  TEST_ASSERT_EQUAL_FLOAT(COLOPRESSO_WEBP_DEFAULT_QUALITY, config.webp_quality);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_METHOD, config.webp_method);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_SEGMENTS, config.webp_segments);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_SNS_STRENGTH, config.webp_sns_strength);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_FILTER_STRENGTH, config.webp_filter_strength);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_FILTER_SHARPNESS, config.webp_filter_sharpness);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_FILTER_TYPE, config.webp_filter_type);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_ALPHA_FILTERING, config.webp_alpha_filtering);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_ALPHA_QUALITY, config.webp_alpha_quality);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_PASS, config.webp_pass);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_PREPROCESSING, config.webp_preprocessing);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_PARTITIONS, config.webp_partitions);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_PARTITION_LIMIT, config.webp_partition_limit);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_WEBP_DEFAULT_NEAR_LOSSLESS, config.webp_near_lossless);

  TEST_ASSERT_FALSE(config.webp_lossless);
  TEST_ASSERT_TRUE(config.webp_autofilter);
  TEST_ASSERT_TRUE(config.webp_alpha_compression);
  TEST_ASSERT_FALSE(config.webp_emulate_jpeg_size);
  TEST_ASSERT_FALSE(config.webp_low_memory);
  TEST_ASSERT_FALSE(config.webp_exact);
  TEST_ASSERT_FALSE(config.webp_use_delta_palette);
  TEST_ASSERT_FALSE(config.webp_use_sharp_yuv);

  TEST_ASSERT_EQUAL_FLOAT(COLOPRESSO_AVIF_DEFAULT_QUALITY, config.avif_quality);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_AVIF_DEFAULT_ALPHA_QUALITY, config.avif_alpha_quality);
  TEST_ASSERT_FALSE(config.avif_lossless);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_AVIF_DEFAULT_SPEED, config.avif_speed);
  TEST_ASSERT_EQUAL_INT(COLOPRESSO_AVIF_DEFAULT_THREADS, config.avif_threads);
}

void test_config_init_null(void) { cpres_config_init_defaults(NULL); }

void test_config_lossless_mode(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_lossless = true;

  TEST_ASSERT_TRUE(config.webp_lossless);
}

void test_config_method_range(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_method = 0;

  TEST_ASSERT_EQUAL_INT(0, config.webp_method);

  config.webp_method = 6;

  TEST_ASSERT_EQUAL_INT(6, config.webp_method);
}

void test_config_near_lossless_range(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_near_lossless = 0;

  TEST_ASSERT_EQUAL_INT(0, config.webp_near_lossless);

  config.webp_near_lossless = 100;

  TEST_ASSERT_EQUAL_INT(100, config.webp_near_lossless);
}

void test_config_partitions_settings(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_partitions = 0;

  TEST_ASSERT_EQUAL_INT(0, config.webp_partitions);

  config.webp_partitions = 3;

  TEST_ASSERT_EQUAL_INT(3, config.webp_partitions);

  config.webp_partition_limit = 50;

  TEST_ASSERT_EQUAL_INT(50, config.webp_partition_limit);
}

void test_config_pass_range(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_pass = 1;

  TEST_ASSERT_EQUAL_INT(1, config.webp_pass);

  config.webp_pass = 10;

  TEST_ASSERT_EQUAL_INT(10, config.webp_pass);

  config.webp_pass = 5;

  TEST_ASSERT_EQUAL_INT(5, config.webp_pass);
}

void test_config_preprocessing_settings(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_preprocessing = 0;

  TEST_ASSERT_EQUAL_INT(0, config.webp_preprocessing);

  config.webp_preprocessing = 1;

  TEST_ASSERT_EQUAL_INT(1, config.webp_preprocessing);

  config.webp_preprocessing = 2;

  TEST_ASSERT_EQUAL_INT(2, config.webp_preprocessing);
}

void test_config_quality_range(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_quality = 0.0f;

  TEST_ASSERT_EQUAL_FLOAT(0.0f, config.webp_quality);

  config.webp_quality = 100.0f;

  TEST_ASSERT_EQUAL_FLOAT(100.0f, config.webp_quality);

  config.webp_quality = 50.0f;

  TEST_ASSERT_EQUAL_FLOAT(50.0f, config.webp_quality);
}

void test_config_segments_range(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_segments = 1;

  TEST_ASSERT_EQUAL_INT(1, config.webp_segments);

  config.webp_segments = 4;

  TEST_ASSERT_EQUAL_INT(4, config.webp_segments);
}

void test_config_sns_strength_range(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_sns_strength = 0;

  TEST_ASSERT_EQUAL_INT(0, config.webp_sns_strength);

  config.webp_sns_strength = 100;

  TEST_ASSERT_EQUAL_INT(100, config.webp_sns_strength);
}

void test_config_target_settings(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_target_size = 1024;

  TEST_ASSERT_EQUAL_INT(1024, config.webp_target_size);

  config.webp_target_psnr = 42.5f;

  TEST_ASSERT_EQUAL_FLOAT(42.5f, config.webp_target_psnr);
}

void test_config_thread_level(void) {
  cpres_config_t config;

  cpres_config_init_defaults(&config);
  config.webp_thread_level = 1;

  TEST_ASSERT_EQUAL_INT(1, config.webp_thread_level);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_config_advanced_options);
  RUN_TEST(test_config_alpha_settings);
  RUN_TEST(test_config_emulate_jpeg_size);
  RUN_TEST(test_config_filter_settings);
  RUN_TEST(test_config_init);
  RUN_TEST(test_config_init_defaults);
  RUN_TEST(test_config_init_null);
  RUN_TEST(test_config_lossless_mode);
  RUN_TEST(test_config_method_range);
  RUN_TEST(test_config_near_lossless_range);
  RUN_TEST(test_config_partitions_settings);
  RUN_TEST(test_config_pass_range);
  RUN_TEST(test_config_preprocessing_settings);
  RUN_TEST(test_config_quality_range);
  RUN_TEST(test_config_segments_range);
  RUN_TEST(test_config_sns_strength_range);
  RUN_TEST(test_config_target_settings);
  RUN_TEST(test_config_thread_level);

  return UNITY_END();
}
