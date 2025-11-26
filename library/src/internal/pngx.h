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

#ifndef COLOPRESSO_INTERNAL_PNGX_H
#define COLOPRESSO_INTERNAL_PNGX_H

#include <colopresso.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <png.h>

#define PNGX_IMPORTANCE_SCALE 65535.0f
#define PNGX_CHROMA_BUCKET_DIM 16
#define PNGX_CHROMA_BUCKET_BITS 4
#define PNGX_CHROMA_BUCKET_SHIFT (8 - PNGX_CHROMA_BUCKET_BITS)
#define PNGX_CHROMA_BUCKET_COUNT (PNGX_CHROMA_BUCKET_DIM * PNGX_CHROMA_BUCKET_DIM * PNGX_CHROMA_BUCKET_DIM)
#define PNGX_MAX_DERIVED_COLORS 48
#define PNGX_REDUCED_RGBA32_PASSTHROUGH_GRID_DIVISOR 4
#define PNGX_REDUCED_RGBA32_PASSTHROUGH_MIN_COLORS 512
#define PNGX_RGBA_CHANNELS 4
#define PNGX_FULL_CHANNEL_BITS 8
#define PNGX_LIMITED_RGBA4444_BITS 4
#define PNGX_ALPHA_NEAR_TRANSPARENT 8
#define PNGX_ALPHA_MIN_DITHER_FACTOR 0.04f
#define PNGX_VIBRANT_RATIO_LOW 0.04f

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  PNGX_BRIDGE_RESULT_SUCCESS = 0,
  PNGX_BRIDGE_RESULT_INVALID_INPUT = 1,
  PNGX_BRIDGE_RESULT_OPTIMIZATION_FAILED = 2,
  PNGX_BRIDGE_RESULT_IO_ERROR = 3,
} PngxBridgeResult;

typedef enum {
  PNGX_BRIDGE_QUANT_STATUS_OK = 0,
  PNGX_BRIDGE_QUANT_STATUS_QUALITY_TOO_LOW = 1,
  PNGX_BRIDGE_QUANT_STATUS_ERROR = 2,
} PngxBridgeQuantStatus;

typedef enum {
  PNGX_LOSSY_TYPE_PALETTE256 = COLOPRESSO_PNGX_LOSSY_TYPE_PALETTE256,
  PNGX_LOSSY_TYPE_LIMITED_RGBA4444 = COLOPRESSO_PNGX_LOSSY_TYPE_LIMITED_RGBA4444,
  PNGX_LOSSY_TYPE_REDUCED_RGBA32 = COLOPRESSO_PNGX_LOSSY_TYPE_REDUCED_RGBA32,
} pngx_lossy_type_t;

typedef struct {
  uint8_t optimization_level;
  bool strip_safe;
  bool optimize_alpha;
} PngxBridgeOptions;

typedef struct {
  uint8_t optimization_level;
  bool strip_safe;
  bool optimize_alpha;
} PngxBridgeLosslessOptions;

typedef struct {
  int32_t speed;
  uint8_t quality_min;
  uint8_t quality_max;
  uint32_t max_colors;
  int32_t min_posterization;
  float dithering_level;
  const uint8_t *importance_map;
  size_t importance_map_len;
  const cpres_rgba_color_t *fixed_colors;
  size_t fixed_colors_len;
  bool remap;
} PngxBridgeQuantParams;

typedef struct {
  cpres_rgba_color_t *palette;
  size_t palette_len;
  uint8_t *indices;
  size_t indices_len;
  int32_t quality;
} PngxBridgeQuantOutput;

