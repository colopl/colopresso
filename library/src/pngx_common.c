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

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>
#include <zlib.h>

#include "internal/log.h"
#include "internal/png.h"
#include "internal/pngx_common.h"

#define PNGX_LUMA_R 0.2126f
#define PNGX_LUMA_G 0.7152f
#define PNGX_LUMA_B 0.0722f

#define PNGX_ANCHOR_SCALE_DIVISOR 8192
#define PNGX_ANCHOR_SCALE_MIN 12
#define PNGX_ANCHOR_AUTO_LIMIT_DEFAULT 16
#define PNGX_ANCHOR_IMPORTANCE_FACTOR 0.45f
#define PNGX_ANCHOR_IMPORTANCE_THRESHOLD 0.4f
#define PNGX_ANCHOR_IMPORTANCE_BOOST_BASE 0.4f
#define PNGX_ANCHOR_IMPORTANCE_BOOST_SCALE 0.5f
#define PNGX_ANCHOR_SCORE_THRESHOLD 0.35f
#define PNGX_ANCHOR_LOW_COUNT_PENALTY 0.5f
#define PNGX_ANCHOR_LOW_COUNT_THRESHOLD 4
#define PNGX_ANCHOR_DISTANCE_SQ_THRESHOLD 625

#define PNGX_DITHER_OPAQUE_THRESHOLD 248
#define PNGX_DITHER_TRANSLUCENT_THRESHOLD 32
#define PNGX_DITHER_GRADIENT_MIN 0.02f
#define PNGX_DITHER_BASE_LEVEL 0.62f
#define PNGX_DITHER_HIGH_GRADIENT_BOOST 0.12f
#define PNGX_DITHER_MID_GRADIENT_BOOST 0.05f
#define PNGX_DITHER_LOW_GRADIENT_CUT 0.12f
#define PNGX_DITHER_MID_LOW_GRADIENT_CUT 0.05f
#define PNGX_DITHER_OPAQUE_LOW_CUT 0.08f
#define PNGX_DITHER_OPAQUE_HIGH_BOOST 0.05f
#define PNGX_DITHER_TRANSLUCENT_CUT 0.05f
#define PNGX_DITHER_COVERAGE_THRESHOLD 0.35f
#define PNGX_DITHER_SPAN_THRESHOLD 2.0f
#define PNGX_DITHER_TARGET_CAP 0.9f
#define PNGX_DITHER_TARGET_CAP_LOW_BIT 0.96f
#define PNGX_DITHER_LOW_BIT_BOOST 0.05f
#define PNGX_DITHER_LOW_BIT_GRADIENT_BOOST 0.05f
#define PNGX_DITHER_MIN 0.2f
#define PNGX_DITHER_MAX 0.95f

#define PNGX_FIXED_PALETTE_DISTANCE_SQ 400
#define PNGX_FIXED_PALETTE_MAX 256

#define PNGX_RESOLVE_DEFAULT_GRADIENT 0.2f
#define PNGX_RESOLVE_DEFAULT_SATURATION 0.2f
#define PNGX_RESOLVE_DEFAULT_OPAQUE 1.0f
#define PNGX_RESOLVE_DEFAULT_VIBRANT 0.05f
#define PNGX_RESOLVE_AUTO_BASE 0.35f
#define PNGX_RESOLVE_AUTO_GRADIENT_WEIGHT 0.35f
#define PNGX_RESOLVE_AUTO_SATURATION_WEIGHT 0.15f
#define PNGX_RESOLVE_AUTO_OPAQUE_CUT 0.06f
#define PNGX_RESOLVE_ADAPTIVE_FLAT_CUT 0.12f
#define PNGX_RESOLVE_ADAPTIVE_GRADIENT_BOOST 0.06f
#define PNGX_RESOLVE_ADAPTIVE_VIBRANT_CUT 0.05f
#define PNGX_RESOLVE_ADAPTIVE_SATURATION_BOOST 0.03f
#define PNGX_RESOLVE_ADAPTIVE_SATURATION_CUT 0.02f
#define PNGX_RESOLVE_MIN 0.02f
#define PNGX_RESOLVE_MAX 0.90f

