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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <colopresso.h>

#include <unity.h>

#include "../src/internal/png.h"
#include "../src/internal/pngx.h"
#include "../src/internal/pngx_common.h"

#include "test.h"

static cpres_config_t g_config;

static void fill_rgba_grid_2bit_unique(uint8_t *rgba, uint32_t width, uint32_t height) {
  static const uint8_t levels[4] = {0, 85, 170, 255};
  uint32_t x, y;
  size_t idx, base;
  uint32_t v;

  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      idx = (size_t)y * (size_t)width + (size_t)x;
      base = idx * PNGX_RGBA_CHANNELS;
      v = (uint32_t)idx;

      rgba[base + 0] = levels[v & 3u];
      rgba[base + 1] = levels[(v >> 2) & 3u];
      rgba[base + 2] = levels[(v >> 4) & 3u];
      rgba[base + 3] = levels[(v >> 6) & 3u];
    }
  }
}

static inline uint8_t quantize_to_bits_for_test(uint8_t value, uint8_t bits) {
  uint32_t levels;
  double scaled, rounded, quantized;

  if (bits >= 8) {
    return value;
  }

  levels = 1U << bits;
  scaled = ((double)value * (double)(levels - 1U)) / 255.0;

  rounded = floor(scaled + 0.5);
  if (rounded < 0.0) {
    rounded = 0.0;
  } else if (rounded > (double)(levels - 1U)) {
    rounded = (double)(levels - 1U);
  }

  quantized = rounded * 255.0 / (double)(levels - 1U);
  if (quantized < 0.0) {
    quantized = 0.0;
  } else if (quantized > 255.0) {
    quantized = 255.0;
  }

  return (uint8_t)(quantized + 0.5);
}

void setUp(void) {
  cpres_config_init_defaults(&g_config);
  g_config.pngx_level = 1;
}

void tearDown(void) { release_cached_example_png(); }

void test_pngx_reduced_rgba32_manual_target(void) {
  const size_t target_colors = 64;
  size_t png_size = 0, pngx_size = 0, unique_colors = 0;
  uint8_t *png_data = NULL, *pngx_data = NULL, *rgba = NULL;
  png_uint_32 width = 0, height = 0;
  bool counted = false;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("example_reduce.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example_reduce.png not found for Reduced RGBA32 manual test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;
  g_config.pngx_lossy_reduced_colors = (int)target_colors;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  error = png_decode_from_memory(pngx_data, pngx_size, &rgba, &width, &height);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(rgba);
  TEST_ASSERT_GREATER_THAN(0, width);
  TEST_ASSERT_GREATER_THAN(0, height);

  counted = count_unique_rgba_colors(rgba, (size_t)width * (size_t)height, &unique_colors);
  TEST_ASSERT_TRUE_MESSAGE(counted, "failed to count unique colors");
  TEST_ASSERT_GREATER_THAN_size_t(0, unique_colors);
  TEST_ASSERT_LESS_OR_EQUAL_size_t(target_colors, unique_colors);

  free(rgba);
  cpres_free(pngx_data);
  free(png_data);
}

void test_pngx_reduced_rgba32_auto_target(void) {
  size_t png_size = 0, pngx_size = 0, unique_in = 0, unique_out = 0;
  uint8_t *png_data = NULL, *pngx_data = NULL, *src_rgba = NULL, *dst_rgba = NULL;
  png_uint_32 width = 0, height = 0;
  bool counted = false;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("example_reduce.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example_reduce.png not found for Reduced RGBA32 auto test");

  error = png_decode_from_memory(png_data, png_size, &src_rgba, &width, &height);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  counted = count_unique_rgba_colors(src_rgba, (size_t)width * (size_t)height, &unique_in);
  TEST_ASSERT_TRUE(counted);
  TEST_ASSERT_GREATER_THAN_size_t(0, unique_in);

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;
  g_config.pngx_lossy_reduced_colors = COLOPRESSO_PNGX_DEFAULT_REDUCED_COLORS;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  error = png_decode_from_memory(pngx_data, pngx_size, &dst_rgba, &width, &height);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  counted = count_unique_rgba_colors(dst_rgba, (size_t)width * (size_t)height, &unique_out);
  TEST_ASSERT_TRUE(counted);
  TEST_ASSERT_GREATER_THAN_size_t(0, unique_out);
  TEST_ASSERT_LESS_OR_EQUAL_size_t(unique_in, unique_out);

  free(src_rgba);
  free(dst_rgba);
  cpres_free(pngx_data);
  free(png_data);
}

void test_pngx_reduced_rgba32_grid_bits(void) {
  const uint8_t rgb_bits = 4, alpha_bits = 4;
  uint8_t *png_data = NULL, *pngx_data = NULL, *rgba = NULL, r, g, b, a;
  size_t png_size = 0, pngx_size = 0, pixel_count, base, i;
  png_uint_32 width = 0, height = 0;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("example_reduce.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "example_reduce.png not found for Reduced RGBA32 grid test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;
  g_config.pngx_lossy_reduced_colors = 2048;
  g_config.pngx_lossy_reduced_bits_rgb = rgb_bits;
  g_config.pngx_lossy_reduced_alpha_bits = alpha_bits;
  g_config.pngx_lossy_dither_level = -1.0f;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  error = png_decode_from_memory(pngx_data, pngx_size, &rgba, &width, &height);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(rgba);
  TEST_ASSERT_GREATER_THAN(0, width);
  TEST_ASSERT_GREATER_THAN(0, height);

  pixel_count = (size_t)width * (size_t)height;
  for (i = 0; i < pixel_count; ++i) {
    base = i * 4;
    r = rgba[base + 0];
    g = rgba[base + 1];
    b = rgba[base + 2];
    a = rgba[base + 3];

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(r, rgb_bits), r, "R out of grid");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(g, rgb_bits), g, "G out of grid");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(b, rgb_bits), b, "B out of grid");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(a, alpha_bits), a, "Alpha out of grid");
  }

  free(rgba);
  cpres_free(pngx_data);
  free(png_data);
}

