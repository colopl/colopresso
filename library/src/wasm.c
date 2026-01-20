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

#ifdef PNGX_BRIDGE_WASM_SEPARATION

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
  PNGX_BRIDGE_RESULT_SUCCESS = 0,
  PNGX_BRIDGE_RESULT_GENERIC_ERROR = 1,
  PNGX_BRIDGE_RESULT_INVALID_INPUT = 2,
  PNGX_BRIDGE_RESULT_OPTIMIZATION_FAILED = 3,
  PNGX_BRIDGE_RESULT_WASM_SEPARATION = 100
} PngxBridgeResult;

typedef enum { PNGX_BRIDGE_QUANT_OK = 0, PNGX_BRIDGE_QUANT_QUALITY_TOO_LOW = 1, PNGX_BRIDGE_QUANT_ERROR = 2, PNGX_BRIDGE_QUANT_WASM_SEPARATION = 100 } PngxBridgeQuantStatus;

typedef struct {
  uint8_t r, g, b, a;
} cpres_rgba_color_t;

typedef struct {
  int32_t optimization_level;
  bool strip_safe;
  bool optimize_alpha;
} PngxBridgeLosslessOptions;

typedef struct {
  int32_t speed;
  int32_t quality_min;
  int32_t quality_max;
  uint32_t max_colors;
  int32_t min_posterization;
  float dithering_level;
  bool remap;
  const uint8_t *importance_map;
  size_t importance_map_len;
  const cpres_rgba_color_t *fixed_colors;
  size_t fixed_colors_len;
} PngxBridgeQuantParams;

typedef struct {
  cpres_rgba_color_t *palette;
  size_t palette_len;
  uint8_t *indices;
  size_t indices_len;
  int32_t quality;
} PngxBridgeQuantOutput;

PngxBridgeResult pngx_bridge_optimize_lossless(const uint8_t *input_data, size_t input_size, uint8_t **output_data, size_t *output_size, const PngxBridgeLosslessOptions *options) {
  (void)input_data;
  (void)input_size;
  (void)options;
  if (output_data)
    *output_data = NULL;
  if (output_size)
    *output_size = 0;
  return PNGX_BRIDGE_RESULT_WASM_SEPARATION;
}

PngxBridgeQuantStatus pngx_bridge_quantize(const cpres_rgba_color_t *pixels, size_t pixel_count, uint32_t width, uint32_t height, const PngxBridgeQuantParams *params, PngxBridgeQuantOutput *output) {
  (void)pixels;
  (void)pixel_count;
  (void)width;
  (void)height;
  (void)params;
  if (output) {
    output->palette = NULL;
    output->palette_len = 0;
    output->indices = NULL;
    output->indices_len = 0;
    output->quality = -1;
  }
  return PNGX_BRIDGE_QUANT_WASM_SEPARATION;
}

void pngx_bridge_free(uint8_t *ptr) { (void)ptr; }

uint32_t pngx_bridge_oxipng_version(void) { return 0; }

uint32_t pngx_bridge_libimagequant_version(void) { return 0; }

const char *pngx_bridge_rust_version_string(void) { return "unknown"; }

bool pngx_bridge_init_threads(int num_threads) {
  (void)num_threads;
  return true;
}

#endif /* PNGX_BRIDGE_WASM_SEPARATION */