#define PNGX_PREPARE_GRADIENT_SCALE 0.5f
#define PNGX_PREPARE_VIBRANT_SATURATION 0.55f
#define PNGX_PREPARE_VIBRANT_GRADIENT 0.05f
#define PNGX_PREPARE_VIBRANT_ALPHA 127
#define PNGX_PREPARE_CHROMA_WEIGHT 0.35f
#define PNGX_PREPARE_BOOST_THRESHOLD 0.25f
#define PNGX_PREPARE_BOOST_BASE 0.08f
#define PNGX_PREPARE_BOOST_FACTOR 0.3f
#define PNGX_PREPARE_CUT_THRESHOLD 0.08f
#define PNGX_PREPARE_CUT_FACTOR 0.65f
#define PNGX_PREPARE_ALPHA_THRESHOLD 0.85f
#define PNGX_PREPARE_ALPHA_BASE 0.4f
#define PNGX_PREPARE_ALPHA_MULTIPLIER 0.6f
#define PNGX_PREPARE_BUCKET_SATURATION 0.35f
#define PNGX_PREPARE_BUCKET_IMPORTANCE 0.55f
#define PNGX_PREPARE_BUCKET_ALPHA 170
#define PNGX_PREPARE_MIX_IMPORTANCE 0.6f
#define PNGX_PREPARE_MIX_GRADIENT 0.3f
#define PNGX_PREPARE_ANCHOR_SATURATION 0.45f
#define PNGX_PREPARE_ANCHOR_MIX 0.55f
#define PNGX_PREPARE_ANCHOR_IMP_THRESHOLD 0.75f
#define PNGX_PREPARE_ANCHOR_IMP_BONUS 0.05f
#define PNGX_PREPARE_ANCHOR_SCORE_THRESHOLD 0.35f
#define PNGX_PREPARE_MAP_MIN_VALUE 4

#define PNGX_FS_COEFF_7 (7.0f / 16.0f)
#define PNGX_FS_COEFF_3 (3.0f / 16.0f)
#define PNGX_FS_COEFF_5 (5.0f / 16.0f)
#define PNGX_FS_COEFF_1 (1.0f / 16.0f)

typedef struct {
  uint64_t r_sum;
  uint64_t g_sum;
  uint64_t b_sum;
  uint64_t a_sum;
  uint32_t count;
  float score;
  float importance_accum;
} pngx_chroma_bucket_t;

static inline float absf(float value) { return (value < 0.0f) ? -value : value; }

static inline uint32_t chroma_bucket_index(uint8_t r, uint8_t g, uint8_t b) {
  return (((uint32_t)r >> PNGX_CHROMA_BUCKET_SHIFT) * PNGX_CHROMA_BUCKET_DIM * PNGX_CHROMA_BUCKET_DIM) + (((uint32_t)g >> PNGX_CHROMA_BUCKET_SHIFT) * PNGX_CHROMA_BUCKET_DIM) +
         ((uint32_t)b >> PNGX_CHROMA_BUCKET_SHIFT);
}

static inline float calc_saturation(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t max_v = r, min_v = r;

  if (g > max_v) {
    max_v = g;
  }
  if (b > max_v) {
    max_v = b;
  }
  if (g < min_v) {
    min_v = g;
  }
  if (b < min_v) {
    min_v = b;
  }

  if (max_v == 0) {
    return 0.0f;
  }

  return (float)(max_v - min_v) / (float)max_v;
}

static inline float calc_luma(uint8_t r, uint8_t g, uint8_t b) { return PNGX_LUMA_R * (float)r + PNGX_LUMA_G * (float)g + PNGX_LUMA_B * (float)b; }

static inline bool decode_png_rgba(const uint8_t *png_data, size_t png_size, uint8_t **rgba, png_uint_32 *width, png_uint_32 *height) {
  cpres_error_t status;

  if (!png_data || png_size == 0 || !rgba || !width || !height) {
    return false;
  }

  status = cpres_png_decode_from_memory(png_data, png_size, rgba, width, height);
  if (status != CPRES_OK) {
    cpres_log(CPRES_LOG_LEVEL_WARNING, "PNGX: Failed to decode PNG (%d)", (int)status);
    return false;
  }

  return true;
}