void test_pngx_reduced_rgba32_zero_dither_no_dither_quantization(void) {
  static const uint8_t source_rgba[] = {
      5, 17, 249, 255, 123, 210, 55, 255, 200, 50, 10, 200, 10, 220, 230, 80,
  };
  const uint32_t width = 2, height = 2;
  const uint8_t rgb_bits = 3, alpha_bits = 2;
  const size_t pixel_count = sizeof(source_rgba) / PNGX_RGBA_CHANNELS;
  uint8_t *png_data = NULL, *pngx_data = NULL, *decoded = NULL, r, g, b, a;
  size_t base, png_size = 0, pngx_size = 0, i;
  bool written;
  png_uint_32 decoded_width = 0, decoded_height = 0;
  cpres_error_t error;

  written = create_rgba_png(source_rgba, pixel_count, width, height, &png_data, &png_size);
  TEST_ASSERT_TRUE_MESSAGE(written, "failed to create synthetic RGBA PNG");
  TEST_ASSERT_NOT_NULL(png_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, png_size);

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;
  g_config.pngx_lossy_reduced_colors = COLOPRESSO_PNGX_DEFAULT_REDUCED_COLORS;
  g_config.pngx_lossy_reduced_bits_rgb = rgb_bits;
  g_config.pngx_lossy_reduced_alpha_bits = alpha_bits;
  g_config.pngx_lossy_dither_level = 0.0f;
  g_config.pngx_saliency_map_enable = false;
  g_config.pngx_gradient_boost_enable = false;
  g_config.pngx_chroma_weight_enable = false;
  g_config.pngx_postprocess_smooth_enable = false;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  error = png_decode_from_memory(pngx_data, pngx_size, &decoded, &decoded_width, &decoded_height);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(decoded);
  TEST_ASSERT_EQUAL_UINT32(width, decoded_width);
  TEST_ASSERT_EQUAL_UINT32(height, decoded_height);
  TEST_ASSERT_EQUAL_size_t(sizeof(source_rgba), (size_t)decoded_width * (size_t)decoded_height * PNGX_RGBA_CHANNELS);

  for (i = 0; i < pixel_count; ++i) {
    base = i * PNGX_RGBA_CHANNELS;
    r = decoded[base + 0];
    g = decoded[base + 1];
    b = decoded[base + 2];
    a = decoded[base + 3];

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(r, rgb_bits), r, "R not aligned to simple grid");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(g, rgb_bits), g, "G not aligned to simple grid");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(b, rgb_bits), b, "B not aligned to simple grid");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(a, alpha_bits), a, "Alpha not aligned to simple grid");
  }

  free(decoded);
  cpres_free(pngx_data);
  free(png_data);
}

