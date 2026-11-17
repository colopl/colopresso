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
#define PROTECTED_COLOR_OVER      300
#define TRUNCATED_PNG_OUTPUT_FILE "test_encode_robustness_truncated.png"
#define HEADER_ONLY_PNG_CAPACITY  128

static const uint8_t g_png_signature[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
/* zlib stream of 16 zero bytes: valid image data, far too short for the declared dimensions */
static const uint8_t g_tiny_idat[11] = {0x78, 0x9c, 0x63, 0x60, 0x40, 0x05,
                                        0x00, 0x00, 0x10, 0x00, 0x01};

static uint8_t *g_png_data = NULL;
static size_t g_png_size = 0;

static inline void put_be32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value >> 24);
    out[1] = (uint8_t)(value >> 16);
    out[2] = (uint8_t)(value >> 8);
    out[3] = (uint8_t)value;
}

static inline uint32_t png_chunk_crc(const uint8_t *data, size_t size)
{
    uint32_t crc, bit;
    size_t i;

    crc = 0xffffffffu;
    for (i = 0; i < size; ++i) {
        crc ^= data[i];
        for (bit = 0; bit < 8; ++bit) {
            crc = (crc & 1u) ? (crc >> 1) ^ 0xedb88320u : crc >> 1;
        }
    }

    return crc ^ 0xffffffffu;
}

static inline size_t append_png_chunk(uint8_t *out, size_t offset, const char *type,
                                      const uint8_t *data, uint32_t length)
{
    put_be32(out + offset, length);
    memcpy(out + offset + 4, type, 4);

    if (length > 0) {
        memcpy(out + offset + 8, data, length);
    }

    put_be32(out + offset + 8 + length, png_chunk_crc(out + offset + 4, (size_t)length + 4));

    return offset + 12 + length;
}

/* An 8-bit RGBA PNG whose IHDR declares width x height, followed by a tiny IDAT. */
static size_t build_header_only_png(uint8_t *out, uint32_t width, uint32_t height)
{
    uint8_t ihdr[13];
    size_t offset;

    put_be32(ihdr, width);
    put_be32(ihdr + 4, height);
    ihdr[8] = 8;
    ihdr[9] = 6;
    ihdr[10] = 0;
    ihdr[11] = 0;
    ihdr[12] = 0;

    memcpy(out, g_png_signature, sizeof(g_png_signature));
    offset = append_png_chunk(out, sizeof(g_png_signature), "IHDR", ihdr, sizeof(ihdr));
    offset = append_png_chunk(out, offset, "IDAT", g_tiny_idat, sizeof(g_tiny_idat));

    return append_png_chunk(out, offset, "IEND", NULL, 0);
}

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

void test_encode_rejects_oversized_header(void)
{
    const uint32_t sizes[3][2] = {{1000000, 1000000}, {65537, 1}, {20000, 20000}};
    cpres_config_t config;
    uint32_t i;
    uint8_t png[HEADER_ONLY_PNG_CAPACITY], *out;
    size_t png_size, out_size;

    cpres_config_init_defaults(&config);

    for (i = 0; i < 3; ++i) {
        png_size = build_header_only_png(png, sizes[i][0], sizes[i][1]);

        out = NULL;
        TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PNG,
                              cpres_encode_webp_memory(png, png_size, &out, &out_size, &config));
        TEST_ASSERT_NULL(out);
        TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PNG,
                              cpres_encode_avif_memory(png, png_size, &out, &out_size, &config));
        TEST_ASSERT_NULL(out);
        TEST_ASSERT_EQUAL_INT(CPRES_ERROR_INVALID_PNG,
                              cpres_encode_pngx_memory(png, png_size, &out, &out_size, &config));
        TEST_ASSERT_NULL(out);
    }
}

void test_encode_pngx_protected_colors_over_limit(void)
{
    cpres_rgba_color_t colors[PROTECTED_COLOR_OVER];
    cpres_config_t config;
    uint32_t i;
    uint8_t *expected, *out;
    size_t expected_size, out_size;

    for (i = 0; i < PROTECTED_COLOR_OVER; ++i) {
        colors[i].r = (uint8_t)i;
        colors[i].g = (uint8_t)(255 - i);
        colors[i].b = (uint8_t)((i * 7) & 0xff);
        colors[i].a = 255;
    }

    cpres_config_init_defaults(&config);
    config.pngx_protected_colors = colors;
    config.pngx_protected_colors_count = PROTECTED_COLOR_LIMIT;

    expected = NULL;
    TEST_ASSERT_EQUAL_INT(CPRES_OK, cpres_encode_pngx_memory(g_png_data, g_png_size, &expected,
                                                             &expected_size, &config));

    config.pngx_protected_colors_count = PROTECTED_COLOR_OVER;

    out = NULL;
    TEST_ASSERT_EQUAL_INT(
        CPRES_OK, cpres_encode_pngx_memory(g_png_data, g_png_size, &out, &out_size, &config));
    TEST_ASSERT_EQUAL_size_t(expected_size, out_size);
    TEST_ASSERT_EQUAL_MEMORY(expected, out, out_size);

    cpres_free(out);
    cpres_free(expected);
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
    RUN_TEST(test_encode_rejects_oversized_header);
    RUN_TEST(test_encode_pngx_protected_colors_over_limit);

    result = UNITY_END();
    free(g_png_data);

    return result;
}