static inline void extract_chroma_anchors(pngx_quant_support_t *support, pngx_chroma_bucket_t *buckets, size_t bucket_count, size_t pixel_count) {
  cpres_rgba_color_t chosen[PNGX_MAX_DERIVED_COLORS];
  pngx_chroma_bucket_t *bucket;
  size_t selected, best_index, i, dst, auto_limit, scaled, check;
  float best_score, score, importance_boost;
  bool duplicate;

  if (!support || !buckets || bucket_count == 0) {
    return;
  }

  if (pixel_count > 0) {
    scaled = pixel_count / PNGX_ANCHOR_SCALE_DIVISOR;
    if (scaled < PNGX_ANCHOR_SCALE_MIN) {
      scaled = PNGX_ANCHOR_SCALE_MIN;
    }
    if (scaled > PNGX_MAX_DERIVED_COLORS) {
      scaled = PNGX_MAX_DERIVED_COLORS;
    }
    auto_limit = scaled;
  } else {
    auto_limit = PNGX_ANCHOR_AUTO_LIMIT_DEFAULT;
  }

  selected = 0;
  while (selected < auto_limit) {
    best_score = 0.0f;
    best_index = SIZE_MAX;
    for (i = 0; i < bucket_count; ++i) {
      bucket = &buckets[i];
      if (!bucket->count || bucket->score <= 0.0f) {
        continue;
      }
      importance_boost = bucket->importance_accum * PNGX_ANCHOR_IMPORTANCE_FACTOR;
      if (importance_boost > PNGX_ANCHOR_IMPORTANCE_THRESHOLD) {
        importance_boost = PNGX_ANCHOR_IMPORTANCE_BOOST_BASE + (importance_boost - PNGX_ANCHOR_IMPORTANCE_THRESHOLD) * PNGX_ANCHOR_IMPORTANCE_BOOST_SCALE;
      }
      score = bucket->score + importance_boost;
      if (bucket->count < PNGX_ANCHOR_LOW_COUNT_THRESHOLD) {
        score *= PNGX_ANCHOR_LOW_COUNT_PENALTY;
      }
      if (score > best_score) {
        best_score = score;
        best_index = i;
      }
    }
    if (best_index == SIZE_MAX || best_score < PNGX_ANCHOR_SCORE_THRESHOLD) {
      break;
    }

    chosen[selected].r = (uint8_t)(buckets[best_index].r_sum / buckets[best_index].count);
    chosen[selected].g = (uint8_t)(buckets[best_index].g_sum / buckets[best_index].count);
    chosen[selected].b = (uint8_t)(buckets[best_index].b_sum / buckets[best_index].count);
    chosen[selected].a = (uint8_t)(buckets[best_index].a_sum / buckets[best_index].count);
    buckets[best_index].score = 0.0f;
    ++selected;
  }

  if (!selected) {
    return;
  }

  dst = 0;
  for (i = 0; i < selected; ++i) {
    duplicate = false;
    for (check = 0; check < dst; ++check) {
      if (color_distance_sq(&chosen[i], &chosen[check]) < PNGX_ANCHOR_DISTANCE_SQ_THRESHOLD) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) {
      continue;
    }
    chosen[dst] = chosen[i];
    ++dst;
  }

  if (!dst) {
    return;
  }

  support->derived_colors = (cpres_rgba_color_t *)malloc(sizeof(cpres_rgba_color_t) * dst);
  if (!support->derived_colors) {
    return;
  }
  memcpy(support->derived_colors, chosen, sizeof(cpres_rgba_color_t) * dst);
  support->derived_colors_len = dst;
}

/* Call from libpng on setjmp */
/* LCOV_EXCL_START */
void memory_buffer_reset(png_memory_buffer_t *buffer) {
  if (!buffer) {
    return;
  }

  free(buffer->data);
  buffer->data = NULL;
  buffer->size = 0;
  buffer->capacity = 0;
}
/* LCOV_EXCL_STOP */

void image_stats_reset(pngx_image_stats_t *stats) {
  if (!stats) {
    return;
  }

  stats->gradient_mean = 0.0f;
  stats->gradient_max = 0.0f;
  stats->saturation_mean = 0.0f;
  stats->opaque_ratio = 0.0f;
  stats->translucent_ratio = 0.0f;
  stats->vibrant_ratio = 0.0f;
}

void quant_support_reset(pngx_quant_support_t *support) {
  if (!support) {
    return;
  }

  free(support->importance_map);
  support->importance_map = NULL;
  support->importance_map_len = 0;

  free(support->derived_colors);
  support->derived_colors = NULL;
  support->derived_colors_len = 0;

  free(support->combined_fixed_colors);
  support->combined_fixed_colors = NULL;
  support->combined_fixed_len = 0;

  free(support->bit_hint_map);
  support->bit_hint_map = NULL;
  support->bit_hint_len = 0;
}

const char *lossy_type_label(uint8_t lossy_type) {
  switch (lossy_type) {
  case PNGX_LOSSY_TYPE_LIMITED_RGBA4444:
    return "Limited RGBA4444";
  case PNGX_LOSSY_TYPE_REDUCED_RGBA32:
    return "Reduced RGBA32";
  default:
    return "Palette256";
  }
}

void rgba_image_reset(pngx_rgba_image_t *image) {
  if (!image) {
    return;
  }

  free(image->rgba);
  image->rgba = NULL;
  image->width = 0;
  image->height = 0;
  image->pixel_count = 0;
}