void test_pngx_reduced_rgba32_with_rgba64_png(void) {
  size_t png_size = 0, pngx_size = 0;
  uint8_t *png_data = NULL, *pngx_data = NULL;
  cpres_error_t error = CPRES_OK;

  png_data = load_test_asset_png("16bit.png", &png_size);
  TEST_ASSERT_NOT_NULL_MESSAGE(png_data, "16bit.png not found for Reduced RGBA32 RGBA64 test");

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;

  error = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, &g_config);

  TEST_ASSERT_EQUAL_INT(CPRES_OK, error);
  TEST_ASSERT_NOT_NULL(pngx_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, pngx_size);

  cpres_free(pngx_data);
  free(png_data);
}

void test_pngx_reduced_rgba32_grid_passthrough_auto_target(void) {
  const uint32_t width = 16, height = 16;
  const uint8_t rgb_bits = 2, alpha_bits = 2;
  const size_t pixel_count = (size_t)width * (size_t)height;
  uint32_t resolved_target = 0, applied_colors = 0;
  uint8_t *rgba = NULL, *png_data = NULL, *out_png = NULL, *decoded = NULL, r, g, b, a;
  size_t png_size = 0, out_size = 0, unique_out = 0, i, base;
  png_uint_32 decoded_width = 0, decoded_height = 0;
  pngx_options_t opts;
  bool written, success, counted;
  cpres_error_t decode_error;

  rgba = (uint8_t *)malloc(pixel_count * PNGX_RGBA_CHANNELS);
  TEST_ASSERT_NOT_NULL(rgba);

  fill_rgba_grid_2bit_unique(rgba, width, height);

  written = create_rgba_png(rgba, pixel_count, width, height, &png_data, &png_size);
  free(rgba);
  TEST_ASSERT_TRUE_MESSAGE(written, "failed to create RGBA PNG for passthrough test");
  TEST_ASSERT_NOT_NULL(png_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, png_size);

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;
  g_config.pngx_lossy_reduced_colors = 0;
  g_config.pngx_lossy_reduced_bits_rgb = rgb_bits;
  g_config.pngx_lossy_reduced_alpha_bits = alpha_bits;
  g_config.pngx_lossy_dither_level = 0.0f;
  g_config.pngx_saliency_map_enable = false;
  g_config.pngx_chroma_anchor_enable = false;
  g_config.pngx_adaptive_dither_enable = false;
  g_config.pngx_gradient_boost_enable = false;
  g_config.pngx_chroma_weight_enable = false;
  g_config.pngx_postprocess_smooth_enable = false;

  memset(&opts, 0, sizeof(opts));
  pngx_fill_pngx_options(&opts, &g_config);

  opts.lossy_reduced_colors = 0;

  success = pngx_quantize_reduced_rgba32(png_data, png_size, &opts, &resolved_target, &applied_colors, &out_png, &out_size);
  free(png_data);

  TEST_ASSERT_TRUE_MESSAGE(success, "pngx_quantize_reduced_rgba32 failed in passthrough test");
  TEST_ASSERT_NOT_NULL(out_png);
  TEST_ASSERT_GREATER_THAN_size_t(0, out_size);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(256u, resolved_target, "expected full 2-bit grid capacity in passthrough");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(256u, applied_colors, "expected applied colors to match grid capacity in passthrough");

  decode_error = png_decode_from_memory(out_png, out_size, &decoded, &decoded_width, &decoded_height);
  cpres_free(out_png);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, decode_error);
  TEST_ASSERT_NOT_NULL(decoded);
  TEST_ASSERT_EQUAL_UINT32(width, decoded_width);
  TEST_ASSERT_EQUAL_UINT32(height, decoded_height);

  counted = count_unique_rgba_colors(decoded, (size_t)decoded_width * (size_t)decoded_height, &unique_out);
  TEST_ASSERT_TRUE(counted);
  TEST_ASSERT_EQUAL_size_t(256, unique_out);

  for (i = 0; i < pixel_count; ++i) {
    base = i * PNGX_RGBA_CHANNELS;
    r = decoded[base + 0];
    g = decoded[base + 1];
    b = decoded[base + 2];
    a = decoded[base + 3];

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(r, rgb_bits), r, "R out of 2-bit grid");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(g, rgb_bits), g, "G out of 2-bit grid");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(b, rgb_bits), b, "B out of 2-bit grid");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(quantize_to_bits_for_test(a, alpha_bits), a, "A out of 2-bit grid");
  }

  free(decoded);
}