typedef struct {
  PngxBridgeOptions bridge;
  bool lossy_enable;
  uint8_t lossy_type;
  uint16_t lossy_max_colors;
  int32_t lossy_reduced_colors;
  uint8_t lossy_reduced_bits_rgb;
  uint8_t lossy_reduced_alpha_bits;
  uint8_t lossy_quality_min;
  uint8_t lossy_quality_max;
  uint8_t lossy_speed;
  float lossy_dither_level;
  bool lossy_dither_auto;
  const cpres_rgba_color_t *protected_colors;
  int32_t protected_colors_count;
  bool saliency_map_enable;
  bool chroma_anchor_enable;
  bool adaptive_dither_enable;
  bool gradient_boost_enable;
  bool chroma_weight_enable;
  bool postprocess_smooth_enable;
  float postprocess_smooth_importance_cutoff;
} pngx_options_t;

typedef struct {
  uint8_t *data;
  size_t size;
  size_t capacity;
} png_memory_buffer_t;

typedef struct {
  uint8_t *rgba;
  png_uint_32 width;
  png_uint_32 height;
  size_t pixel_count;
} pngx_rgba_image_t;

typedef struct {
  float gradient_mean;
  float gradient_max;
  float saturation_mean;
  float opaque_ratio;
  float translucent_ratio;
  float vibrant_ratio;
} pngx_image_stats_t;

typedef struct {
  uint8_t *importance_map;
  size_t importance_map_len;
  cpres_rgba_color_t *derived_colors;
  size_t derived_colors_len;
  cpres_rgba_color_t *combined_fixed_colors;
  size_t combined_fixed_len;
  uint8_t *bit_hint_map;
  size_t bit_hint_len;
} pngx_quant_support_t;

/* from pngx_bridge rust library */
extern PngxBridgeResult pngx_bridge_optimize_lossless(const uint8_t *input_data, size_t input_size, uint8_t **output_data, size_t *output_size, const PngxBridgeLosslessOptions *options);
extern PngxBridgeQuantStatus pngx_bridge_quantize(const cpres_rgba_color_t *pixels, size_t pixel_count, uint32_t width, uint32_t height, const PngxBridgeQuantParams *params,
                                                  PngxBridgeQuantOutput *output);
extern void pngx_bridge_free(uint8_t *ptr);
extern uint32_t pngx_bridge_oxipng_version(void);
extern uint32_t pngx_bridge_libimagequant_version(void);
extern bool pngx_bridge_init_threads(int num_threads);

#define PNGX_DEFINE_CLAMP(type)                                                                                                                                                                        \
  static inline type clamp_##type(type value, type min_value, type max_value) {                                                                                                                        \
    assert(min_value <= max_value);                                                                                                                                                                    \
                                                                                                                                                                                                       \
    if (value < min_value) {                                                                                                                                                                           \
      return min_value;                                                                                                                                                                                \
    }                                                                                                                                                                                                  \
                                                                                                                                                                                                       \
    if (value > max_value) {                                                                                                                                                                           \
      return max_value;                                                                                                                                                                                \
    }                                                                                                                                                                                                  \
                                                                                                                                                                                                       \
    return value;                                                                                                                                                                                      \
  }

PNGX_DEFINE_CLAMP(int32_t);
PNGX_DEFINE_CLAMP(uint32_t);
PNGX_DEFINE_CLAMP(uint16_t);
PNGX_DEFINE_CLAMP(uint8_t);
PNGX_DEFINE_CLAMP(float);

extern bool pngx_quantize_palette256(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size, int *quant_quality);
extern bool pngx_quantize_limited4444(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size);
extern bool pngx_quantize_reduced_rgba32(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint32_t *resolved_target, uint32_t *applied_colors, uint8_t **out_data,
                                         size_t *out_size);

extern void pngx_fill_pngx_options(pngx_options_t *opts, const cpres_config_t *config);
extern bool pngx_run_quantization(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size, int *quant_quality);
extern bool pngx_run_lossless_optimization(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size);
extern bool pngx_should_attempt_quantization(const pngx_options_t *opts);
extern bool pngx_quantization_better(size_t baseline_size, size_t candidate_size);

extern int pngx_get_last_error(void);
extern void pngx_set_last_error(int error_code);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_INTERNAL_PNGX_H */