bool load_rgba_image(const uint8_t *png_data, size_t png_size, pngx_rgba_image_t *image) {
  if (!png_data || png_size == 0 || !image) {
    return false;
  }

  image->rgba = NULL;
  image->width = 0;
  image->height = 0;
  image->pixel_count = 0;

  if (!decode_png_rgba(png_data, png_size, &image->rgba, &image->width, &image->height)) {
    rgba_image_reset(image);
    return false;
  }

  if (!image->rgba || image->width == 0 || image->height == 0) {
    rgba_image_reset(image);
    return false;
  }

  if ((size_t)image->width > SIZE_MAX / (size_t)image->height) {
    rgba_image_reset(image);
    return false;
  }

  image->pixel_count = (size_t)image->width * (size_t)image->height;

  return true;
}

uint8_t clamp_reduced_bits(uint8_t bits) {
  const uint8_t min_bits = (uint8_t)COLOPRESSO_PNGX_REDUCED_BITS_MIN, max_bits = (uint8_t)COLOPRESSO_PNGX_REDUCED_BITS_MAX;

  if (bits < min_bits) {
    return min_bits;
  }
  if (bits > max_bits) {
    return max_bits;
  }
  return bits;
}

uint8_t quantize_channel_value(float value, uint8_t bits_per_channel) {
  uint32_t levels;
  float clamped, scaled, rounded, quantized;

  if (bits_per_channel >= 8) {
    if (value < 0.0f) {
      return 0;
    }
    if (value > 255.0f) {
      return 255;
    }
    return (uint8_t)(value + 0.5f);
  }

  if (bits_per_channel < 1) {
    bits_per_channel = 1;
  }

  levels = 1 << bits_per_channel;

  clamped = value;
  if (clamped < 0.0f) {
    clamped = 0.0f;
  } else if (clamped > 255.0f) {
    clamped = 255.0f;
  }

  if (levels <= 1) {
    return 0;
  }

  scaled = clamped * (float)(levels - 1) / 255.0f;
  if (scaled < 0.0f) {
    scaled = 0.0f;
  }
  if (scaled > (float)(levels - 1)) {
    scaled = (float)(levels - 1);
  }

  rounded = (float)((int32_t)(scaled + 0.5f));

  quantized = rounded * 255.0f / (float)(levels - 1);
  if (quantized < 0.0f) {
    quantized = 0.0f;
  } else if (quantized > 255.0f) {
    quantized = 255.0f;
  }

  return (uint8_t)(quantized + 0.5f);
}

uint8_t quantize_bits(uint8_t value, uint8_t bits) {
  if (bits >= 8) {
    return value;
  }

  return quantize_channel_value((float)value, bits);
}

void snap_rgba_to_bits(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a, uint8_t bits_rgb, uint8_t bits_alpha) {
  bits_rgb = clamp_reduced_bits(bits_rgb);
  bits_alpha = clamp_reduced_bits(bits_alpha);

  if (r) {
    *r = quantize_bits(*r, bits_rgb);
  }
  if (g) {
    *g = quantize_bits(*g, bits_rgb);
  }
  if (b) {
    *b = quantize_bits(*b, bits_rgb);
  }
  if (a) {
    *a = quantize_bits(*a, bits_alpha);
  }
}

void snap_rgba_image_to_bits(uint8_t *rgba, size_t pixel_count, uint8_t bits_rgb, uint8_t bits_alpha) {
  uint8_t r, g, b, a;
  size_t base, i;

  if (!rgba || pixel_count == 0) {
    return;
  }

  bits_rgb = clamp_reduced_bits(bits_rgb);
  bits_alpha = clamp_reduced_bits(bits_alpha);
  if (bits_rgb >= 8 && bits_alpha >= 8) {
    return;
  }

  for (i = 0; i < pixel_count; ++i) {
    base = i * 4;

    r = rgba[base + 0];
    g = rgba[base + 1];
    b = rgba[base + 2];
    a = rgba[base + 3];

    snap_rgba_to_bits(&r, &g, &b, &a, bits_rgb, bits_alpha);

    rgba[base + 0] = r;
    rgba[base + 1] = g;
    rgba[base + 2] = b;
    rgba[base + 3] = a;
  }
}