void test_pngx_reduced_rgba32_auto_trim_applies_on_head_dominant_long_tail_image(void) {
  const uint32_t width = 256, height = 256;
  const uint8_t rgb_bits = 8, alpha_bits = 8;
  const size_t pixel_count = (size_t)width * (size_t)height, base_pixels = pixel_count / 2, tail_pixels = pixel_count - base_pixels, tail_unique = 2500;
  uint32_t resolved_target = 0, applied_colors = 0, v;
  uint8_t *rgba = NULL, *png_data = NULL, *out_png = NULL, *decoded = NULL;
  size_t png_size = 0, out_size = 0, unique_out = 0, i, base;
  png_uint_32 decoded_width = 0, decoded_height = 0;
  pngx_options_t opts;
  bool written, success, counted;
  cpres_error_t decode_error;

  rgba = (uint8_t *)malloc(pixel_count * PNGX_RGBA_CHANNELS);
  TEST_ASSERT_NOT_NULL(rgba);

  for (i = 0; i < base_pixels; ++i) {
    base = i * PNGX_RGBA_CHANNELS;
    rgba[base + 0] = 0;
    rgba[base + 1] = 0;
    rgba[base + 2] = 0;
    rgba[base + 3] = 255;
  }

  for (i = 0; i < tail_pixels; ++i) {
    base = (base_pixels + i) * PNGX_RGBA_CHANNELS;
    v = (uint32_t)((i % tail_unique) + 1u);
    rgba[base + 0] = (uint8_t)(v & 0xffu);
    rgba[base + 1] = (uint8_t)((v >> 8) & 0xffu);
    rgba[base + 2] = (uint8_t)((v >> 16) & 0xffu);
    rgba[base + 3] = 255;
  }

  written = create_rgba_png(rgba, pixel_count, width, height, &png_data, &png_size);
  free(rgba);
  TEST_ASSERT_TRUE_MESSAGE(written, "failed to create RGBA PNG for auto trim test");
  TEST_ASSERT_NOT_NULL(png_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, png_size);

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;
  g_config.pngx_lossy_reduced_colors = 0;
  g_config.pngx_lossy_reduced_bits_rgb = rgb_bits;
  g_config.pngx_lossy_reduced_alpha_bits = alpha_bits;
  g_config.pngx_lossy_dither_level = 0.0f;
  g_config.pngx_saliency_map_enable = false;
  g_config.pngx_chroma_anchor_enable = false;
  g_config.pngx_adaptive_dither_enable = false;
  g_config.pngx_gradient_boost_enable = false;
  g_config.pngx_chroma_weight_enable = false;
  g_config.pngx_postprocess_smooth_enable = false;

  memset(&opts, 0, sizeof(opts));
  pngx_fill_pngx_options(&opts, &g_config);
  opts.lossy_reduced_colors = 0;

  success = pngx_quantize_reduced_rgba32(png_data, png_size, &opts, &resolved_target, &applied_colors, &out_png, &out_size);
  free(png_data);

  TEST_ASSERT_TRUE_MESSAGE(success, "pngx_quantize_reduced_rgba32 failed in auto trim test");
  TEST_ASSERT_NOT_NULL(out_png);
  TEST_ASSERT_GREATER_THAN_size_t(0, out_size);
  TEST_ASSERT_GREATER_THAN_UINT32(0u, resolved_target);
  TEST_ASSERT_GREATER_THAN_UINT32(0u, applied_colors);

  decode_error = png_decode_from_memory(out_png, out_size, &decoded, &decoded_width, &decoded_height);
  cpres_free(out_png);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, decode_error);
  TEST_ASSERT_NOT_NULL(decoded);
  TEST_ASSERT_EQUAL_UINT32(width, decoded_width);
  TEST_ASSERT_EQUAL_UINT32(height, decoded_height);

  counted = count_unique_rgba_colors(decoded, (size_t)decoded_width * (size_t)decoded_height, &unique_out);
  TEST_ASSERT_TRUE(counted);
  TEST_ASSERT_GREATER_THAN_size_t(0, unique_out);
  TEST_ASSERT_LESS_THAN_size_t(tail_unique + 1, unique_out);
  TEST_ASSERT_LESS_OR_EQUAL_UINT32((uint32_t)unique_out, applied_colors);

  free(decoded);
}

