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

#define PNGX_COMMON_ANCHOR_AUTO_LIMIT_DEFAULT 16u
#define PNGX_COMMON_ANCHOR_DISTANCE_SQ_THRESHOLD 625u
#define PNGX_COMMON_ANCHOR_IMPORTANCE_BOOST_BASE 0.4f
#define PNGX_COMMON_ANCHOR_IMPORTANCE_BOOST_SCALE 0.5f
#define PNGX_COMMON_ANCHOR_IMPORTANCE_FACTOR 0.45f
#define PNGX_COMMON_ANCHOR_IMPORTANCE_THRESHOLD 0.4f
#define PNGX_COMMON_ANCHOR_LOW_COUNT_PENALTY 0.5f
#define PNGX_COMMON_ANCHOR_LOW_COUNT_THRESHOLD 4u
#define PNGX_COMMON_ANCHOR_SCALE_DIVISOR 8192u
#define PNGX_COMMON_ANCHOR_SCALE_MIN 12u
#define PNGX_COMMON_ANCHOR_SCORE_THRESHOLD 0.35f
#define PNGX_COMMON_DITHER_ALPHA_OPAQUE_THRESHOLD 248u
#define PNGX_COMMON_DITHER_ALPHA_TRANSLUCENT_THRESHOLD 32u
#define PNGX_COMMON_DITHER_BASE_LEVEL 0.62f
#define PNGX_COMMON_DITHER_COVERAGE_THRESHOLD 0.35f
#define PNGX_COMMON_DITHER_GRADIENT_MIN 0.02f
#define PNGX_COMMON_DITHER_HIGH_GRADIENT_BOOST 0.12f
#define PNGX_COMMON_DITHER_LOW_BIT_BOOST 0.05f
#define PNGX_COMMON_DITHER_LOW_BIT_GRADIENT_BOOST 0.05f
#define PNGX_COMMON_DITHER_LOW_GRADIENT_CUT 0.12f
#define PNGX_COMMON_DITHER_MAX 0.95f
#define PNGX_COMMON_DITHER_MID_GRADIENT_BOOST 0.05f
#define PNGX_COMMON_DITHER_MID_LOW_GRADIENT_CUT 0.05f
#define PNGX_COMMON_DITHER_MIN 0.2f
#define PNGX_COMMON_DITHER_OPAQUE_HIGH_BOOST 0.05f
#define PNGX_COMMON_DITHER_OPAQUE_LOW_CUT 0.08f
#define PNGX_COMMON_DITHER_SPAN_THRESHOLD 2.0f
#define PNGX_COMMON_DITHER_TARGET_CAP 0.9f
#define PNGX_COMMON_DITHER_TARGET_CAP_LOW_BIT 0.96f
#define PNGX_COMMON_DITHER_TRANSLUCENT_CUT 0.05f
#define PNGX_COMMON_FIXED_PALETTE_DISTANCE_SQ 400u
#define PNGX_COMMON_FIXED_PALETTE_MAX 256u
#define PNGX_COMMON_FS_COEFF_1_16 (1.0f / 16.0f)
#define PNGX_COMMON_FS_COEFF_3_16 (3.0f / 16.0f)
#define PNGX_COMMON_FS_COEFF_5_16 (5.0f / 16.0f)
#define PNGX_COMMON_FS_COEFF_7_16 (7.0f / 16.0f)
#define PNGX_COMMON_LUMA_B_COEFF 0.0722f
#define PNGX_COMMON_LUMA_G_COEFF 0.7152f
#define PNGX_COMMON_LUMA_R_COEFF 0.2126f
#define PNGX_COMMON_PREPARE_ALPHA_BASE 0.4f
#define PNGX_COMMON_PREPARE_ALPHA_MULTIPLIER 0.6f
#define PNGX_COMMON_PREPARE_ALPHA_THRESHOLD 0.85f
#define PNGX_COMMON_PREPARE_ANCHOR_IMPORTANCE_BONUS 0.05f
#define PNGX_COMMON_PREPARE_ANCHOR_IMPORTANCE_THRESHOLD 0.75f
#define PNGX_COMMON_PREPARE_ANCHOR_MIX 0.55f
#define PNGX_COMMON_PREPARE_ANCHOR_SATURATION 0.45f
#define PNGX_COMMON_PREPARE_ANCHOR_SCORE_THRESHOLD 0.35f
#define PNGX_COMMON_PREPARE_BOOST_BASE 0.08f
#define PNGX_COMMON_PREPARE_BOOST_FACTOR 0.3f
#define PNGX_COMMON_PREPARE_BOOST_THRESHOLD 0.25f
#define PNGX_COMMON_PREPARE_BUCKET_ALPHA 170u
#define PNGX_COMMON_PREPARE_BUCKET_IMPORTANCE 0.55f
#define PNGX_COMMON_PREPARE_BUCKET_SATURATION 0.35f
#define PNGX_COMMON_PREPARE_CHROMA_WEIGHT 0.35f
#define PNGX_COMMON_PREPARE_CUT_FACTOR 0.65f
#define PNGX_COMMON_PREPARE_CUT_THRESHOLD 0.08f
#define PNGX_COMMON_PREPARE_GRADIENT_SCALE 0.5f
#define PNGX_COMMON_PREPARE_MAP_MIN_VALUE 4u
#define PNGX_COMMON_PREPARE_MIX_GRADIENT 0.3f
#define PNGX_COMMON_PREPARE_MIX_IMPORTANCE 0.6f
#define PNGX_COMMON_PREPARE_VIBRANT_ALPHA 127u
#define PNGX_COMMON_PREPARE_VIBRANT_GRADIENT 0.05f
#define PNGX_COMMON_PREPARE_VIBRANT_SATURATION 0.55f
#define PNGX_COMMON_RESOLVE_ADAPTIVE_FLAT_CUT 0.12f
#define PNGX_COMMON_RESOLVE_ADAPTIVE_GRADIENT_BOOST 0.06f
#define PNGX_COMMON_RESOLVE_ADAPTIVE_SATURATION_BOOST 0.03f
#define PNGX_COMMON_RESOLVE_ADAPTIVE_SATURATION_CUT 0.02f
#define PNGX_COMMON_RESOLVE_ADAPTIVE_VIBRANT_CUT 0.05f
#define PNGX_COMMON_RESOLVE_AUTO_BASE 0.35f
#define PNGX_COMMON_RESOLVE_AUTO_GRADIENT_WEIGHT 0.35f
#define PNGX_COMMON_RESOLVE_AUTO_OPAQUE_CUT 0.06f
#define PNGX_COMMON_RESOLVE_AUTO_SATURATION_WEIGHT 0.15f
#define PNGX_COMMON_RESOLVE_DEFAULT_GRADIENT 0.2f
#define PNGX_COMMON_RESOLVE_DEFAULT_OPAQUE 1.0f
#define PNGX_COMMON_RESOLVE_DEFAULT_SATURATION 0.2f
#define PNGX_COMMON_RESOLVE_DEFAULT_VIBRANT 0.05f
#define PNGX_COMMON_RESOLVE_MAX 0.90f
#define PNGX_COMMON_RESOLVE_MIN 0.02f
#define PNGX_PALETTE256_GRADIENT_PROFILE_DITHER_FLOOR 0.78f
#define PNGX_PALETTE256_GRADIENT_PROFILE_GRADIENT_MEAN_MAX 0.16f
#define PNGX_PALETTE256_GRADIENT_PROFILE_OPAQUE_RATIO_THRESHOLD 0.90f
#define PNGX_PALETTE256_GRADIENT_PROFILE_SATURATION_MEAN_MAX 0.42f
#define PNGX_PALETTE256_TUNE_GRADIENT_MEAN_MAX 0.14f
#define PNGX_PALETTE256_TUNE_OPAQUE_RATIO_THRESHOLD 0.90f
#define PNGX_PALETTE256_TUNE_QUALITY_MAX_TARGET 100
#define PNGX_PALETTE256_TUNE_QUALITY_MIN_FLOOR 90
#define PNGX_PALETTE256_TUNE_SATURATION_MEAN_MAX 0.35f
#define PNGX_PALETTE256_TUNE_SPEED_MAX 1
#define PNGX_REDUCED_ALPHA_DETAIL_WEIGHT 0.5f
#define PNGX_REDUCED_ALPHA_LEVEL_LIMIT_FEW 4u
#define PNGX_REDUCED_ALPHA_MIN_DITHER_FACTOR 0.04f
#define PNGX_REDUCED_ALPHA_NEAR_TRANSPARENT 8u
#define PNGX_REDUCED_ALPHA_OPAQUE_LIMIT 0.985f
#define PNGX_REDUCED_ALPHA_RATIO_FEW 0.15f
#define PNGX_REDUCED_ALPHA_RATIO_LOW 0.1f
#define PNGX_REDUCED_ALPHA_RATIO_MINIMAL 0.07f
#define PNGX_REDUCED_ALPHA_SIMPLE_DEFAULT_OPAQUE 1.0f
#define PNGX_REDUCED_ALPHA_SIMPLE_DEFAULT_TRANSLUCENT 0.0f
#define PNGX_REDUCED_ALPHA_SIMPLE_OPAQUE_RANGE 0.1f
#define PNGX_REDUCED_ALPHA_SIMPLE_OPAQUE_REF 0.9f
#define PNGX_REDUCED_ALPHA_SIMPLE_OPAQUE_WEIGHT 0.6f
#define PNGX_REDUCED_ALPHA_SIMPLE_TRANSLUCENT_RANGE 0.12f
#define PNGX_REDUCED_ALPHA_SIMPLE_TRANSLUCENT_REF 0.12f
#define PNGX_REDUCED_ALPHA_SIMPLE_TRANSLUCENT_WEIGHT 0.4f
#define PNGX_REDUCED_ALPHA_TRANSLUCENT_LIMIT 0.06f
#define PNGX_REDUCED_HEAD_DOMINANCE_LIMIT 64
#define PNGX_REDUCED_IMPORTANCE_LEVEL_FULL 232
#define PNGX_REDUCED_IMPORTANCE_LEVEL_HIGH 184
#define PNGX_REDUCED_IMPORTANCE_LEVEL_LOW 96
#define PNGX_REDUCED_IMPORTANCE_LEVEL_MEDIUM 136
#define PNGX_REDUCED_IMPORTANCE_SCALE_DENOM 255.0f
#define PNGX_REDUCED_IMPORTANCE_SCALE_MIN 0.25f
#define PNGX_REDUCED_IMPORTANCE_SCALE_RANGE 0.50f
#define PNGX_REDUCED_IMPORTANCE_WEIGHT_BONUS_HIGH 2u
#define PNGX_REDUCED_IMPORTANCE_WEIGHT_BONUS_MEDIUM 1u
#define PNGX_REDUCED_IMPORTANCE_WEIGHT_BONUS_STRONG 3u
#define PNGX_REDUCED_IMPORTANCE_WEIGHT_CAP 16u
#define PNGX_REDUCED_IMPORTANCE_WEIGHT_SHIFT 5u
#define PNGX_REDUCED_IMPORTANCE_WEIGHT_THRESHOLD_HIGH 150
#define PNGX_REDUCED_IMPORTANCE_WEIGHT_THRESHOLD_MEDIUM 80
#define PNGX_REDUCED_IMPORTANCE_WEIGHT_THRESHOLD_STRONG 220
#define PNGX_REDUCED_LOW_WEIGHT_DIVISOR 2048u
#define PNGX_REDUCED_LOW_WEIGHT_MAX 2048u
#define PNGX_REDUCED_LOW_WEIGHT_MIN 32u
#define PNGX_REDUCED_PASSTHROUGH_DEFAULT_GRADIENT 0.22f
#define PNGX_REDUCED_PASSTHROUGH_DEFAULT_SATURATION 0.28f
#define PNGX_REDUCED_PASSTHROUGH_DEFAULT_VIBRANT 0.06f
#define PNGX_REDUCED_PASSTHROUGH_GRADIENT_WEIGHT 0.55f
#define PNGX_REDUCED_PASSTHROUGH_RATIO_BASE 0.82f
#define PNGX_REDUCED_PASSTHROUGH_RATIO_CAP 0.95f
#define PNGX_REDUCED_PASSTHROUGH_RATIO_GAIN 0.10f
#define PNGX_REDUCED_PASSTHROUGH_SATURATION_WEIGHT 0.30f
#define PNGX_REDUCED_PASSTHROUGH_VIBRANT_WEIGHT 0.15f
#define PNGX_REDUCED_PRIORITY_DETAIL_WEIGHT 0.4f
#define PNGX_REDUCED_PRIORITY_MASS_WEIGHT 0.15f
#define PNGX_REDUCED_PRIORITY_SPAN_WEIGHT 0.45f
#define PNGX_REDUCED_RGBA32_PASSTHROUGH_GRID_DIVISOR 4
#define PNGX_REDUCED_RGBA32_PASSTHROUGH_MIN_COLORS 512
#define PNGX_REDUCED_ROUNDING_OFFSET 0.5f
#define PNGX_REDUCED_STATS_FLAT_DEFAULT_GRADIENT 0.2f
#define PNGX_REDUCED_STATS_FLAT_DEFAULT_SATURATION 0.25f
#define PNGX_REDUCED_STATS_FLAT_DEFAULT_VIBRANT 0.05f
#define PNGX_REDUCED_STATS_FLAT_GRADIENT_REF 0.18f
#define PNGX_REDUCED_STATS_FLAT_GRADIENT_WEIGHT 0.5f
#define PNGX_REDUCED_STATS_FLAT_SATURATION_REF 0.24f
#define PNGX_REDUCED_STATS_FLAT_SATURATION_WEIGHT 0.35f
#define PNGX_REDUCED_STATS_FLAT_VIBRANT_WEIGHT 0.15f
#define PNGX_REDUCED_TARGET_ALPHA_SIMPLE_CLAMP 0.12f
#define PNGX_REDUCED_TARGET_ALPHA_SIMPLE_SCALE 0.12f
#define PNGX_REDUCED_TARGET_ALPHA_SIMPLE_THRESHOLD 0.1f
#define PNGX_REDUCED_TARGET_BASE_MIN 512.0f
#define PNGX_REDUCED_TARGET_COMBINED_CUT_BASE 0.45f
#define PNGX_REDUCED_TARGET_COMBINED_CUT_CLAMP 0.28f
#define PNGX_REDUCED_TARGET_DENSITY_GAP_CLAMP 0.15f
#define PNGX_REDUCED_TARGET_DENSITY_GAP_SCALE 0.5f
#define PNGX_REDUCED_TARGET_DENSITY_HIGH_SCALE 1.15f
#define PNGX_REDUCED_TARGET_DENSITY_HIGH_THRESHOLD 0.35f
#define PNGX_REDUCED_TARGET_DENSITY_LOW_SCALE 0.85f
#define PNGX_REDUCED_TARGET_DENSITY_LOW_THRESHOLD 0.08f
#define PNGX_REDUCED_TARGET_DENSITY_THRESHOLD 0.27f
#define PNGX_REDUCED_TARGET_DETAIL_BOOST_CLAMP 0.10f
#define PNGX_REDUCED_TARGET_DETAIL_BOOST_SCALE 0.25f
#define PNGX_REDUCED_TARGET_DETAIL_PRESSURE_ALPHA_LIMIT 0.45f
#define PNGX_REDUCED_TARGET_DETAIL_PRESSURE_BOOST 0.38f
#define PNGX_REDUCED_TARGET_DETAIL_PRESSURE_DENSITY_LIMIT 0.35f
#define PNGX_REDUCED_TARGET_DETAIL_PRESSURE_FLAT_LIMIT 0.32f
#define PNGX_REDUCED_TARGET_DETAIL_PRESSURE_GENTLE_LIMIT 0.5f
#define PNGX_REDUCED_TARGET_DETAIL_PRESSURE_HEAD_LIMIT 0.45f
#define PNGX_REDUCED_TARGET_DETAIL_PRESSURE_STRONG_LIMIT 0.42f
#define PNGX_REDUCED_TARGET_DETAIL_PRESSURE_TAIL_LIMIT 0.35f
#define PNGX_REDUCED_TARGET_DETAIL_RELIEF_BASE 0.45f
#define PNGX_REDUCED_TARGET_DETAIL_RELIEF_CLAMP 0.2f
#define PNGX_REDUCED_TARGET_DETAIL_RELIEF_SCALE 0.35f
#define PNGX_REDUCED_TARGET_DOMINANCE_GAIN_CLAMP 0.32f
#define PNGX_REDUCED_TARGET_DOMINANCE_GAIN_SCALE 1.35f
#define PNGX_REDUCED_TARGET_FLATNESS_CLAMP 0.18f
#define PNGX_REDUCED_TARGET_FLATNESS_SCALE 0.18f
#define PNGX_REDUCED_TARGET_FLATNESS_THRESHOLD 0.05f
#define PNGX_REDUCED_TARGET_GENTLE_CLAMP 0.16f
#define PNGX_REDUCED_TARGET_GENTLE_COLOR_RANGE 384.0f
#define PNGX_REDUCED_TARGET_GENTLE_MAX_COLORS 1024u
#define PNGX_REDUCED_TARGET_GENTLE_MIN_COLORS 640u
#define PNGX_REDUCED_TARGET_GENTLE_SCALE 0.22f
#define PNGX_REDUCED_TARGET_GRADIENT_RELIEF_DEFAULT 0.5f
#define PNGX_REDUCED_TARGET_GRADIENT_RELIEF_REF 0.22f
#define PNGX_REDUCED_TARGET_GRADIENT_RELIEF_SECONDARY_DEFAULT 0.35f
#define PNGX_REDUCED_TARGET_GRADIENT_RELIEF_SECONDARY_REF 0.24f
#define PNGX_REDUCED_TARGET_HEAD_CUT_BASE 0.32f
#define PNGX_REDUCED_TARGET_HEAD_CUT_CLAMP 0.18f
#define PNGX_REDUCED_TARGET_HEAD_CUT_RELIEF 0.28f
#define PNGX_REDUCED_TARGET_HEAD_DOMINANCE_BUCKETS 48u
#define PNGX_REDUCED_TARGET_HEAD_DOMINANCE_STRONG 0.58f
#define PNGX_REDUCED_TARGET_HEAD_DOMINANCE_THRESHOLD 0.6f
#define PNGX_REDUCED_TARGET_LOW_WEIGHT_RATIO_STRONG 0.46f
#define PNGX_REDUCED_TARGET_LOW_WEIGHT_REDUCTION_BASE 0.26f
#define PNGX_REDUCED_TARGET_LOW_WEIGHT_REDUCTION_CLAMP 0.18f
#define PNGX_REDUCED_TARGET_LOW_WEIGHT_REDUCTION_DETAIL 0.12f
#define PNGX_REDUCED_TARGET_LOW_WEIGHT_REDUCTION_START 0.38f
#define PNGX_REDUCED_TARGET_RELIEF_CLAMP 0.2f
#define PNGX_REDUCED_TARGET_RELIEF_GRADIENT_WEIGHT 0.65f
#define PNGX_REDUCED_TARGET_RELIEF_SATURATION_WEIGHT 0.35f
#define PNGX_REDUCED_TARGET_RELIEF_SCALE 0.25f
#define PNGX_REDUCED_TARGET_SATURATION_RELIEF_DEFAULT 0.25f
#define PNGX_REDUCED_TARGET_SATURATION_RELIEF_REF 0.3f
#define PNGX_REDUCED_TARGET_TAIL_CUT_CLAMP 0.12f
#define PNGX_REDUCED_TARGET_TAIL_GAIN_BASE 0.34f
#define PNGX_REDUCED_TARGET_TAIL_GAIN_CLAMP 0.18f
#define PNGX_REDUCED_TARGET_TAIL_GAIN_RELIEF 0.28f
#define PNGX_REDUCED_TARGET_TAIL_RATIO_THRESHOLD 0.52f
#define PNGX_REDUCED_TARGET_TAIL_WIDTH_BASE 0.4f
#define PNGX_REDUCED_TARGET_TAIL_WIDTH_SCALE 0.65f
#define PNGX_REDUCED_TARGET_UNIQUE_BASE_SCALE 12.0f
#define PNGX_REDUCED_TARGET_UNIQUE_COLOR_THRESHOLD 1024u
#define PNGX_REDUCED_TRIM_ALPHA_SIMPLE_CLAMP 0.08f
#define PNGX_REDUCED_TRIM_ALPHA_SIMPLE_SCALE 0.08f
#define PNGX_REDUCED_TRIM_ALPHA_SIMPLE_THRESHOLD 0.1f
#define PNGX_REDUCED_TRIM_DENSITY_CLAMP 0.12f
#define PNGX_REDUCED_TRIM_DENSITY_SCALE 0.25f
#define PNGX_REDUCED_TRIM_DENSITY_THRESHOLD 0.22f
#define PNGX_REDUCED_TRIM_DETAIL_PRESSURE_FLAT_LIMIT 0.42f
#define PNGX_REDUCED_TRIM_DETAIL_PRESSURE_HEAD_LIMIT 0.48f
#define PNGX_REDUCED_TRIM_DETAIL_PRESSURE_TAIL_LIMIT 0.52f
#define PNGX_REDUCED_TRIM_FLATNESS_CLAMP 0.08f
#define PNGX_REDUCED_TRIM_FLATNESS_SCALE 0.15f
#define PNGX_REDUCED_TRIM_FLATNESS_THRESHOLD 0.08f
#define PNGX_REDUCED_TRIM_FLATNESS_WEIGHT 0.45f
#define PNGX_REDUCED_TRIM_HEAD_CLAMP 0.32f
#define PNGX_REDUCED_TRIM_HEAD_DOMINANCE_THRESHOLD 0.58f
#define PNGX_REDUCED_TRIM_HEAD_WEIGHT 0.55f
#define PNGX_REDUCED_TRIM_MIN_COLOR_DIFF 64u
#define PNGX_REDUCED_TRIM_MIN_COLOR_MARGIN 32u
#define PNGX_REDUCED_TRIM_MIN_TRIGGER 0.03f
#define PNGX_REDUCED_TRIM_TAIL_BASE_WEIGHT 0.4f
#define PNGX_REDUCED_TRIM_TAIL_CLAMP 0.2f
#define PNGX_REDUCED_TRIM_TAIL_DETAIL_WEIGHT 0.3f
#define PNGX_REDUCED_TRIM_TAIL_RATIO_THRESHOLD 0.42f
#define PNGX_REDUCED_TRIM_TOTAL_CLAMP 0.38f
#define PNGX_REDUCED_TUNE_DEFAULT_GRADIENT 0.2f
#define PNGX_REDUCED_TUNE_DEFAULT_OPAQUE 1.0f
#define PNGX_REDUCED_TUNE_DEFAULT_SATURATION 0.3f
#define PNGX_REDUCED_TUNE_DEFAULT_TRANSLUCENT 0.0f
#define PNGX_REDUCED_TUNE_DEFAULT_VIBRANT 0.05f
#define PNGX_REDUCED_TUNE_FLAT_GRADIENT_THRESHOLD 0.15f
#define PNGX_REDUCED_TUNE_FLAT_SATURATION_THRESHOLD 0.28f
#define PNGX_REDUCED_TUNE_FLAT_VIBRANT_THRESHOLD 0.08f
#define PNGX_REDUCED_TUNE_VERY_FLAT_GRADIENT 0.08f
#define PNGX_REDUCED_TUNE_VERY_FLAT_SATURATION 0.18f
#define PNGX_REDUCED_VIBRANT_RATIO_LOW 0.04f
#define PNGX_LIMITED_RGBA4444_BITS 4
#define PNGX_RGBA_CHANNELS 4
#define PNGX_CHROMA_BUCKET_BITS 4
#define PNGX_CHROMA_BUCKET_SHIFT (8 - PNGX_CHROMA_BUCKET_BITS)
#define PNGX_CHROMA_BUCKET_DIM 16
#define PNGX_CHROMA_BUCKET_COUNT (PNGX_CHROMA_BUCKET_DIM * PNGX_CHROMA_BUCKET_DIM * PNGX_CHROMA_BUCKET_DIM)
#define PNGX_FULL_CHANNEL_BITS 8
#define PNGX_IMPORTANCE_SCALE 65535.0f
#define PNGX_MAX_DERIVED_COLORS 48
#define PNGX_POSTPROCESS_DISABLE_DITHER_THRESHOLD 0.25f
#define PNGX_POSTPROCESS_MAX_COLOR_DISTANCE_SQ 900

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
  bool palette256_gradient_profile_enable;
  float palette256_gradient_profile_dither_floor;
  bool palette256_alpha_bleed_enable;
  uint16_t palette256_alpha_bleed_max_distance;
  uint8_t palette256_alpha_bleed_opaque_threshold;
  uint8_t palette256_alpha_bleed_soft_limit;
  float palette256_profile_opaque_ratio_threshold;
  float palette256_profile_gradient_mean_max;
  float palette256_profile_saturation_mean_max;
  float palette256_tune_opaque_ratio_threshold;
  float palette256_tune_gradient_mean_max;
  float palette256_tune_saturation_mean_max;
  int16_t palette256_tune_speed_max;
  int16_t palette256_tune_quality_min_floor;
  int16_t palette256_tune_quality_max_target;
  uint32_t thread_count;
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
PngxBridgeResult pngx_bridge_optimize_lossless(const uint8_t *input_data, size_t input_size, uint8_t **output_data, size_t *output_size, const PngxBridgeLosslessOptions *options);
PngxBridgeQuantStatus pngx_bridge_quantize(const cpres_rgba_color_t *pixels, size_t pixel_count, uint32_t width, uint32_t height, const PngxBridgeQuantParams *params, PngxBridgeQuantOutput *output);
void pngx_bridge_free(uint8_t *ptr);
uint32_t pngx_bridge_oxipng_version(void);
uint32_t pngx_bridge_libimagequant_version(void);
bool pngx_bridge_init_threads(int num_threads);

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