uint32_t color_distance_sq(const cpres_rgba_color_t *lhs, const cpres_rgba_color_t *rhs) {
  int32_t dr, dg, db, da;

  if (!lhs || !rhs) {
    return 0;
  }

  dr = (int32_t)lhs->r - (int32_t)rhs->r;
  dg = (int32_t)lhs->g - (int32_t)rhs->g;
  db = (int32_t)lhs->b - (int32_t)rhs->b;
  da = (int32_t)lhs->a - (int32_t)rhs->a;

  return (uint32_t)(dr * dr + dg * dg + db * db + da * da);
}

float estimate_bitdepth_dither_level(const uint8_t *rgba, png_uint_32 width, png_uint_32 height, uint8_t bits_per_channel) {
  png_uint_32 y, x;
  uint8_t r, g, b, a;
  size_t pixel_count, gradient_samples, opaque_pixels, translucent_pixels, base, right, below;
  double gradient_accum;
  float base_level, target, right_luma, below_luma, luma, normalized_gradient, opaque_ratio, translucent_ratio, min_luma, max_luma, coverage, gradient_span;

  if (!rgba || width == 0 || height == 0) {
    return clamp_float(COLOPRESSO_PNGX_DEFAULT_LOSSY_DITHER_LEVEL, 0.0f, 1.0f);
  }

  pixel_count = (size_t)width * (size_t)height;
  gradient_accum = 0.0;
  gradient_samples = 0;
  opaque_pixels = 0;
  translucent_pixels = 0;
  min_luma = 255.0f;
  max_luma = 0.0f;

  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      base = (((size_t)y * (size_t)width) + (size_t)x) * 4;
      r = rgba[base + 0];
      g = rgba[base + 1];
      b = rgba[base + 2];
      a = rgba[base + 3];
      luma = calc_luma(r, g, b);

      if (luma < min_luma) {
        min_luma = luma;
      }
      if (luma > max_luma) {
        max_luma = luma;
      }

      if (a > PNGX_DITHER_OPAQUE_THRESHOLD) {
        ++opaque_pixels;
      } else if (a > PNGX_DITHER_TRANSLUCENT_THRESHOLD) {
        ++translucent_pixels;
      }

      if (x + 1 < width) {
        right = base + 4;
        right_luma = calc_luma(rgba[right + 0], rgba[right + 1], rgba[right + 2]);
        gradient_accum += absf(right_luma - luma);
        ++gradient_samples;
      }

      if (y + 1 < height) {
        below = (((size_t)(y + 1) * (size_t)width) + (size_t)x) * 4;
        below_luma = calc_luma(rgba[below + 0], rgba[below + 1], rgba[below + 2]);
        gradient_accum += absf(below_luma - luma);
        ++gradient_samples;
      }
    }
  }

  if (gradient_samples == 0) {
    gradient_samples = 1;
  }

  normalized_gradient = (float)(gradient_accum / ((double)gradient_samples * 255.0));
  opaque_ratio = (pixel_count > 0) ? (float)((double)opaque_pixels / (double)pixel_count) : 0.0f;
  translucent_ratio = (pixel_count > 0) ? (float)((double)translucent_pixels / (double)pixel_count) : 0.0f;

  coverage = (max_luma - min_luma) / 255.0f;
  if (coverage < 0.0f) {
    coverage = 0.0f;
  } else if (coverage > 1.0f) {
    coverage = 1.0f;
  }

  gradient_span = coverage / ((normalized_gradient > PNGX_DITHER_GRADIENT_MIN) ? normalized_gradient : PNGX_DITHER_GRADIENT_MIN);

  base_level = PNGX_DITHER_BASE_LEVEL;
  target = base_level;

  if (normalized_gradient > 0.35f) {
    target += PNGX_DITHER_HIGH_GRADIENT_BOOST;
  } else if (normalized_gradient > 0.2f) {
    target += PNGX_DITHER_MID_GRADIENT_BOOST;
  } else if (normalized_gradient < 0.08f) {
    target -= PNGX_DITHER_LOW_GRADIENT_CUT;
  } else if (normalized_gradient < 0.15f) {
    target -= PNGX_DITHER_MID_LOW_GRADIENT_CUT;
  }

  if (opaque_ratio < 0.35f) {
    target -= PNGX_DITHER_OPAQUE_LOW_CUT;
  } else if (opaque_ratio > 0.9f) {
    target += PNGX_DITHER_OPAQUE_HIGH_BOOST;
  }

  if (translucent_ratio > 0.3f) {
    target -= PNGX_DITHER_TRANSLUCENT_CUT;
  }

  if (coverage > PNGX_DITHER_COVERAGE_THRESHOLD && gradient_span > PNGX_DITHER_SPAN_THRESHOLD) {
    if (target < PNGX_DITHER_TARGET_CAP) {
      target = PNGX_DITHER_TARGET_CAP;
    }
    if (bits_per_channel <= 4 && target < PNGX_DITHER_TARGET_CAP_LOW_BIT) {
      target = PNGX_DITHER_TARGET_CAP_LOW_BIT;
    }
  }

  if (bits_per_channel <= 2) {
    target += PNGX_DITHER_LOW_BIT_BOOST;
    if (normalized_gradient > 0.25f) {
      target += PNGX_DITHER_LOW_BIT_GRADIENT_BOOST;
    }
  }

  return clamp_float(target, PNGX_DITHER_MIN, PNGX_DITHER_MAX);
}

