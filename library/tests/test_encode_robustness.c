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

#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <colopresso.h>

#include <unity.h>

#include "test.h"

#define TRUNCATED_PNG_SIZE        2048
#define PNG_IHDR_COLOR_TYPE       25
#define PNG_COLOR_TYPE_INDEXED    3
#define PROTECTED_COLOR_LIMIT     256
#define TRUNCATED_PNG_OUTPUT_FILE "test_encode_robustness_truncated.png"

static uint8_t *g_png_data = NULL;
static size_t g_png_size = 0;

static void load_asset(void)
{
    if (g_png_data) {
        return;
    }

    g_png_data = load_test_asset_png("128x128.png", &g_png_size);
    TEST_ASSERT_NOT_NULL(g_png_data);
    TEST_ASSERT_GREATER_THAN_size_t(TRUNCATED_PNG_SIZE, g_png_size);
}

void setUp(void)
{
    load_asset();
}

void tearDown(void)
{
}

void test_encode_truncated_png_memory(void)
{
    cpres_config_t config;
    uint8_t *out;
    size_t out_size;

    cpres_config_init_defaults(&config);

    out = NULL;
    TEST_ASSERT_EQUAL_INT(
        CPRES_ERROR_INVALID_PNG,
        cpres_encode_webp_memory(g_png_data, TRUNCATED_PNG_SIZE, &out, &out_size, &config));
    TEST_ASSERT_NULL(out);

    out = NULL;
    TEST_ASSERT_EQUAL_INT(
        CPRES_ERROR_INVALID_PNG,
        cpres_encode_avif_memory(g_png_data, TRUNCATED_PNG_SIZE, &out, &out_size, &config));
    TEST_ASSERT_NULL(out);

    out = NULL;
    TEST_ASSERT_NOT_EQUAL_INT(CPRES_OK, cpres_encode_pngx_memory(g_png_data, TRUNCATED_PNG_SIZE,
                                                                 &out, &out_size, &config));
    TEST_ASSERT_NULL(out);
}

#ifndef COLOPRESSO_DISABLE_FILE_OPS
void test_encode_truncated_png_file(void)
{
    cpres_config_t config;
    FILE *fp;

    cpres_config_init_defaults(&config);

    fp = fopen(TRUNCATED_PNG_OUTPUT_FILE, "wb");
    TEST_ASSERT_NOT_NULL(fp);
    TEST_ASSERT_EQUAL_size_t(TRUNCATED_PNG_SIZE, fwrite(g_png_data, 1, TRUNCATED_PNG_SIZE, fp));
    TEST_ASSERT_EQUAL_INT(0, fclose(fp));

    TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PNG,
                          cpres_encode_webp_file(TRUNCATED_PNG_OUTPUT_FILE,
                                                 TRUNCATED_PNG_OUTPUT_FILE ".webp", &config));
    TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PNG,
                          cpres_encode_avif_file(TRUNCATED_PNG_OUTPUT_FILE,
                                                 TRUNCATED_PNG_OUTPUT_FILE ".avif", &config));

    remove(TRUNCATED_PNG_OUTPUT_FILE);
}
#endif

void test_encode_non_finite_quality(void)
{
    cpres_config_t config;
    uint8_t *out;
    size_t out_size;

    cpres_config_init_defaults(&config);
    config.webp_quality = NAN;

    out = NULL;
    TEST_ASSERT_EQUAL_INT(
        CPRES_ERROR_INVALID_PARAMETER,
        cpres_encode_webp_memory(g_png_data, g_png_size, &out, &out_size, &config));
    TEST_ASSERT_NULL(out);

    cpres_config_init_defaults(&config);
    config.webp_target_psnr = INFINITY;
    TEST_ASSERT_EQUAL_INT(
        CPRES_ERROR_INVALID_PARAMETER,
        cpres_encode_webp_memory(g_png_data, g_png_size, &out, &out_size, &config));
    TEST_ASSERT_NULL(out);

    cpres_config_init_defaults(&config);
    config.avif_quality = NAN;
    TEST_ASSERT_EQUAL_INT(
        CPRES_ERROR_INVALID_PARAMETER,
        cpres_encode_avif_memory(g_png_data, g_png_size, &out, &out_size, &config));
    TEST_ASSERT_NULL(out);
}

void test_encode_pngx_excessive_threads(void)
{
    cpres_config_t config;
    uint8_t *expected, *out;
    size_t expected_size, out_size;

    cpres_config_init_defaults(&config);
    config.pngx_threads = 1;

    expected = NULL;
    TEST_ASSERT_EQUAL_INT(CPRES_OK, cpres_encode_pngx_memory(g_png_data, g_png_size, &expected,
                                                             &expected_size, &config));

    config.pngx_threads = INT_MAX;

    out = NULL;
    TEST_ASSERT_EQUAL_INT(
        CPRES_OK, cpres_encode_pngx_memory(g_png_data, g_png_size, &out, &out_size, &config));
    TEST_ASSERT_EQUAL_size_t(expected_size, out_size);
    TEST_ASSERT_EQUAL_MEMORY(expected, out, out_size);

    cpres_free(out);
    cpres_free(expected);
}

void test_encode_pngx_non_finite_smooth_cutoff(void)
{
    cpres_config_t config;
    uint8_t *out;
    size_t out_size;

    cpres_config_init_defaults(&config);
    config.pngx_postprocess_smooth_importance_cutoff = NAN;

    out = NULL;
    TEST_ASSERT_EQUAL_INT(
        CPRES_OK, cpres_encode_pngx_memory(g_png_data, g_png_size, &out, &out_size, &config));
    TEST_ASSERT_NOT_NULL(out);

    cpres_free(out);
}

void test_encode_pngx_protected_color_limit(void)
{
    cpres_rgba_color_t colors[PROTECTED_COLOR_LIMIT];
    cpres_config_t config;
    uint32_t i;
    uint8_t *out;
    size_t out_size;

    for (i = 0; i < PROTECTED_COLOR_LIMIT; ++i) {
        colors[i].r = (uint8_t)i;
        colors[i].g = (uint8_t)(255 - i);
        colors[i].b = (uint8_t)((i * 7) & 0xff);
        colors[i].a = 255;
    }

    cpres_config_init_defaults(&config);
    config.pngx_protected_colors = colors;
    config.pngx_protected_colors_count = PROTECTED_COLOR_LIMIT;

    out = NULL;
    TEST_ASSERT_EQUAL_INT(
        CPRES_OK, cpres_encode_pngx_memory(g_png_data, g_png_size, &out, &out_size, &config));
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_GREATER_THAN_size_t(PNG_IHDR_COLOR_TYPE, out_size);
    TEST_ASSERT_EQUAL_UINT8(PNG_COLOR_TYPE_INDEXED, out[PNG_IHDR_COLOR_TYPE]);

    cpres_free(out);
}

int main(void)
{
    int result;

    UNITY_BEGIN();

    RUN_TEST(test_encode_truncated_png_memory);
#ifndef COLOPRESSO_DISABLE_FILE_OPS
    RUN_TEST(test_encode_truncated_png_file);
#endif
    RUN_TEST(test_encode_non_finite_quality);
    RUN_TEST(test_encode_pngx_excessive_threads);
    RUN_TEST(test_encode_pngx_non_finite_smooth_cutoff);
    RUN_TEST(test_encode_pngx_protected_color_limit);

    result = UNITY_END();
    free(g_png_data);

    return result;
}
