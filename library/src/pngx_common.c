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

#include <colopresso.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "internal/log.h"
#include "internal/png.h"
#include "internal/pngx_common.h"
#include "internal/simd.h"
#include "internal/threads.h"

typedef struct {
  uint64_t r_sum;
  uint64_t g_sum;
  uint64_t b_sum;
  uint64_t a_sum;
  uint32_t count;
  float score;
  float importance_accum;
} chroma_bucket_t;

typedef struct {
  uint8_t *rgba;
  size_t pixel_count;
  uint8_t bits_rgb;
  uint8_t bits_alpha;
} snap_rgba_parallel_ctx_t;

static void snap_rgba_parallel_worker(void *context, uint32_t start, uint32_t end) {
  snap_rgba_parallel_ctx_t *ctx = (snap_rgba_parallel_ctx_t *)context;
  uint8_t r, g, b, a;
  size_t base, i;

  if (!ctx || !ctx->rgba) {
    return;
  }

  for (i = (size_t)start; i < (size_t)end && i < ctx->pixel_count; ++i) {
    base = i * 4;
    r = ctx->rgba[base + 0];
    g = ctx->rgba[base + 1];
    b = ctx->rgba[base + 2];
    a = ctx->rgba[base + 3];

    snap_rgba_to_bits(&r, &g, &b, &a, ctx->bits_rgb, ctx->bits_alpha);

    ctx->rgba[base + 0] = r;
    ctx->rgba[base + 1] = g;
    ctx->rgba[base + 2] = b;
    ctx->rgba[base + 3] = a;
  }
}

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

static inline float calc_luma(uint8_t r, uint8_t g, uint8_t b) { return PNGX_COMMON_LUMA_R_COEFF * (float)r + PNGX_COMMON_LUMA_G_COEFF * (float)g + PNGX_COMMON_LUMA_B_COEFF * (float)b; }

static inline bool decode_png_rgba(const uint8_t *png_data, size_t png_size, uint8_t **rgba, png_uint_32 *width, png_uint_32 *height) {
  cpres_error_t status;

  if (!png_data || png_size == 0 || !rgba || !width || !height) {
    return false;
  }

  status = png_decode_from_memory(png_data, png_size, rgba, width, height);
  if (status != CPRES_OK) {
    colopresso_log(CPRES_LOG_LEVEL_WARNING, "PNGX: Failed to decode PNG (%d)", (int)status);
    return false;
  }

  return true;
}