void test_pngx_reduced_rgba32_gentle_cut_path_executes_for_1024_unique_colors(void) {
  const uint32_t width = 32, height = 32;
  const size_t pixel_count = (size_t)width * (size_t)height;
  uint32_t resolved_target = 0, applied_colors = 0, v;
  uint8_t *rgba = NULL, *png_data = NULL, *out_png = NULL, *decoded = NULL;
  size_t png_size = 0, out_size = 0, unique_out = 0, i, base;
  png_uint_32 decoded_width = 0, decoded_height = 0;
  pngx_options_t opts;
  bool written, success, counted;
  cpres_error_t decode_error;

  rgba = (uint8_t *)malloc(pixel_count * PNGX_RGBA_CHANNELS);
  TEST_ASSERT_NOT_NULL(rgba);

  for (i = 0; i < pixel_count; ++i) {
    base = i * PNGX_RGBA_CHANNELS;
    v = (uint32_t)i;
    rgba[base + 0] = (uint8_t)(v & 0xffu);
    rgba[base + 1] = (uint8_t)(((v >> 8) & 3u) * 85u);
    rgba[base + 2] = (uint8_t)(255u - (v & 0xffu));
    rgba[base + 3] = 255;
  }

  written = create_rgba_png(rgba, pixel_count, width, height, &png_data, &png_size);
  free(rgba);
  TEST_ASSERT_TRUE_MESSAGE(written, "failed to create RGBA PNG for gentle-cut test");
  TEST_ASSERT_NOT_NULL(png_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, png_size);

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;
  g_config.pngx_lossy_reduced_colors = 0;
  g_config.pngx_lossy_reduced_bits_rgb = 8;
  g_config.pngx_lossy_reduced_alpha_bits = 8;
  g_config.pngx_lossy_dither_level = 0.0f;
  g_config.pngx_saliency_map_enable = false;
  g_config.pngx_chroma_anchor_enable = false;
  g_config.pngx_adaptive_dither_enable = false;
  g_config.pngx_gradient_boost_enable = false;
  g_config.pngx_chroma_weight_enable = false;
  g_config.pngx_postprocess_smooth_enable = false;

  memset(&opts, 0, sizeof(opts));
  pngx_fill_pngx_options(&opts, &g_config);
  opts.lossy_reduced_colors = 0;

  success = pngx_quantize_reduced_rgba32(png_data, png_size, &opts, &resolved_target, &applied_colors, &out_png, &out_size);
  free(png_data);
  TEST_ASSERT_TRUE_MESSAGE(success, "pngx_quantize_reduced_rgba32 failed in gentle-cut test");
  TEST_ASSERT_NOT_NULL(out_png);
  TEST_ASSERT_GREATER_THAN_size_t(0, out_size);
  TEST_ASSERT_GREATER_THAN_UINT32(0u, resolved_target);
  TEST_ASSERT_GREATER_THAN_UINT32(0u, applied_colors);

  TEST_ASSERT_LESS_THAN_UINT32(1024u, resolved_target);

  decode_error = png_decode_from_memory(out_png, out_size, &decoded, &decoded_width, &decoded_height);
  cpres_free(out_png);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, decode_error);
  TEST_ASSERT_NOT_NULL(decoded);
  TEST_ASSERT_EQUAL_UINT32(width, decoded_width);
  TEST_ASSERT_EQUAL_UINT32(height, decoded_height);

  counted = count_unique_rgba_colors(decoded, (size_t)decoded_width * (size_t)decoded_height, &unique_out);
  TEST_ASSERT_TRUE(counted);
  TEST_ASSERT_GREATER_THAN_size_t(0, unique_out);

  free(decoded);
}