void build_fixed_palette(const pngx_options_t *source_opts, pngx_quant_support_t *support, pngx_options_t *patched_opts) {
  size_t user_count, derived_count, total_cap, i, j;
  bool duplicate;

  if (!source_opts || !support || !patched_opts) {
    return;
  }

  *patched_opts = *source_opts;
  user_count = (source_opts->protected_colors && source_opts->protected_colors_count > 0) ? (size_t)source_opts->protected_colors_count : 0;
  derived_count = support->derived_colors_len;
  if (derived_count == 0) {
    return;
  }

  total_cap = user_count + derived_count;
  support->combined_fixed_colors = (cpres_rgba_color_t *)malloc(sizeof(cpres_rgba_color_t) * total_cap);
  if (!support->combined_fixed_colors) {
    return;
  }

  if (user_count > 0 && source_opts->protected_colors) {
    memcpy(support->combined_fixed_colors, source_opts->protected_colors, sizeof(cpres_rgba_color_t) * user_count);
  }
  support->combined_fixed_len = user_count;
  for (i = 0; i < derived_count; ++i) {
    duplicate = false;
    for (j = 0; j < support->combined_fixed_len; ++j) {
      if (color_distance_sq(&support->derived_colors[i], &support->combined_fixed_colors[j]) < PNGX_FIXED_PALETTE_DISTANCE_SQ) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) {
      continue;
    }
    if (support->combined_fixed_len < total_cap) {
      support->combined_fixed_colors[support->combined_fixed_len] = support->derived_colors[i];
      ++support->combined_fixed_len;
    }
    if (support->combined_fixed_len >= PNGX_FIXED_PALETTE_MAX) {
      break;
    }
  }
  if (support->combined_fixed_len > user_count) {
    patched_opts->protected_colors = support->combined_fixed_colors;
    patched_opts->protected_colors_count = (int32_t)support->combined_fixed_len;
  }
}

float resolve_quant_dither(const pngx_options_t *opts, const pngx_image_stats_t *stats) {
  float resolved, gradient_mean, saturation_mean, opaque_ratio, vibrant_ratio, gradient_max;

  if (!opts) {
    return 0.5f;
  }

  if (stats) {
    gradient_mean = stats->gradient_mean;
    saturation_mean = stats->saturation_mean;
    opaque_ratio = stats->opaque_ratio;
    vibrant_ratio = stats->vibrant_ratio;
    gradient_max = stats->gradient_max;
  } else {
    gradient_mean = PNGX_RESOLVE_DEFAULT_GRADIENT;
    saturation_mean = PNGX_RESOLVE_DEFAULT_SATURATION;
    opaque_ratio = PNGX_RESOLVE_DEFAULT_OPAQUE;
    vibrant_ratio = PNGX_RESOLVE_DEFAULT_VIBRANT;
    gradient_max = PNGX_RESOLVE_DEFAULT_GRADIENT;
  }

  resolved = opts->lossy_dither_level;

  if (opts->lossy_dither_auto) {
    resolved = PNGX_RESOLVE_AUTO_BASE + gradient_mean * PNGX_RESOLVE_AUTO_GRADIENT_WEIGHT + saturation_mean * PNGX_RESOLVE_AUTO_SATURATION_WEIGHT;
    if (opaque_ratio < 0.7f) {
      resolved -= PNGX_RESOLVE_AUTO_OPAQUE_CUT;
    }
  }

  if (opts->adaptive_dither_enable) {
    if (gradient_mean < 0.10f) {
      resolved -= PNGX_RESOLVE_ADAPTIVE_FLAT_CUT;
    } else if (gradient_mean > 0.30f) {
      resolved += PNGX_RESOLVE_ADAPTIVE_GRADIENT_BOOST;
    }
    if (gradient_max > 0.5f && vibrant_ratio > 0.12f) {
      resolved -= PNGX_RESOLVE_ADAPTIVE_VIBRANT_CUT;
    }
    if (saturation_mean > 0.38f) {
      resolved += PNGX_RESOLVE_ADAPTIVE_SATURATION_BOOST;
    } else if (saturation_mean < 0.12f) {
      resolved -= PNGX_RESOLVE_ADAPTIVE_SATURATION_CUT;
    }
  }

  if (resolved < PNGX_RESOLVE_MIN) {
    resolved = PNGX_RESOLVE_MIN;
  } else if (resolved > PNGX_RESOLVE_MAX) {
    resolved = PNGX_RESOLVE_MAX;
  }

  return resolved;
}