static inline void extract_chroma_anchors(pngx_quant_support_t *support, chroma_bucket_t *buckets, size_t bucket_count, size_t pixel_count) {
  cpres_rgba_color_t chosen[PNGX_MAX_DERIVED_COLORS];
  chroma_bucket_t *bucket;
  size_t selected, best_index, i, dst, auto_limit, scaled, check;
  float best_score, score, importance_boost;
  bool duplicate;

  if (!support || !buckets || bucket_count == 0) {
    return;
  }

  if (pixel_count > 0) {
    scaled = pixel_count / (size_t)PNGX_COMMON_ANCHOR_SCALE_DIVISOR;
    if (scaled < (size_t)PNGX_COMMON_ANCHOR_SCALE_MIN) {
      scaled = (size_t)PNGX_COMMON_ANCHOR_SCALE_MIN;
    }
    if (scaled > PNGX_MAX_DERIVED_COLORS) {
      scaled = PNGX_MAX_DERIVED_COLORS;
    }
    auto_limit = scaled;
  } else {
    auto_limit = (size_t)PNGX_COMMON_ANCHOR_AUTO_LIMIT_DEFAULT;
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
      importance_boost = bucket->importance_accum * PNGX_COMMON_ANCHOR_IMPORTANCE_FACTOR;
      if (importance_boost > PNGX_COMMON_ANCHOR_IMPORTANCE_THRESHOLD) {
        importance_boost = PNGX_COMMON_ANCHOR_IMPORTANCE_BOOST_BASE + (importance_boost - PNGX_COMMON_ANCHOR_IMPORTANCE_THRESHOLD) * PNGX_COMMON_ANCHOR_IMPORTANCE_BOOST_SCALE;
      }
      score = bucket->score + importance_boost;
      if (bucket->count < PNGX_COMMON_ANCHOR_LOW_COUNT_THRESHOLD) {
        score *= PNGX_COMMON_ANCHOR_LOW_COUNT_PENALTY;
      }
      if (score > best_score) {
        best_score = score;
        best_index = i;
      }
    }
    if (best_index == SIZE_MAX || best_score < PNGX_COMMON_ANCHOR_SCORE_THRESHOLD) {
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
      if (color_distance_sq(&chosen[i], &chosen[check]) < PNGX_COMMON_ANCHOR_DISTANCE_SQ_THRESHOLD) {
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

  return bits < min_bits ? min_bits : (bits > max_bits ? max_bits : bits);
}

uint8_t quantize_channel_value(float value, uint8_t bits_per_channel) {
  uint32_t levels;
  float clamped, scaled, rounded, quantized;

  if (bits_per_channel >= 8) {
    return value < 0.0f ? 0 : (value > 255.0f ? 255 : (uint8_t)(value + 0.5f));
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
  uint8_t rgb = clamp_reduced_bits(bits_rgb);

  if (r) {
    *r = quantize_bits(*r, rgb);
  }
  if (g) {
    *g = quantize_bits(*g, rgb);
  }
  if (b) {
    *b = quantize_bits(*b, rgb);
  }
  if (a) {
    *a = quantize_bits(*a, clamp_reduced_bits(bits_alpha));
  }
}

void snap_rgba_image_to_bits(uint32_t thread_count, uint8_t *rgba, size_t pixel_count, uint8_t bits_rgb, uint8_t bits_alpha) {
  snap_rgba_parallel_ctx_t ctx;

  if (!rgba || pixel_count == 0) {
    return;
  }

  if (clamp_reduced_bits(bits_rgb) >= 8 && clamp_reduced_bits(bits_alpha) >= 8) {
    return;
  }

  ctx.rgba = rgba;
  ctx.pixel_count = pixel_count;
  ctx.bits_rgb = bits_rgb;
  ctx.bits_alpha = bits_alpha;

#if COLOPRESSO_ENABLE_THREADS
  colopresso_parallel_for(thread_count, (uint32_t)pixel_count, snap_rgba_parallel_worker, &ctx);
#else
  snap_rgba_parallel_worker(&ctx, 0, (uint32_t)pixel_count);
#endif
}

uint32_t color_distance_sq(const cpres_rgba_color_t *lhs, const cpres_rgba_color_t *rhs) {
  uint32_t lhs_packed, rhs_packed;

  if (!lhs || !rhs) {
    return 0;
  }

  lhs_packed = (uint32_t)lhs->r | ((uint32_t)lhs->g << 8) | ((uint32_t)lhs->b << 16) | ((uint32_t)lhs->a << 24);
  rhs_packed = (uint32_t)rhs->r | ((uint32_t)rhs->g << 8) | ((uint32_t)rhs->b << 16) | ((uint32_t)rhs->a << 24);

  return simd_color_distance_sq_u32(lhs_packed, rhs_packed);
}

float estimate_bitdepth_dither_level(const uint8_t *rgba, png_uint_32 width, png_uint_32 height, uint8_t bits_per_channel) {
  png_uint_32 y, x;
  uint8_t r, g, b, a;
  size_t pixel_count, gradient_samples, opaque_pixels, translucent_pixels, base, right, below;
  float base_level, target, right_luma, below_luma, luma, normalized_gradient, opaque_ratio, translucent_ratio, min_luma, max_luma, coverage, gradient_span;
  double gradient_accum;

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

      if (a > PNGX_COMMON_DITHER_ALPHA_OPAQUE_THRESHOLD) {
        ++opaque_pixels;
      } else if (a > PNGX_COMMON_DITHER_ALPHA_TRANSLUCENT_THRESHOLD) {
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

  gradient_span = coverage / ((normalized_gradient > PNGX_COMMON_DITHER_GRADIENT_MIN) ? normalized_gradient : PNGX_COMMON_DITHER_GRADIENT_MIN);

  base_level = PNGX_COMMON_DITHER_BASE_LEVEL;
  target = base_level;

  if (normalized_gradient > 0.35f) {
    target += PNGX_COMMON_DITHER_HIGH_GRADIENT_BOOST;
  } else if (normalized_gradient > 0.2f) {
    target += PNGX_COMMON_DITHER_MID_GRADIENT_BOOST;
  } else if (normalized_gradient < 0.08f) {
    target -= PNGX_COMMON_DITHER_LOW_GRADIENT_CUT;
  } else if (normalized_gradient < 0.15f) {
    target -= PNGX_COMMON_DITHER_MID_LOW_GRADIENT_CUT;
  }

  if (opaque_ratio < 0.35f) {
    target -= PNGX_COMMON_DITHER_OPAQUE_LOW_CUT;
  } else if (opaque_ratio > 0.9f) {
    target += PNGX_COMMON_DITHER_OPAQUE_HIGH_BOOST;
  }

  if (translucent_ratio > 0.3f) {
    target -= PNGX_COMMON_DITHER_TRANSLUCENT_CUT;
  }

  if (coverage > PNGX_COMMON_DITHER_COVERAGE_THRESHOLD && gradient_span > PNGX_COMMON_DITHER_SPAN_THRESHOLD) {
    if (target < PNGX_COMMON_DITHER_TARGET_CAP) {
      target = PNGX_COMMON_DITHER_TARGET_CAP;
    }
    if (bits_per_channel <= 4 && target < PNGX_COMMON_DITHER_TARGET_CAP_LOW_BIT) {
      target = PNGX_COMMON_DITHER_TARGET_CAP_LOW_BIT;
    }
  }

  if (bits_per_channel <= 2) {
    target += PNGX_COMMON_DITHER_LOW_BIT_BOOST;
    if (normalized_gradient > 0.25f) {
      target += PNGX_COMMON_DITHER_LOW_BIT_GRADIENT_BOOST;
    }
  }

  return clamp_float(target, PNGX_COMMON_DITHER_MIN, PNGX_COMMON_DITHER_MAX);
}

float estimate_bitdepth_dither_level_limited4444(const uint8_t *rgba, png_uint_32 width, png_uint_32 height) {
  uint8_t r, g, b, a;
  size_t pixel_count, gradient_samples, opaque_pixels, translucent_pixels, base, right, below;
  float right_luma, below_luma, luma, normalized_gradient, opaque_ratio, translucent_ratio, min_luma, max_luma, coverage, gradient_span, target;
  double gradient_accum;
  png_uint_32 y, x;

  if (!rgba || width == 0 || height == 0) {
    return 0.0f;
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

      if (a > PNGX_COMMON_DITHER_ALPHA_OPAQUE_THRESHOLD) {
        ++opaque_pixels;
      } else if (a > PNGX_COMMON_DITHER_ALPHA_TRANSLUCENT_THRESHOLD) {
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

  gradient_span = coverage / ((normalized_gradient > PNGX_COMMON_DITHER_GRADIENT_MIN) ? normalized_gradient : PNGX_COMMON_DITHER_GRADIENT_MIN);

  target = 0.0f;

  if (coverage > PNGX_COMMON_DITHER_COVERAGE_THRESHOLD && gradient_span > PNGX_COMMON_DITHER_SPAN_THRESHOLD && normalized_gradient < 0.20f) {
    target = 0.55f;
  }

  if (translucent_ratio > 0.3f) {
    target *= 0.5f;
  }

  if (opaque_ratio > 0.9f) {
    target += 0.05f;
  }

  return clamp_float(target, 0.0f, 1.0f);
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
      if (color_distance_sq(&support->derived_colors[i], &support->combined_fixed_colors[j]) < PNGX_COMMON_FIXED_PALETTE_DISTANCE_SQ) {
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
    if (support->combined_fixed_len >= PNGX_COMMON_FIXED_PALETTE_MAX) {
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
    gradient_mean = PNGX_COMMON_RESOLVE_DEFAULT_GRADIENT;
    saturation_mean = PNGX_COMMON_RESOLVE_DEFAULT_SATURATION;
    opaque_ratio = PNGX_COMMON_RESOLVE_DEFAULT_OPAQUE;
    vibrant_ratio = PNGX_COMMON_RESOLVE_DEFAULT_VIBRANT;
    gradient_max = PNGX_COMMON_RESOLVE_DEFAULT_GRADIENT;
  }

  resolved = opts->lossy_dither_level;

  if (opts->lossy_dither_auto) {
    resolved = PNGX_COMMON_RESOLVE_AUTO_BASE + gradient_mean * PNGX_COMMON_RESOLVE_AUTO_GRADIENT_WEIGHT + saturation_mean * PNGX_COMMON_RESOLVE_AUTO_SATURATION_WEIGHT;
    if (opaque_ratio < 0.7f) {
      resolved -= PNGX_COMMON_RESOLVE_AUTO_OPAQUE_CUT;
    }
  }

  if (opts->adaptive_dither_enable) {
    if (gradient_mean < 0.10f) {
      resolved -= PNGX_COMMON_RESOLVE_ADAPTIVE_FLAT_CUT;
    } else if (gradient_mean > 0.30f) {
      resolved += PNGX_COMMON_RESOLVE_ADAPTIVE_GRADIENT_BOOST;
    }
    if (gradient_max > 0.5f && vibrant_ratio > 0.12f) {
      resolved -= PNGX_COMMON_RESOLVE_ADAPTIVE_VIBRANT_CUT;
    }
    if (saturation_mean > 0.38f) {
      resolved += PNGX_COMMON_RESOLVE_ADAPTIVE_SATURATION_BOOST;
    } else if (saturation_mean < 0.12f) {
      resolved -= PNGX_COMMON_RESOLVE_ADAPTIVE_SATURATION_CUT;
    }
  }

  if (resolved < PNGX_COMMON_RESOLVE_MIN) {
    resolved = PNGX_COMMON_RESOLVE_MIN;
  } else if (resolved > PNGX_COMMON_RESOLVE_MAX) {
    resolved = PNGX_COMMON_RESOLVE_MAX;
  }

  return resolved;
}

bool prepare_quant_support(const pngx_rgba_image_t *image, const pngx_options_t *opts, pngx_quant_support_t *support, pngx_image_stats_t *stats) {
  chroma_bucket_t *buckets = NULL, *bucket_entry;
  uint32_t x, y, range, sample;
  uint16_t *importance_work = NULL, raw_min, raw_max;
  uint8_t r, g, b, a, value;
  size_t pixel_index, base, next_row_base, opaque_pixels, translucent_pixels, vibrant_pixels;
  float gradient_sum, saturation_sum, luma, gradient, saturation, importance, alpha_factor, anchor_score, right_luma, below_luma, importance_mix, *luma_row_curr = NULL, *luma_row_next = NULL,
                                                                                                                                                  *luma_row_tmp;
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
    buckets = (chroma_bucket_t *)calloc(PNGX_CHROMA_BUCKET_COUNT, sizeof(chroma_bucket_t));
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

      gradient *= PNGX_COMMON_PREPARE_GRADIENT_SCALE;
      if (gradient > 1.0f) {
        gradient = 1.0f;
      }

      gradient_sum += gradient;
      if (gradient > stats->gradient_max) {
        stats->gradient_max = gradient;
      }

      saturation_sum += saturation;

      if (a > PNGX_COMMON_DITHER_ALPHA_OPAQUE_THRESHOLD) {
        ++opaque_pixels;
      } else if (a > PNGX_COMMON_DITHER_ALPHA_TRANSLUCENT_THRESHOLD) {
        ++translucent_pixels;
      }

      if (saturation > PNGX_COMMON_PREPARE_VIBRANT_SATURATION && gradient > PNGX_COMMON_PREPARE_VIBRANT_GRADIENT && a > PNGX_COMMON_PREPARE_VIBRANT_ALPHA) {
        ++vibrant_pixels;
      }

      importance = gradient;

      if (opts->chroma_weight_enable) {
        importance += saturation * PNGX_COMMON_PREPARE_CHROMA_WEIGHT;
      }

      if (opts->gradient_boost_enable) {
        if (gradient > PNGX_COMMON_PREPARE_BOOST_THRESHOLD) {
          importance += PNGX_COMMON_PREPARE_BOOST_BASE + (gradient * PNGX_COMMON_PREPARE_BOOST_FACTOR);
        } else if (gradient < PNGX_COMMON_PREPARE_CUT_THRESHOLD) {
          importance *= PNGX_COMMON_PREPARE_CUT_FACTOR;
        }
      }

      if (alpha_factor < PNGX_COMMON_PREPARE_ALPHA_THRESHOLD) {
        importance *= (PNGX_COMMON_PREPARE_ALPHA_BASE + alpha_factor * PNGX_COMMON_PREPARE_ALPHA_MULTIPLIER);
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
        if ((saturation > PNGX_COMMON_PREPARE_BUCKET_SATURATION || importance > PNGX_COMMON_PREPARE_BUCKET_IMPORTANCE) && a > PNGX_COMMON_PREPARE_BUCKET_ALPHA) {
          importance_mix = (importance * PNGX_COMMON_PREPARE_MIX_IMPORTANCE) + (gradient * PNGX_COMMON_PREPARE_MIX_GRADIENT);
          anchor_score = (saturation * PNGX_COMMON_PREPARE_ANCHOR_SATURATION) + (importance_mix * PNGX_COMMON_PREPARE_ANCHOR_MIX);
          if (importance > PNGX_COMMON_PREPARE_ANCHOR_IMPORTANCE_THRESHOLD) {
            anchor_score += PNGX_COMMON_PREPARE_ANCHOR_IMPORTANCE_BONUS;
          }
          if (anchor_score > PNGX_COMMON_PREPARE_ANCHOR_SCORE_THRESHOLD) {
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
        if (value < PNGX_COMMON_PREPARE_MAP_MIN_VALUE) {
          value = (uint8_t)PNGX_COMMON_PREPARE_MAP_MIN_VALUE;
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