void test_pngx_reduced_rgba32_all_colors_protected_results_in_no_unlocked_entries(void) {
  static const uint8_t source_rgba[] = {
      0, 0, 0, 255, 17, 0, 0, 255, 0, 17, 0, 255, 0, 0, 17, 255,
  };
  const uint32_t width = 4, height = 1;
  const size_t pixel_count = sizeof(source_rgba) / PNGX_RGBA_CHANNELS;
  uint32_t resolved_target = 0, applied_colors = 0;
  uint8_t *png_data = NULL, *out_png = NULL, *decoded = NULL;
  size_t png_size = 0, out_size = 0, unique_out = 0;
  png_uint_32 decoded_width = 0, decoded_height = 0;
  pngx_options_t opts;
  bool written, success, counted;
  cpres_error_t decode_error;
  cpres_rgba_color_t protected_colors[4];

  written = create_rgba_png(source_rgba, pixel_count, width, height, &png_data, &png_size);
  TEST_ASSERT_TRUE(written);
  TEST_ASSERT_NOT_NULL(png_data);
  TEST_ASSERT_GREATER_THAN_size_t(0, png_size);

  protected_colors[0].r = 0;
  protected_colors[0].g = 0;
  protected_colors[0].b = 0;
  protected_colors[0].a = 255;
  protected_colors[1].r = 17;
  protected_colors[1].g = 0;
  protected_colors[1].b = 0;
  protected_colors[1].a = 255;
  protected_colors[2].r = 0;
  protected_colors[2].g = 17;
  protected_colors[2].b = 0;
  protected_colors[2].a = 255;
  protected_colors[3].r = 0;
  protected_colors[3].g = 0;
  protected_colors[3].b = 17;
  protected_colors[3].a = 255;

  g_config.pngx_lossy_enable = true;
  g_config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;
  g_config.pngx_lossy_reduced_bits_rgb = 4;
  g_config.pngx_lossy_reduced_alpha_bits = 4;
  g_config.pngx_lossy_dither_level = 0.0f;
  g_config.pngx_saliency_map_enable = false;
  g_config.pngx_chroma_anchor_enable = false;
  g_config.pngx_adaptive_dither_enable = false;
  g_config.pngx_gradient_boost_enable = false;
  g_config.pngx_chroma_weight_enable = false;
  g_config.pngx_postprocess_smooth_enable = false;
  g_config.pngx_protected_colors = protected_colors;
  g_config.pngx_protected_colors_count = 4;

  memset(&opts, 0, sizeof(opts));
  pngx_fill_pngx_options(&opts, &g_config);

  opts.lossy_reduced_colors = 0;

  success = pngx_quantize_reduced_rgba32(png_data, png_size, &opts, &resolved_target, &applied_colors, &out_png, &out_size);
  free(png_data);

  TEST_ASSERT_TRUE(success);
  TEST_ASSERT_NOT_NULL(out_png);
  TEST_ASSERT_GREATER_THAN_size_t(0, out_size);

  TEST_ASSERT_EQUAL_UINT32(4u, resolved_target);
  TEST_ASSERT_EQUAL_UINT32(4u, applied_colors);

  decode_error = png_decode_from_memory(out_png, out_size, &decoded, &decoded_width, &decoded_height);
  cpres_free(out_png);
  TEST_ASSERT_EQUAL_INT(CPRES_OK, decode_error);
  TEST_ASSERT_NOT_NULL(decoded);
  TEST_ASSERT_EQUAL_UINT32(width, decoded_width);
  TEST_ASSERT_EQUAL_UINT32(height, decoded_height);

  counted = count_unique_rgba_colors(decoded, (size_t)decoded_width * (size_t)decoded_height, &unique_out);
  TEST_ASSERT_TRUE(counted);
  TEST_ASSERT_EQUAL_size_t(4, unique_out);

  free(decoded);

  g_config.pngx_protected_colors = NULL;
  g_config.pngx_protected_colors_count = 0;
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_pngx_reduced_rgba32_manual_target);
  RUN_TEST(test_pngx_reduced_rgba32_auto_target);
  RUN_TEST(test_pngx_reduced_rgba32_grid_bits);
  RUN_TEST(test_pngx_reduced_rgba32_zero_dither_no_dither_quantization);
  RUN_TEST(test_pngx_reduced_rgba32_with_rgba64_png);
  RUN_TEST(test_pngx_reduced_rgba32_grid_passthrough_auto_target);
  RUN_TEST(test_pngx_reduced_rgba32_all_colors_protected_results_in_no_unlocked_entries);
  RUN_TEST(test_pngx_reduced_rgba32_auto_trim_applies_on_head_dominant_long_tail_image);
  RUN_TEST(test_pngx_reduced_rgba32_gentle_cut_path_executes_for_1024_unique_colors);

  return UNITY_END();
}