bool prepare_quant_support(const pngx_rgba_image_t *image, const pngx_options_t *opts, pngx_quant_support_t *support, pngx_image_stats_t *stats) {
  pngx_chroma_bucket_t *buckets = NULL, *bucket_entry;
  uint32_t x, y, range, sample;
  uint16_t *importance_work = NULL, raw_min, raw_max;
  uint8_t r, g, b, a, value;
  size_t pixel_index, base, next_row_base, opaque_pixels, translucent_pixels, vibrant_pixels;
  float gradient_sum, saturation_sum, luma, gradient, saturation, importance, alpha_factor, anchor_score, right_luma, below_luma, importance_mix, *luma_row_curr = NULL, *luma_row_next = NULL, *luma_row_tmp;
  bool need_map, need_buckets;

  if (!image || !opts || !support || !stats || image->pixel_count == 0) {
    return false;
  }

  raw_min = UINT16_MAX;
  raw_max = 0;
  need_map = opts->saliency_map_enable || opts->postprocess_smooth_enable;
  need_buckets = opts->chroma_anchor_enable;
  gradient_sum = 0.0f;
  saturation_sum = 0.0f;
  opaque_pixels = 0;
  translucent_pixels = 0;
  vibrant_pixels = 0;
  if (need_map) {
    importance_work = (uint16_t *)malloc(sizeof(uint16_t) * image->pixel_count);
    if (!importance_work) {
      return false;
    }
  }

  if (need_buckets) {
    buckets = (pngx_chroma_bucket_t *)calloc(PNGX_CHROMA_BUCKET_COUNT, sizeof(pngx_chroma_bucket_t));
    if (!buckets) {
      if (importance_work) {
        free(importance_work);
      }

      return false;
    }
  }

  luma_row_curr = (float *)malloc(sizeof(float) * image->width);
  luma_row_next = (float *)malloc(sizeof(float) * image->width);
  if (!luma_row_curr || !luma_row_next) {
    free(importance_work);
    free(buckets);
    free(luma_row_curr);
    free(luma_row_next);

    return false;
  }

  for (x = 0; x < image->width; ++x) {
    base = (size_t)x * 4;
    luma_row_curr[x] = calc_luma(image->rgba[base + 0], image->rgba[base + 1], image->rgba[base + 2]) / 255.0f;
  }

  for (y = 0; y < image->height; ++y) {
    if (y + 1 < image->height) {
      next_row_base = ((size_t)(y + 1) * (size_t)image->width) * 4;
      for (x = 0; x < image->width; ++x) {
        base = next_row_base + (size_t)x * 4;
        luma_row_next[x] = calc_luma(image->rgba[base + 0], image->rgba[base + 1], image->rgba[base + 2]) / 255.0f;
      }
    }

    for (x = 0; x < image->width; ++x) {
      base = (((size_t)y * (size_t)image->width) + (size_t)x) * 4;
      r = image->rgba[base + 0];
      g = image->rgba[base + 1];
      b = image->rgba[base + 2];
      a = image->rgba[base + 3];
      luma = luma_row_curr[x];
      gradient = 0.0f;
      saturation = calc_saturation(r, g, b);
      alpha_factor = (float)a / 255.0f;

      if (x + 1 < image->width) {
        right_luma = luma_row_curr[x + 1];
        gradient += absf(right_luma - luma);
      }

      if (y + 1 < image->height) {
        below_luma = luma_row_next[x];
        gradient += absf(below_luma - luma);
      }

      gradient *= PNGX_PREPARE_GRADIENT_SCALE;
      if (gradient > 1.0f) {
        gradient = 1.0f;
      }

      gradient_sum += gradient;
      if (gradient > stats->gradient_max) {
        stats->gradient_max = gradient;
      }

      saturation_sum += saturation;

      if (a > PNGX_DITHER_OPAQUE_THRESHOLD) {
        ++opaque_pixels;
      } else if (a > PNGX_DITHER_TRANSLUCENT_THRESHOLD) {
        ++translucent_pixels;
      }

      if (saturation > PNGX_PREPARE_VIBRANT_SATURATION && gradient > PNGX_PREPARE_VIBRANT_GRADIENT && a > PNGX_PREPARE_VIBRANT_ALPHA) {
        ++vibrant_pixels;
      }

      importance = gradient;

      if (opts->chroma_weight_enable) {
        importance += saturation * PNGX_PREPARE_CHROMA_WEIGHT;
      }

      if (opts->gradient_boost_enable) {
        if (gradient > PNGX_PREPARE_BOOST_THRESHOLD) {
          importance += PNGX_PREPARE_BOOST_BASE + (gradient * PNGX_PREPARE_BOOST_FACTOR);
        } else if (gradient < PNGX_PREPARE_CUT_THRESHOLD) {
          importance *= PNGX_PREPARE_CUT_FACTOR;
        }
      }

      if (alpha_factor < PNGX_PREPARE_ALPHA_THRESHOLD) {
        importance *= (PNGX_PREPARE_ALPHA_BASE + alpha_factor * PNGX_PREPARE_ALPHA_MULTIPLIER);
      }

      if (importance < 0.0f) {
        importance = 0.0f;
      } else if (importance > 1.0f) {
        importance = 1.0f;
      }

      if (need_map && importance_work) {
        pixel_index = ((size_t)y * (size_t)image->width) + (size_t)x;
        importance_work[pixel_index] = (uint16_t)(importance * PNGX_IMPORTANCE_SCALE + 0.5f);

        if (importance_work[pixel_index] < raw_min) {
          raw_min = importance_work[pixel_index];
        }

        if (importance_work[pixel_index] > raw_max) {
          raw_max = importance_work[pixel_index];
        }
      }

      if (need_buckets && buckets) {
        if ((saturation > PNGX_PREPARE_BUCKET_SATURATION || importance > PNGX_PREPARE_BUCKET_IMPORTANCE) && a > PNGX_PREPARE_BUCKET_ALPHA) {
          importance_mix = (importance * PNGX_PREPARE_MIX_IMPORTANCE) + (gradient * PNGX_PREPARE_MIX_GRADIENT);
          anchor_score = (saturation * PNGX_PREPARE_ANCHOR_SATURATION) + (importance_mix * PNGX_PREPARE_ANCHOR_MIX);
          if (importance > PNGX_PREPARE_ANCHOR_IMP_THRESHOLD) {
            anchor_score += PNGX_PREPARE_ANCHOR_IMP_BONUS;
          }
          if (anchor_score > PNGX_PREPARE_ANCHOR_SCORE_THRESHOLD) {
            bucket_entry = &buckets[chroma_bucket_index(r, g, b)];
            bucket_entry->r_sum += (uint64_t)r;
            bucket_entry->g_sum += (uint64_t)g;
            bucket_entry->b_sum += (uint64_t)b;
            bucket_entry->a_sum += (uint64_t)a;
            bucket_entry->count += 1;
            bucket_entry->score += anchor_score;
            bucket_entry->importance_accum += importance;
          }
        }
      }
    }

    luma_row_tmp = luma_row_curr;
    luma_row_curr = luma_row_next;
    luma_row_next = luma_row_tmp;
  }

  free(luma_row_curr);
  free(luma_row_next);

  if (image->pixel_count > 0) {
    stats->gradient_mean = gradient_sum / (float)image->pixel_count;
    stats->saturation_mean = saturation_sum / (float)image->pixel_count;
    stats->opaque_ratio = (float)opaque_pixels / (float)image->pixel_count;
    stats->translucent_ratio = (float)translucent_pixels / (float)image->pixel_count;
    stats->vibrant_ratio = (float)vibrant_pixels / (float)image->pixel_count;
  }
  if (need_map && importance_work && image->pixel_count > 0) {
    range = (uint32_t)(raw_max - raw_min);
    if (range == 0) {
      range = 1;
    }

    support->importance_map = (uint8_t *)malloc(image->pixel_count);
    if (support->importance_map) {
      for (pixel_index = 0; pixel_index < image->pixel_count; ++pixel_index) {
        sample = (uint32_t)(importance_work[pixel_index] - raw_min);
        value = (uint8_t)((sample * 255) / range);
        if (value < 4) {
          value = 4;
        }
        support->importance_map[pixel_index] = value;
      }
      support->importance_map_len = image->pixel_count;
    }
  }

  free(importance_work);

  if (need_buckets && buckets) {
    extract_chroma_anchors(support, buckets, PNGX_CHROMA_BUCKET_COUNT, image->pixel_count);
  }

  free(buckets);

  return true;
}