bool pngx_quantize_palette256(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size, int *quant_quality);
bool pngx_palette256_prepare(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_rgba, uint32_t *out_width, uint32_t *out_height, uint8_t **out_importance_map,
                             size_t *out_importance_map_len, int32_t *out_speed, uint8_t *out_quality_min, uint8_t *out_quality_max, uint32_t *out_max_colors, float *out_dither_level,
                             uint8_t **out_fixed_colors, size_t *out_fixed_colors_len);
bool pngx_palette256_finalize(const uint8_t *indices, size_t indices_len, const cpres_rgba_color_t *palette, size_t palette_len, uint8_t **out_data, size_t *out_size);
void pngx_palette256_cleanup(void);
bool pngx_create_palette_png(const uint8_t *indices, size_t indices_len, const cpres_rgba_color_t *palette, size_t palette_len, uint32_t width, uint32_t height, uint8_t **out_data, size_t *out_size);
bool create_rgba_png(const uint8_t *rgba, size_t pixel_count, uint32_t width, uint32_t height, uint8_t **out_data, size_t *out_size);
bool pngx_quantize_limited4444(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size);
bool pngx_quantize_reduced_rgba32(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint32_t *resolved_target, uint32_t *applied_colors, uint8_t **out_data, size_t *out_size);
void pngx_fill_pngx_options(pngx_options_t *opts, const cpres_config_t *config);
bool pngx_run_quantization(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size, int *quant_quality);
bool pngx_run_lossless_optimization(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size);
bool pngx_should_attempt_quantization(const pngx_options_t *opts);
bool pngx_quantization_better(size_t baseline_size, size_t candidate_size);

int pngx_get_last_error(void);
void pngx_set_last_error(int error_code);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_INTERNAL_PNGX_H */
