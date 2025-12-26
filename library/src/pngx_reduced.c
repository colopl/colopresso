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

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>

#include "internal/log.h"
#include "internal/pngx_common.h"
#include "internal/simd.h"
#include "internal/threads.h"

typedef struct {
  uint32_t color;
  uint32_t count;
  uint32_t mapped_color;
  uint8_t detail_bits_rgb;
  uint8_t detail_bits_alpha;
  bool locked;
} color_entry_t;

typedef struct {
  color_entry_t *entries;
  size_t count;
  size_t unlocked_count;
} color_histogram_t;

typedef struct {
  uint32_t color;
  uint16_t weight;
  uint8_t rgb_bits;
  uint8_t alpha_bits;
} histogram_sample_t;

typedef struct {
  size_t start;
  size_t end;
  uint8_t min_r;
  uint8_t min_g;
  uint8_t min_b;
  uint8_t min_a;
  uint8_t max_r;
  uint8_t max_g;
  uint8_t max_b;
  uint8_t max_a;
  uint64_t total_weight;
} color_box_t;

typedef struct {
  uint32_t color;
  uint32_t mapped_color;
} color_map_entry_t;

typedef struct {
  uint32_t color;
  uint32_t count;
} color_freq_t;

typedef struct {
  size_t index;
  uint32_t count;
  uint32_t color;
} freq_rank_t;

typedef struct {
  uint8_t *rgba;
  size_t pixel_count;
  const uint8_t *importance_map;
  size_t importance_map_len;
  uint8_t bits_rgb;
  uint8_t bits_alpha;
  uint8_t boost_bits_rgb;
  uint8_t boost_bits_alpha;
  uint8_t *bit_hint_map;
  size_t bit_hint_len;
} reduce_bitdepth_parallel_ctx_t;

typedef struct {
  uint8_t *rgba;
  size_t pixel_count;
  const color_map_entry_t *map;
  size_t map_count;
} color_remap_parallel_ctx_t;

typedef struct {
  const uint8_t *rgba;
  uint32_t *packed;
  size_t pixel_count;
} pack_rgba_parallel_ctx_t;

static int compare_entries_r(const void *lhs, const void *rhs) {
  const color_entry_t *a = (const color_entry_t *)lhs, *b = (const color_entry_t *)rhs;
  uint8_t av = (uint8_t)(a->color & 0xff), bv = (uint8_t)(b->color & 0xff);

  return (av < bv) ? -1 : ((av > bv) ? 1 : 0);
}

static int compare_entries_g(const void *lhs, const void *rhs) {
  const color_entry_t *a = (const color_entry_t *)lhs, *b = (const color_entry_t *)rhs;
  uint8_t av = (uint8_t)((a->color >> 8) & 0xff), bv = (uint8_t)((b->color >> 8) & 0xff);

  return (av < bv) ? -1 : ((av > bv) ? 1 : 0);
}

static int compare_entries_b(const void *lhs, const void *rhs) {
  const color_entry_t *a = (const color_entry_t *)lhs, *b = (const color_entry_t *)rhs;
  uint8_t av = (uint8_t)((a->color >> 16) & 0xff), bv = (uint8_t)((b->color >> 16) & 0xff);

  return (av < bv) ? -1 : ((av > bv) ? 1 : 0);
}

static int compare_entries_a(const void *lhs, const void *rhs) {
  const color_entry_t *a = (const color_entry_t *)lhs, *b = (const color_entry_t *)rhs;
  uint8_t av = (uint8_t)((a->color >> 24) & 0xff), bv = (uint8_t)((b->color >> 24) & 0xff);

  return (av < bv) ? -1 : ((av > bv) ? 1 : 0);
}

static inline uint32_t compute_grid_capacity(uint8_t bits_rgb, uint8_t bits_alpha) {
  uint64_t capacity;
  uint32_t rgb_levels = 1, alpha_levels = 1;

  bits_rgb = clamp_reduced_bits(bits_rgb);
  bits_alpha = clamp_reduced_bits(bits_alpha);

  if (bits_rgb < 8) {
    rgb_levels = (uint32_t)1 << bits_rgb;
    rgb_levels = rgb_levels * rgb_levels * rgb_levels;
  } else {
    rgb_levels = UINT32_MAX;
  }

  if (bits_alpha < 8) {
    alpha_levels = (uint32_t)1 << bits_alpha;
  } else {
    alpha_levels = UINT32_MAX;
  }

  if (rgb_levels == UINT32_MAX || alpha_levels == UINT32_MAX) {
    return (uint32_t)COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
  }

  capacity = (uint64_t)rgb_levels * (uint64_t)alpha_levels;
  if (capacity > (uint64_t)COLOPRESSO_PNGX_REDUCED_COLORS_MAX) {
    capacity = (uint64_t)COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
  }

  return (uint32_t)capacity;
}

static inline uint32_t pack_rgba_u32(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | (uint32_t)r; }

static uint32_t box_representative_color(const color_entry_t *entries, const color_box_t *box) {
  uint64_t sum_r = 0, sum_g = 0, sum_b = 0, sum_a = 0, total = 0, weight;
  uint32_t c;
  size_t i;

  if (!entries || !box) {
    return 0;
  }

  for (i = box->start; i < box->end; ++i) {
    c = entries[i].color;
    weight = (uint64_t)entries[i].count;
    sum_r += (uint64_t)(c & 0xff) * weight;
    sum_g += (uint64_t)((c >> 8) & 0xff) * weight;
    sum_b += (uint64_t)((c >> 16) & 0xff) * weight;
    sum_a += (uint64_t)((c >> 24) & 0xff) * weight;
    total += weight;
  }

  if (total == 0) {
    return entries[box->start].color;
  }

  sum_r = (sum_r + (total / 2)) / total;
  sum_g = (sum_g + (total / 2)) / total;
  sum_b = (sum_b + (total / 2)) / total;
  sum_a = (sum_a + (total / 2)) / total;

  return pack_rgba_u32((uint8_t)sum_r, (uint8_t)sum_g, (uint8_t)sum_b, (uint8_t)sum_a);
}

static inline uint8_t color_component(uint32_t color, uint8_t channel) { return (uint8_t)((color >> (channel * 8)) & 0xff); }

static inline void unpack_rgba_u32(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a) {
  if (r) {
    *r = (uint8_t)(color & 0xff);
  }

  if (g) {
    *g = (uint8_t)((color >> 8) & 0xff);
  }

  if (b) {
    *b = (uint8_t)((color >> 16) & 0xff);
  }

  if (a) {
    *a = (uint8_t)((color >> 24) & 0xff);
  }
}

static inline float importance_dither_scale(uint8_t importance_value) {
  float normalized, scale;

  normalized = (float)importance_value / PNGX_REDUCED_IMPORTANCE_SCALE_DENOM;
  scale = PNGX_REDUCED_IMPORTANCE_SCALE_MIN + (1.0f - normalized) * PNGX_REDUCED_IMPORTANCE_SCALE_RANGE;

  return scale < 0.0f ? 0.0f : (scale > 1.0f ? 1.0f : scale);
}

static inline void color_histogram_reset(color_histogram_t *hist) {
  if (!hist) {
    return;
  }

  free(hist->entries);
  hist->entries = NULL;
  hist->count = 0;
  hist->unlocked_count = 0;
}

static inline bool is_protected_color(uint32_t color, const uint32_t *protected_colors, size_t protected_count) {
  size_t i;

  if (!protected_colors || protected_count == 0) {
    return false;
  }

  for (i = 0; i < protected_count; ++i) {
    if (protected_colors[i] == color) {
      return true;
    }
  }

  return false;
}

static inline uint16_t histogram_importance_weight(const pngx_quant_support_t *support, size_t pixel_index) {
  const uint8_t *map;
  uint16_t weight;
  uint8_t importance;

  if (!support || !support->importance_map || pixel_index >= support->importance_map_len) {
    return 1;
  }

  map = support->importance_map;
  importance = map[pixel_index];

  weight = 1 + (uint16_t)(importance >> PNGX_REDUCED_IMPORTANCE_WEIGHT_SHIFT);
  if (importance > PNGX_REDUCED_IMPORTANCE_WEIGHT_THRESHOLD_STRONG) {
    weight += PNGX_REDUCED_IMPORTANCE_WEIGHT_BONUS_STRONG;
  } else if (importance > PNGX_REDUCED_IMPORTANCE_WEIGHT_THRESHOLD_HIGH) {
    weight += PNGX_REDUCED_IMPORTANCE_WEIGHT_BONUS_HIGH;
  } else if (importance > PNGX_REDUCED_IMPORTANCE_WEIGHT_THRESHOLD_MEDIUM) {
    weight += PNGX_REDUCED_IMPORTANCE_WEIGHT_BONUS_MEDIUM;
  }

  if (weight > PNGX_REDUCED_IMPORTANCE_WEIGHT_CAP) {
    weight = PNGX_REDUCED_IMPORTANCE_WEIGHT_CAP;
  }

  return weight;
}

static inline int compare_histogram_sample(const void *lhs, const void *rhs) {
  const histogram_sample_t *a = (const histogram_sample_t *)lhs, *b = (const histogram_sample_t *)rhs;

  return a->color < b->color ? -1 : (a->color > b->color ? 1 : 0);
}

static inline void color_box_refresh(const color_entry_t *entries, color_box_t *box) {
  uint32_t c;
  uint8_t r, g, b, a;
  size_t i;

  if (!entries || !box || box->end <= box->start) {
    return;
  }

  box->min_r = 255;
  box->min_g = 255;
  box->min_b = 255;
  box->min_a = 255;
  box->max_r = 0;
  box->max_g = 0;
  box->max_b = 0;
  box->max_a = 0;
  box->total_weight = 0;
  for (i = box->start; i < box->end; ++i) {
    c = entries[i].color;
    r = (uint8_t)(c & 0xff);
    g = (uint8_t)((c >> 8) & 0xff);
    b = (uint8_t)((c >> 16) & 0xff);
    a = (uint8_t)((c >> 24) & 0xff);
    if (r < box->min_r) {
      box->min_r = r;
    }
    if (r > box->max_r) {
      box->max_r = r;
    }
    if (g < box->min_g) {
      box->min_g = g;
    }
    if (g > box->max_g) {
      box->max_g = g;
    }
    if (b < box->min_b) {
      box->min_b = b;
    }
    if (b > box->max_b) {
      box->max_b = b;
    }
    if (a < box->min_a) {
      box->min_a = a;
    }
    if (a > box->max_a) {
      box->max_a = a;
    }
    box->total_weight += (uint64_t)entries[i].count;
  }
}

static inline int compare_u32(const void *lhs, const void *rhs) {
  uint32_t a = *(const uint32_t *)lhs, b = *(const uint32_t *)rhs;

  if (a < b) {
    return -1;
  }

  if (a > b) {
    return 1;
  }

  return 0;
}

static inline bool color_box_splittable(const color_box_t *box) { return box && box->end > box->start + 1; }

static inline uint8_t color_box_max_span(const color_box_t *box) {
  uint8_t span_r = box->max_r - box->min_r, span_g = box->max_g - box->min_g, span_b = box->max_b - box->min_b, span_a = box->max_a - box->min_a, span = span_r;

  return span_g > span ? span_g : (span_b > span ? span_b : (span_a > span ? span_a : span));
}

static inline float color_box_detail_bias(const color_entry_t *entries, const color_box_t *box, uint8_t base_bits_rgb, uint8_t base_bits_alpha) {
  const color_entry_t *entry;
  uint64_t weight = 0, w, accum_x2 = 0;
  uint32_t delta_rgb, delta_alpha, score_x2;
  size_t idx;

  if (!entries || !box || box->end <= box->start) {
    return 0.0f;
  }

  for (idx = box->start; idx < box->end; ++idx) {
    entry = &entries[idx];
    w = entry->count ? entry->count : 1;
    delta_rgb = (entry->detail_bits_rgb > base_bits_rgb) ? (uint32_t)(entry->detail_bits_rgb - base_bits_rgb) : 0;
    delta_alpha = (entry->detail_bits_alpha > base_bits_alpha) ? (uint32_t)(entry->detail_bits_alpha - base_bits_alpha) : 0;
    score_x2 = (delta_rgb << 1) + delta_alpha;

    if (score_x2 > 0) {
      accum_x2 += (uint64_t)score_x2 * w;
    }
    weight += w;
  }

  if (weight == 0) {
    return 0.0f;
  }

  return (float)accum_x2 / (float)(weight * 2);
}

static inline int select_box_to_split(const color_box_t *boxes, size_t box_count, const color_entry_t *entries, uint8_t base_bits_rgb, uint8_t base_bits_alpha) {
  size_t i;
  int best_index = -1;
  float best_priority = -1.0f, detail_bias, span_score, weight_score, priority;

  if (!boxes || box_count == 0) {
    return -1;
  }

  for (i = 0; i < box_count; ++i) {
    if (!color_box_splittable(&boxes[i])) {
      continue;
    }

    detail_bias = color_box_detail_bias(entries, &boxes[i], base_bits_rgb, base_bits_alpha);
    span_score = (float)color_box_max_span(&boxes[i]) / 255.0f;
    weight_score = (boxes[i].total_weight > 0) ? logf((float)boxes[i].total_weight + 1.0f) : 0.0f;
    priority = (span_score * PNGX_REDUCED_PRIORITY_SPAN_WEIGHT) + (detail_bias * PNGX_REDUCED_PRIORITY_DETAIL_WEIGHT) + (weight_score * PNGX_REDUCED_PRIORITY_MASS_WEIGHT);

    if (priority > best_priority) {
      best_priority = priority;
      best_index = (int)i;
    }
  }

  return best_index;
}

static inline size_t box_find_split_index(const color_entry_t *entries, const color_box_t *box) {
  uint64_t half, accum = 0;
  size_t idx;

  if (!entries || !box || box->end <= box->start) {
    return box ? box->start : 0;
  }

  half = box->total_weight / 2;
  if (half == 0) {
    half = 1;
  }

  for (idx = box->start; idx < box->end; ++idx) {
    accum += (uint64_t)entries[idx].count;
    if (accum >= half) {
      return idx;
    }
  }

  return box->start + ((box->end - box->start) / 2);
}

static inline void sort_entries_by_axis(color_entry_t *entries, size_t count, int axis) {
  if (!entries || count == 0) {
    return;
  }

  switch (axis) {
  case 0:
    qsort(entries, count, sizeof(color_entry_t), compare_entries_r);
    break;
  case 1:
    qsort(entries, count, sizeof(color_entry_t), compare_entries_g);
    break;
  case 2:
    qsort(entries, count, sizeof(color_entry_t), compare_entries_b);
    break;
  case 3:
    qsort(entries, count, sizeof(color_entry_t), compare_entries_a);
    break;
  default:
    qsort(entries, count, sizeof(color_entry_t), compare_entries_r);
    break;
  }
}

static inline bool split_color_box(color_entry_t *entries, color_box_t *box, color_box_t *new_box) {
  size_t split_index;
  int axis;

  if (!entries || !box || !new_box || !color_box_splittable(box)) {
    return false;
  }

  axis = 0;
  if ((box->max_g - box->min_g) > (box->max_r - box->min_r)) {
    axis = 1;
  }
  if ((box->max_b - box->min_b) > (box->max_g - box->min_g) && (box->max_b - box->min_b) >= (box->max_r - box->min_r)) {
    axis = 2;
  }
  if ((box->max_a - box->min_a) > (box->max_b - box->min_b) && (box->max_a - box->min_a) >= (box->max_g - box->min_g) && (box->max_a - box->min_a) >= (box->max_r - box->min_r)) {
    axis = 3;
  }

  sort_entries_by_axis(entries + box->start, box->end - box->start, axis);

  split_index = box_find_split_index(entries, box);
  if (split_index <= box->start) {
    split_index = box->start + 1;
  }
  if (split_index >= box->end) {
    split_index = box->end - 1;
  }

  new_box->start = split_index;
  new_box->end = box->end;
  box->end = split_index;

  color_box_refresh(entries, box);
  color_box_refresh(entries, new_box);

  return true;
}

static inline int compare_color_map_by_color(const void *lhs, const void *rhs) {
  const color_map_entry_t *a = (const color_map_entry_t *)lhs, *b = (const color_map_entry_t *)rhs;

  return a->color < b->color ? -1 : (a->color > b->color ? 1 : 0);
}

static inline int compare_color_map_by_mapped(const void *lhs, const void *rhs) {
  const color_map_entry_t *a = (const color_map_entry_t *)lhs, *b = (const color_map_entry_t *)rhs;

  return a->mapped_color < b->mapped_color ? -1 : (a->mapped_color > b->mapped_color ? 1 : 0);
}

static inline const color_map_entry_t *find_color_mapping(const color_map_entry_t *map, size_t count, uint32_t color) {
  size_t left = 0, right = (count > 0) ? count - 1 : 0, mid;

  if (!map || count == 0) {
    return NULL;
  }

  while (left <= right) {
    mid = left + ((right - left) / 2);

    if (map[mid].color == color) {
      return &map[mid];
    }

    if (map[mid].color < color) {
      left = mid + 1;
    } else {
      if (mid == 0) {
        break;
      }
      right = mid - 1;
    }
  }

  return NULL;
}

static void color_remap_parallel_worker(void *context, uint32_t start, uint32_t end) {
  color_remap_parallel_ctx_t *ctx = (color_remap_parallel_ctx_t *)context;
  const color_map_entry_t *mapping;
  uint8_t *px;
  uint32_t original;
  size_t i, base;

  if (!ctx || !ctx->rgba || !ctx->map || ctx->map_count == 0) {
    return;
  }

  for (i = (size_t)start; i < (size_t)end && i < ctx->pixel_count; ++i) {
    base = i * PNGX_RGBA_CHANNELS;
    px = &ctx->rgba[base];
    if (px[3] <= PNGX_REDUCED_ALPHA_NEAR_TRANSPARENT) {
      continue;
    }
    original = pack_rgba_u32(px[0], px[1], px[2], px[3]);
    mapping = find_color_mapping(ctx->map, ctx->map_count, original);

    if (!mapping) {
      continue;
    }

    unpack_rgba_u32(mapping->mapped_color, &px[0], &px[1], &px[2], &px[3]);
  }
}

static inline void refine_reduced_palette(color_entry_t *entries, size_t unlocked_count, const uint32_t *seed_palette, const uint8_t *palette_bits_rgb, const uint8_t *palette_bits_alpha,
                                          size_t palette_count) {
  const size_t max_iter = 3;
  uint64_t *sum_r, *sum_g, *sum_b, *sum_a, *sum_w, w;
  uint32_t *palette, color, best_dist, weight, dist, new_color;
  uint8_t r, g, b, a, snap_bits_rgb, snap_bits_alpha, cr, cg, cb, ca;
  size_t iter, best_index, candidate, i, p;
  bool palette_changed;

  if (!entries || unlocked_count == 0 || !seed_palette || palette_count == 0 || palette_count > 4096) {
    return;
  }

  palette = (uint32_t *)malloc(sizeof(uint32_t) * palette_count);
  if (!palette) {
    return;
  }

  memcpy(palette, seed_palette, sizeof(uint32_t) * palette_count);

  sum_r = (uint64_t *)malloc(sizeof(uint64_t) * palette_count);
  sum_g = (uint64_t *)malloc(sizeof(uint64_t) * palette_count);
  sum_b = (uint64_t *)malloc(sizeof(uint64_t) * palette_count);
  sum_a = (uint64_t *)malloc(sizeof(uint64_t) * palette_count);
  sum_w = (uint64_t *)malloc(sizeof(uint64_t) * palette_count);

  if (!sum_r || !sum_g || !sum_b || !sum_a || !sum_w) {
    free(sum_r);
    free(sum_g);
    free(sum_b);
    free(sum_a);
    free(sum_w);
    free(palette);

    return;
  }

  for (iter = 0; iter < max_iter; ++iter) {
    palette_changed = false;

    memset(sum_r, 0, sizeof(uint64_t) * palette_count);
    memset(sum_g, 0, sizeof(uint64_t) * palette_count);
    memset(sum_b, 0, sizeof(uint64_t) * palette_count);
    memset(sum_a, 0, sizeof(uint64_t) * palette_count);
    memset(sum_w, 0, sizeof(uint64_t) * palette_count);

    for (i = 0; i < unlocked_count; ++i) {
      color = entries[i].color;
      best_dist = UINT32_MAX;
      best_index = 0;
      candidate = 0;
      weight = entries[i].count ? entries[i].count : 1;

      for (candidate = 0; candidate < palette_count; ++candidate) {
        dist = simd_color_distance_sq_u32(color, palette[candidate]);
        if (dist < best_dist) {
          best_dist = dist;
          best_index = candidate;
        }
      }

      unpack_rgba_u32(color, &cr, &cg, &cb, &ca);

      sum_r[best_index] += (uint64_t)cr * (uint64_t)weight;
      sum_g[best_index] += (uint64_t)cg * (uint64_t)weight;
      sum_b[best_index] += (uint64_t)cb * (uint64_t)weight;
      sum_a[best_index] += (uint64_t)ca * (uint64_t)weight;
      sum_w[best_index] += (uint64_t)weight;
    }

    for (p = 0; p < palette_count; ++p) {
      w = sum_w[p];
      if (w == 0) {
        continue;
      }
      r = (uint8_t)((sum_r[p] + (w / 2)) / w);
      g = (uint8_t)((sum_g[p] + (w / 2)) / w);
      b = (uint8_t)((sum_b[p] + (w / 2)) / w);
      a = (uint8_t)((sum_a[p] + (w / 2)) / w);
      snap_bits_rgb = palette_bits_rgb ? palette_bits_rgb[p] : COLOPRESSO_PNGX_REDUCED_BITS_MAX;
      snap_bits_alpha = palette_bits_alpha ? palette_bits_alpha[p] : COLOPRESSO_PNGX_REDUCED_BITS_MAX;
      snap_rgba_to_bits(&r, &g, &b, &a, snap_bits_rgb, snap_bits_alpha);
      new_color = pack_rgba_u32(r, g, b, a);
      if (new_color != palette[p]) {
        palette[p] = new_color;
        palette_changed = true;
      }
    }

    if (!palette_changed) {
      break;
    }
  }

  for (i = 0; i < unlocked_count; ++i) {
    color = entries[i].color;
    best_dist = UINT32_MAX;
    best_index = 0;

    for (candidate = 0; candidate < palette_count; ++candidate) {
      dist = simd_color_distance_sq_u32(color, palette[candidate]);
      if (dist < best_dist) {
        best_dist = dist;
        best_index = candidate;
      }
    }

    entries[i].mapped_color = palette[best_index];
  }

  free(sum_r);
  free(sum_g);
  free(sum_b);
  free(sum_a);
  free(sum_w);

  free(palette);
}

static inline bool build_color_histogram(const pngx_rgba_image_t *image, const pngx_options_t *opts, const pngx_quant_support_t *support, color_histogram_t *hist) {
  const cpres_rgba_color_t *protected_colors = (opts && opts->protected_colors_count > 0) ? opts->protected_colors : NULL;
  histogram_sample_t *samples = NULL;
  color_entry_t tmp;
  uint64_t weight_sum;
  uint32_t protected_table[256], color;
  uint8_t bits_rgb = 8, bits_alpha = 8, r, g, b, a, sample_bits_rgb, sample_bits_alpha, hint, hint_rgb, hint_alpha, max_bits_rgb, max_bits_alpha;
  size_t pixel_count, i, unique_count, base, run, write, protected_count = (protected_colors && opts->protected_colors_count > 0) ? (size_t)opts->protected_colors_count : 0;
  bool quantize_rgb = false, quantize_alpha = false;

  if (!image || !image->rgba || !hist || image->pixel_count == 0) {
    return false;
  }

  if (opts) {
    bits_rgb = clamp_reduced_bits(opts->lossy_reduced_bits_rgb);
    bits_alpha = clamp_reduced_bits(opts->lossy_reduced_alpha_bits);
  }

  quantize_rgb = bits_rgb < 8;
  quantize_alpha = bits_alpha < 8;

  if (protected_count > 256) {
    protected_count = 256;
  }

  for (i = 0; i < protected_count; ++i) {
    r = protected_colors[i].r;
    g = protected_colors[i].g;
    b = protected_colors[i].b;
    a = protected_colors[i].a;
    if (quantize_rgb) {
      r = quantize_bits(r, bits_rgb);
      g = quantize_bits(g, bits_rgb);
      b = quantize_bits(b, bits_rgb);
    }
    if (quantize_alpha) {
      a = quantize_bits(a, bits_alpha);
    }
    protected_table[i] = pack_rgba_u32(r, g, b, a);
  }

  pixel_count = image->pixel_count;

  samples = (histogram_sample_t *)malloc(pixel_count * sizeof(histogram_sample_t));
  if (!samples) {
    return false;
  }

  for (i = 0; i < pixel_count; ++i) {
    base = i * 4;
    r = image->rgba[base + 0];
    g = image->rgba[base + 1];
    b = image->rgba[base + 2];
    a = image->rgba[base + 3];
    sample_bits_rgb = bits_rgb;
    sample_bits_alpha = bits_alpha;

    if (support && support->bit_hint_map && i < support->bit_hint_len) {
      hint = support->bit_hint_map[i];
      hint_rgb = hint >> 4;
      hint_alpha = hint & 0x0fu;

      if (hint_rgb >= COLOPRESSO_PNGX_REDUCED_BITS_MIN) {
        sample_bits_rgb = clamp_reduced_bits(hint_rgb);
      }

      if (hint_alpha >= COLOPRESSO_PNGX_REDUCED_BITS_MIN) {
        sample_bits_alpha = clamp_reduced_bits(hint_alpha);
      }
    }

    if (quantize_rgb) {
      r = quantize_bits(r, bits_rgb);
      g = quantize_bits(g, bits_rgb);
      b = quantize_bits(b, bits_rgb);
    }

    if (quantize_alpha) {
      a = quantize_bits(a, bits_alpha);
    }

    samples[i].color = pack_rgba_u32(r, g, b, a);
    samples[i].weight = histogram_importance_weight(support, i);
    samples[i].rgb_bits = sample_bits_rgb;
    samples[i].alpha_bits = sample_bits_alpha;
  }

  qsort(samples, pixel_count, sizeof(histogram_sample_t), compare_histogram_sample);

  unique_count = 0;

  for (i = 0; i < pixel_count;) {
    color = samples[i].color;
    run = 1;

    while (i + run < pixel_count && samples[i + run].color == color) {
      ++run;
    }

    ++unique_count;
    i += run;
  }

  hist->entries = (color_entry_t *)malloc(unique_count * sizeof(color_entry_t));
  if (!hist->entries) {
    free(samples);
    return false;
  }
  hist->count = unique_count;
  hist->unlocked_count = 0;

  for (i = 0, unique_count = 0; i < pixel_count;) {
    color = samples[i].color;
    run = 1;
    weight_sum = samples[i].weight;
    max_bits_rgb = samples[i].rgb_bits;
    max_bits_alpha = samples[i].alpha_bits;

    while (i + run < pixel_count && samples[i + run].color == color) {
      weight_sum += samples[i + run].weight;

      if (samples[i + run].rgb_bits > max_bits_rgb) {
        max_bits_rgb = samples[i + run].rgb_bits;
      }

      if (samples[i + run].alpha_bits > max_bits_alpha) {
        max_bits_alpha = samples[i + run].alpha_bits;
      }

      ++run;
    }

    hist->entries[unique_count].color = color;
    hist->entries[unique_count].count = (uint32_t)((weight_sum > UINT32_MAX) ? UINT32_MAX : weight_sum);
    hist->entries[unique_count].mapped_color = color;
    hist->entries[unique_count].locked = is_protected_color(color, protected_table, protected_count);
    hist->entries[unique_count].detail_bits_rgb = clamp_reduced_bits(max_bits_rgb);
    hist->entries[unique_count].detail_bits_alpha = clamp_reduced_bits(max_bits_alpha);
    ++unique_count;
    i += run;
  }

  free(samples);

  if (unique_count == 0) {
    return true;
  }

  write = 0;

  for (i = 0; i < unique_count; ++i) {
    if (!hist->entries[i].locked) {
      if (write != i) {
        tmp = hist->entries[write];
        hist->entries[write] = hist->entries[i];
        hist->entries[i] = tmp;
      }
      ++write;
    }
  }
  hist->unlocked_count = write;

  return true;
}

static inline bool apply_reduced_rgba32_quantization(uint32_t thread_count, color_histogram_t *hist, pngx_rgba_image_t *image, uint32_t target_colors, uint8_t bits_rgb, uint8_t bits_alpha,
                                                     uint32_t *applied_colors) {
  color_box_t *boxes = NULL, new_box;
  color_map_entry_t *map = NULL;
  uint32_t *palette_seed = NULL, actual_colors = 0, mapped, split_index;
  uint8_t *palette_bits_rgb = NULL, *palette_bits_alpha = NULL, r, g, b, a;
  size_t box_count = 0, map_count, i, idx;
  color_remap_parallel_ctx_t remap_ctx;

  if (!hist || !image || target_colors == 0) {
    if (applied_colors) {
      *applied_colors = (uint32_t)(hist ? hist->count : 0);
    }

    return true;
  }

  if (hist->unlocked_count == 0) {
    if (applied_colors) {
      *applied_colors = (uint32_t)hist->count;
    }

    return true;
  }

  if (target_colors > (uint32_t)hist->unlocked_count) {
    target_colors = (uint32_t)hist->unlocked_count;
  }

  if (target_colors == 0) {
    target_colors = 1;
  }

  boxes = (color_box_t *)calloc(target_colors, sizeof(color_box_t));
  if (!boxes) {
    return false;
  }

  boxes[0].start = 0;
  boxes[0].end = hist->unlocked_count;
  color_box_refresh(hist->entries, &boxes[0]);
  box_count = (boxes[0].end > boxes[0].start) ? 1 : 0;

  while (box_count < target_colors) {
    split_index = select_box_to_split(boxes, box_count, hist->entries, bits_rgb, bits_alpha);
    if (split_index < 0) {
      break;
    }

    if (!split_color_box(hist->entries, &boxes[split_index], &new_box)) {
      break;
    }

    boxes[box_count++] = new_box;
  }

  if (box_count == 0) {
    free(boxes);

    if (applied_colors) {
      *applied_colors = (uint32_t)hist->count;
    }

    return true;
  }

  palette_seed = (uint32_t *)malloc(sizeof(uint32_t) * box_count);
  palette_bits_rgb = (uint8_t *)calloc(box_count, sizeof(uint8_t));
  palette_bits_alpha = (uint8_t *)calloc(box_count, sizeof(uint8_t));
  if (!palette_seed || !palette_bits_rgb || !palette_bits_alpha) {
    free(palette_seed);
    free(palette_bits_rgb);
    free(palette_bits_alpha);
    free(boxes);

    return false;
  }

  for (i = 0; i < box_count; ++i) {
    mapped = box_representative_color(hist->entries, &boxes[i]);
    unpack_rgba_u32(mapped, &r, &g, &b, &a);
    snap_rgba_to_bits(&r, &g, &b, &a, bits_rgb, bits_alpha);
    mapped = pack_rgba_u32(r, g, b, a);
    palette_seed[i] = mapped;
    palette_bits_rgb[i] = bits_rgb;
    palette_bits_alpha[i] = bits_alpha;

    for (idx = boxes[i].start; idx < boxes[i].end; ++idx) {
      hist->entries[idx].mapped_color = mapped;
    }
  }

  if (box_count > 0 && box_count <= 4096) {
    refine_reduced_palette(hist->entries, hist->unlocked_count, palette_seed, palette_bits_rgb, palette_bits_alpha, box_count);
  }

  for (i = hist->unlocked_count; i < hist->count; ++i) {
    hist->entries[i].mapped_color = hist->entries[i].color;
  }

  map_count = hist->count;
  if (map_count == 0) {
    free(palette_seed);
    free(palette_bits_rgb);
    free(palette_bits_alpha);
    free(boxes);

    if (applied_colors) {
      *applied_colors = 0;
    }

    return true;
  }

  map = (color_map_entry_t *)malloc(map_count * sizeof(color_map_entry_t));
  if (!map) {
    free(palette_seed);
    free(palette_bits_rgb);
    free(palette_bits_alpha);
    free(boxes);
    return false;
  }

  for (i = 0; i < map_count; ++i) {
    map[i].color = hist->entries[i].color;
    map[i].mapped_color = hist->entries[i].mapped_color;
  }

  qsort(map, map_count, sizeof(*map), compare_color_map_by_color);

  remap_ctx.rgba = image->rgba;
  remap_ctx.pixel_count = image->pixel_count;
  remap_ctx.map = map;
  remap_ctx.map_count = map_count;

#if COLOPRESSO_ENABLE_THREADS
  colopresso_parallel_for(thread_count, (uint32_t)image->pixel_count, color_remap_parallel_worker, &remap_ctx);
#else
  color_remap_parallel_worker(&remap_ctx, 0, (uint32_t)image->pixel_count);
#endif

  qsort(map, map_count, sizeof(*map), compare_color_map_by_mapped);
  for (i = 0; i < map_count; ++i) {
    if (i == 0 || map[i].mapped_color != map[i - 1].mapped_color) {
      ++actual_colors;
    }
  }

  if (applied_colors) {
    *applied_colors = actual_colors;
  }

  free(map);
  free(palette_seed);
  free(palette_bits_rgb);
  free(palette_bits_alpha);
  free(boxes);

  return true;
}

static void pack_rgba_parallel_worker(void *context, uint32_t start, uint32_t end) {
  pack_rgba_parallel_ctx_t *ctx = (pack_rgba_parallel_ctx_t *)context;
  size_t i, base;

  if (!ctx || !ctx->rgba || !ctx->packed) {
    return;
  }

  for (i = (size_t)start; i < (size_t)end && i < ctx->pixel_count; ++i) {
    base = i * 4;
    ctx->packed[i] = pack_rgba_u32(ctx->rgba[base + 0], ctx->rgba[base + 1], ctx->rgba[base + 2], ctx->rgba[base + 3]);
  }
}

static inline bool pack_sorted_rgba(const uint8_t *rgba, size_t pixel_count, uint32_t **out_sorted) {
  uint32_t *packed;
  pack_rgba_parallel_ctx_t ctx;

  if (!rgba || pixel_count == 0 || !out_sorted) {
    return false;
  }

  if (pixel_count > SIZE_MAX / sizeof(uint32_t)) {
    return false;
  }

  packed = (uint32_t *)malloc(sizeof(uint32_t) * pixel_count);
  if (!packed) {
    return false;
  }

  ctx.rgba = rgba;
  ctx.packed = packed;
  ctx.pixel_count = pixel_count;

#if COLOPRESSO_ENABLE_THREADS
  colopresso_parallel_for(0, (uint32_t)pixel_count, pack_rgba_parallel_worker, &ctx);
#else
  pack_rgba_parallel_worker(&ctx, 0, (uint32_t)pixel_count);
#endif

  qsort(packed, pixel_count, sizeof(uint32_t), compare_u32);
  *out_sorted = packed;

  return true;
}

static inline size_t count_unique_rgba(const uint8_t *rgba, size_t pixel_count) {
  uint32_t *packed;
  size_t i, unique = 0;

  if (!rgba || pixel_count == 0) {
    return 0;
  }

  if (!pack_sorted_rgba(rgba, pixel_count, &packed)) {
    return 0;
  }

  for (i = 0; i < pixel_count; ++i) {
    if (i == 0 || packed[i] != packed[i - 1]) {
      ++unique;
    }
  }

  free(packed);

  return unique;
}

static inline bool build_color_frequency(const uint8_t *rgba, size_t pixel_count, color_freq_t **out_freq, size_t *out_count) {
  color_freq_t *freq, *shrunk;
  uint32_t *packed;
  size_t i, unique;

  if (!out_freq || !out_count) {
    return false;
  }

  if (!pack_sorted_rgba(rgba, pixel_count, &packed)) {
    return false;
  }

  freq = (color_freq_t *)malloc(sizeof(color_freq_t) * pixel_count);
  if (!freq) {
    free(packed);
    return false;
  }

  unique = 0;
  for (i = 0; i < pixel_count; ++i) {
    if (i == 0 || packed[i] != packed[i - 1]) {
      freq[unique].color = packed[i];
      freq[unique].count = 1;
      ++unique;
    } else {
      ++freq[unique - 1].count;
    }
  }

  free(packed);

  if (unique == 0) {
    free(freq);
    *out_freq = NULL;
    *out_count = 0;
    return true;
  }

  shrunk = (color_freq_t *)realloc(freq, sizeof(color_freq_t) * unique);
  if (!shrunk) {
    free(freq);
    return false;
  }
  freq = shrunk;

  *out_freq = freq;
  *out_count = unique;

  return true;
}

static inline bool find_freq_index(const color_freq_t *freq, size_t freq_count, uint32_t color, size_t *index_out) {
  size_t left = 0, right = freq_count, mid;

  if (!freq || freq_count == 0) {
    return false;
  }

  while (left < right) {
    mid = left + (right - left) / 2;
    if (freq[mid].color == color) {
      if (index_out) {
        *index_out = mid;
      }

      return true;
    }

    if (freq[mid].color < color) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  return false;
}

static inline int compare_freq_rank_desc(const void *lhs, const void *rhs) {
  const freq_rank_t *a = (const freq_rank_t *)lhs, *b = (const freq_rank_t *)rhs;

  return a->count != b->count ? (b->count - a->count) : (a->color != b->color ? (a->color - b->color) : 0);
}

static inline bool enforce_manual_reduced_limit(uint32_t thread_count, pngx_rgba_image_t *image, uint32_t manual_limit, uint8_t bits_rgb, uint8_t bits_alpha, uint32_t *applied_colors) {
  color_freq_t *freq = NULL;
  freq_rank_t *rank = NULL;
  uint32_t *mapped = NULL, source, best_color, best_dist, candidate_color, distance, original;
  size_t *keep_indices = NULL, freq_count = 0, keep_count, current_unique, unique_after, pixel_index, idx, best_keep, keep_idx, candidate_index, base, freq_index, i;
  bool success = false;

  if (!image || !image->rgba || image->pixel_count == 0 || manual_limit == 0) {
    if (applied_colors && image && image->rgba) {
      current_unique = count_unique_rgba(image->rgba, image->pixel_count);

      if (current_unique > UINT32_MAX) {
        current_unique = UINT32_MAX;
      }

      *applied_colors = (uint32_t)current_unique;
    }

    return true;
  }

  snap_rgba_image_to_bits(thread_count, image->rgba, image->pixel_count, bits_rgb, bits_alpha);

  if (!build_color_frequency(image->rgba, image->pixel_count, &freq, &freq_count)) {
    goto bailout;
  }

  if (freq_count == 0) {
    if (applied_colors) {
      *applied_colors = 0;
    }

    success = true;

    goto bailout;
  }

  if (freq_count <= (size_t)manual_limit) {
    if (applied_colors) {
      *applied_colors = (uint32_t)freq_count;
    }

    success = true;

    goto bailout;
  }

  keep_count = (size_t)manual_limit;
  if (keep_count > freq_count) {
    keep_count = freq_count;
  }

  rank = (freq_rank_t *)malloc(sizeof(freq_rank_t) * freq_count);
  mapped = (uint32_t *)malloc(sizeof(uint32_t) * freq_count);
  keep_indices = (size_t *)malloc(sizeof(size_t) * keep_count);
  if (!rank || !mapped || !keep_indices) {
    goto bailout;
  }

  for (i = 0; i < freq_count; ++i) {
    rank[i].index = i;
    rank[i].count = freq[i].count;
    rank[i].color = freq[i].color;
    mapped[i] = freq[i].color;
  }

  qsort(rank, freq_count, sizeof(freq_rank_t), compare_freq_rank_desc);

  for (i = 0; i < keep_count; ++i) {
    keep_indices[i] = rank[i].index;
  }

  for (i = keep_count; i < freq_count; ++i) {
    idx = rank[i].index;
    source = freq[idx].color;
    best_keep = keep_indices[0];
    best_color = freq[best_keep].color;
    best_dist = simd_color_distance_sq_u32(source, best_color);

    for (keep_idx = 1; keep_idx < keep_count; ++keep_idx) {
      candidate_index = keep_indices[keep_idx];
      candidate_color = freq[candidate_index].color;
      distance = simd_color_distance_sq_u32(source, candidate_color);

      if (distance < best_dist) {
        best_dist = distance;
        best_color = candidate_color;
      }
    }

    mapped[idx] = best_color;
  }

  for (pixel_index = 0; pixel_index < image->pixel_count; ++pixel_index) {
    base = pixel_index * 4;
    original = pack_rgba_u32(image->rgba[base + 0], image->rgba[base + 1], image->rgba[base + 2], image->rgba[base + 3]);

    if (!find_freq_index(freq, freq_count, original, &freq_index)) {
      continue;
    }

    if (mapped[freq_index] == original) {
      continue;
    }

    unpack_rgba_u32(mapped[freq_index], &image->rgba[base + 0], &image->rgba[base + 1], &image->rgba[base + 2], &image->rgba[base + 3]);
  }

  snap_rgba_image_to_bits(thread_count, image->rgba, image->pixel_count, bits_rgb, bits_alpha);

  unique_after = count_unique_rgba(image->rgba, image->pixel_count);
  if (applied_colors) {
    if (unique_after > UINT32_MAX) {
      unique_after = UINT32_MAX;
    }
    *applied_colors = (uint32_t)unique_after;
  }

  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Reduced RGBA32 manual target enforcement trimmed %zu -> %zu colors", freq_count, unique_after);

  success = true;

bailout:
  free(freq);
  free(rank);
  free(mapped);
  free(keep_indices);

  return success;
}

static inline uint8_t resolve_pixel_bits(uint8_t importance, uint8_t base_bits, uint8_t boost_bits) {
  uint8_t resolved = base_bits, mid_bits;

  if (boost_bits <= base_bits) {
    return base_bits;
  }

  if (importance >= PNGX_REDUCED_IMPORTANCE_LEVEL_FULL) {
    resolved = boost_bits;
  } else if (importance >= PNGX_REDUCED_IMPORTANCE_LEVEL_HIGH) {
    mid_bits = (uint8_t)((base_bits + boost_bits + 1) / 2);
    resolved = mid_bits;
  } else if (importance >= PNGX_REDUCED_IMPORTANCE_LEVEL_MEDIUM) {
    mid_bits = (uint8_t)((base_bits * 2 + boost_bits + 2) / 3);
    resolved = mid_bits;
  } else if (importance >= PNGX_REDUCED_IMPORTANCE_LEVEL_LOW) {
    resolved = (uint8_t)((base_bits * 3 + boost_bits + 3) / 4);
  }

  if (resolved > boost_bits) {
    resolved = boost_bits;
  }
  if (resolved < base_bits) {
    resolved = base_bits;
  }

  return resolved;
}

static void reduce_bitdepth_parallel_worker(void *context, uint32_t start, uint32_t end) {
  reduce_bitdepth_parallel_ctx_t *ctx = (reduce_bitdepth_parallel_ctx_t *)context;
  uint8_t importance, pixel_bits_rgb, pixel_bits_alpha;
  size_t i, base;

  if (!ctx || !ctx->rgba) {
    return;
  }

  for (i = (size_t)start; i < (size_t)end && i < ctx->pixel_count; ++i) {
    base = i * PNGX_RGBA_CHANNELS;
    importance = (ctx->importance_map && i < ctx->importance_map_len) ? ctx->importance_map[i] : 0;
    pixel_bits_rgb = resolve_pixel_bits(importance, ctx->bits_rgb, ctx->boost_bits_rgb);
    pixel_bits_alpha = resolve_pixel_bits(importance, ctx->bits_alpha, ctx->boost_bits_alpha);

    if (ctx->bit_hint_map && i < ctx->bit_hint_len) {
      ctx->bit_hint_map[i] = (uint8_t)((pixel_bits_rgb << 4) | (pixel_bits_alpha & 0x0fu));
    }

    if (ctx->rgba[base + 3] > PNGX_REDUCED_ALPHA_NEAR_TRANSPARENT) {
      if (pixel_bits_rgb < PNGX_FULL_CHANNEL_BITS) {
        ctx->rgba[base + 0] = quantize_bits(ctx->rgba[base + 0], pixel_bits_rgb);
        ctx->rgba[base + 1] = quantize_bits(ctx->rgba[base + 1], pixel_bits_rgb);
        ctx->rgba[base + 2] = quantize_bits(ctx->rgba[base + 2], pixel_bits_rgb);
      }
    }
    if (pixel_bits_alpha < PNGX_FULL_CHANNEL_BITS) {
      ctx->rgba[base + 3] = quantize_bits(ctx->rgba[base + 3], pixel_bits_alpha);
    }
  }
}

static inline void reduce_rgba_custom_bitdepth_simple(uint32_t thread_count, uint8_t *rgba, size_t pixel_count, uint8_t bits_rgb, uint8_t bits_alpha, const uint8_t *importance_map,
                                                      size_t importance_map_len, uint8_t boost_bits_rgb, uint8_t boost_bits_alpha, uint8_t *bit_hint_map, size_t bit_hint_len) {
  reduce_bitdepth_parallel_ctx_t ctx;

  if (!rgba || pixel_count == 0) {
    return;
  }

  ctx.rgba = rgba;
  ctx.pixel_count = pixel_count;
  ctx.importance_map = importance_map;
  ctx.importance_map_len = importance_map_len;
  ctx.bits_rgb = clamp_reduced_bits(bits_rgb);
  ctx.bits_alpha = clamp_reduced_bits(bits_alpha);
  ctx.boost_bits_rgb = clamp_reduced_bits(boost_bits_rgb);
  ctx.boost_bits_alpha = clamp_reduced_bits(boost_bits_alpha);
  ctx.bit_hint_map = bit_hint_map;
  ctx.bit_hint_len = bit_hint_len;

#if COLOPRESSO_ENABLE_THREADS
  colopresso_parallel_for(thread_count, (uint32_t)pixel_count, reduce_bitdepth_parallel_worker, &ctx);
#else
  reduce_bitdepth_parallel_worker(&ctx, 0, (uint32_t)pixel_count);
#endif
}

static inline void process_custom_bitdepth_pixel(uint8_t *rgba, png_uint_32 width, png_uint_32 height, uint32_t x, uint32_t y, uint8_t base_bits_rgb, uint8_t base_bits_alpha, uint8_t boost_bits_rgb,
                                                 uint8_t boost_bits_alpha, float base_dither, const uint8_t *importance_map, size_t pixel_count, float *err_curr, float *err_next, bool left_to_right,
                                                 uint8_t *bit_hint_map, size_t bit_hint_len) {
  uint8_t importance = 0, pixel_bits_rgb, pixel_bits_alpha, channel, bits, quantized;
  size_t pixel_index = (size_t)y * (size_t)width + (size_t)x, rgba_index = pixel_index * PNGX_RGBA_CHANNELS, err_index = (size_t)x * PNGX_RGBA_CHANNELS;
  float dither = base_dither, value, error, alpha_factor, dither_ch;

  if (importance_map && pixel_index < pixel_count) {
    importance = importance_map[pixel_index];
    dither *= importance_dither_scale(importance);
  }
  alpha_factor = (float)rgba[rgba_index + 3] / 255.0f;

  pixel_bits_rgb = resolve_pixel_bits(importance, base_bits_rgb, boost_bits_rgb);
  pixel_bits_alpha = resolve_pixel_bits(importance, base_bits_alpha, boost_bits_alpha);
  if (bit_hint_map && pixel_index < bit_hint_len) {
    bit_hint_map[pixel_index] = (uint8_t)((pixel_bits_rgb << 4) | (pixel_bits_alpha & 0x0fu));
  }

  for (channel = 0; channel < PNGX_RGBA_CHANNELS; ++channel) {
    if (channel != 3 && rgba[rgba_index + 3] <= PNGX_REDUCED_ALPHA_NEAR_TRANSPARENT) {
      bits = PNGX_FULL_CHANNEL_BITS;
    } else {
      bits = (channel == 3) ? pixel_bits_alpha : pixel_bits_rgb;
    }

    if (bits >= PNGX_FULL_CHANNEL_BITS) {
      err_curr[err_index + channel] = 0.0f;
      continue;
    }

    value = (float)rgba[rgba_index + channel] + err_curr[err_index + channel];
    quantized = quantize_channel_value(value, bits);
    error = (value - (float)quantized);

    if (channel == 3) {
      dither_ch = 0.0f;
    } else {
      dither_ch = dither * alpha_factor;
      if (alpha_factor < PNGX_REDUCED_ALPHA_MIN_DITHER_FACTOR) {
        dither_ch = 0.0f;
      }
    }

    error *= dither_ch;

    rgba[rgba_index + channel] = quantized;
    err_curr[err_index + channel] = 0.0f;

    if (dither_ch <= 0.0f || error == 0.0f) {
      continue;
    }

    if (left_to_right) {
      if (x + 1 < width) {
        err_curr[err_index + PNGX_RGBA_CHANNELS + channel] += error * (7.0f / 16.0f);
      }
      if (y + 1 < height) {
        if (x > 0) {
          err_next[err_index - PNGX_RGBA_CHANNELS + channel] += error * (3.0f / 16.0f);
        }
        err_next[err_index + channel] += error * (5.0f / 16.0f);
        if (x + 1 < width) {
          err_next[err_index + PNGX_RGBA_CHANNELS + channel] += error * (1.0f / 16.0f);
        }
      }
    } else {
      if (x > 0) {
        err_curr[err_index - PNGX_RGBA_CHANNELS + channel] += error * (7.0f / 16.0f);
      }
      if (y + 1 < height) {
        if (x + 1 < width) {
          err_next[err_index + PNGX_RGBA_CHANNELS + channel] += error * (3.0f / 16.0f);
        }
        err_next[err_index + channel] += error * (5.0f / 16.0f);
        if (x > 0) {
          err_next[err_index - PNGX_RGBA_CHANNELS + channel] += error * (1.0f / 16.0f);
        }
      }
    }
  }
}

static inline bool reduce_rgba_custom_bitdepth_dither(uint8_t *rgba, png_uint_32 width, png_uint_32 height, uint8_t bits_rgb, uint8_t bits_alpha, uint8_t boost_bits_rgb, uint8_t boost_bits_alpha,
                                                      float dither_level, const uint8_t *importance_map, size_t pixel_count, uint8_t *bit_hint_map, size_t bit_hint_len) {
  uint32_t x, y;
  size_t row_stride;
  float *err_curr, *err_next, *tmp;
  bool left_to_right;

  bits_rgb = clamp_reduced_bits(bits_rgb);
  bits_alpha = clamp_reduced_bits(bits_alpha);
  boost_bits_rgb = clamp_reduced_bits(boost_bits_rgb);
  boost_bits_alpha = clamp_reduced_bits(boost_bits_alpha);
  if ((!rgba) || width == 0 || height == 0 || (bits_rgb >= PNGX_FULL_CHANNEL_BITS && bits_alpha >= PNGX_FULL_CHANNEL_BITS)) {
    return true;
  }

  row_stride = (size_t)width * PNGX_RGBA_CHANNELS;
  err_curr = (float *)calloc(row_stride, sizeof(float));
  err_next = (float *)calloc(row_stride, sizeof(float));
  if (!err_curr || !err_next) {
    free(err_curr);
    free(err_next);
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "PNGX: Reduced RGBA32 dither allocation failed");

    return false;
  }

  for (y = 0; y < height; ++y) {
    left_to_right = ((y & 1) == 0);
    memset(err_next, 0, row_stride * sizeof(float));
    if (left_to_right) {
      for (x = 0; x < width; ++x) {
        process_custom_bitdepth_pixel(rgba, width, height, x, y, bits_rgb, bits_alpha, boost_bits_rgb, boost_bits_alpha, dither_level, importance_map, pixel_count, err_curr, err_next, true,
                                      bit_hint_map, bit_hint_len);
      }
    } else {
      x = width;
      while (x-- > 0) {
        process_custom_bitdepth_pixel(rgba, width, height, x, y, bits_rgb, bits_alpha, boost_bits_rgb, boost_bits_alpha, dither_level, importance_map, pixel_count, err_curr, err_next, false,
                                      bit_hint_map, bit_hint_len);
      }
    }

    tmp = err_curr;
    err_curr = err_next;
    err_next = tmp;
  }

  free(err_curr);
  free(err_next);

  return true;
}

static inline bool reduce_rgba_custom_bitdepth(uint32_t thread_count, uint8_t *rgba, png_uint_32 width, png_uint_32 height, uint8_t bits_rgb, uint8_t bits_alpha, float dither_level,
                                               const uint8_t *importance_map, size_t importance_map_len, pngx_quant_support_t *support) {
  uint8_t boost_bits_rgb, boost_bits_alpha, *bit_hint_map = NULL;
  size_t bit_hint_len = 0, pixel_count, hint_len;
  bool need_rgb, need_alpha;

  if (!rgba || width == 0 || height == 0) {
    return true;
  }

  bits_rgb = clamp_reduced_bits(bits_rgb);
  bits_alpha = clamp_reduced_bits(bits_alpha);
  need_rgb = bits_rgb < PNGX_FULL_CHANNEL_BITS;
  need_alpha = bits_alpha < PNGX_FULL_CHANNEL_BITS;
  if (!need_rgb && !need_alpha) {
    return true;
  }

  pixel_count = (size_t)width * (size_t)height;
  if (!importance_map || importance_map_len < pixel_count) {
    importance_map = NULL;
  }

  boost_bits_rgb = (importance_map && bits_rgb < PNGX_FULL_CHANNEL_BITS) ? clamp_reduced_bits((uint8_t)(bits_rgb + 3)) : bits_rgb;
  boost_bits_alpha = (importance_map && bits_alpha < PNGX_FULL_CHANNEL_BITS) ? clamp_reduced_bits((uint8_t)(bits_alpha + 2)) : bits_alpha;

  if (support) {
    free(support->bit_hint_map);
    support->bit_hint_map = NULL;
    support->bit_hint_len = 0;
    if (pixel_count > 0) {
      support->bit_hint_map = (uint8_t *)malloc(pixel_count);
      if (support->bit_hint_map) {
        support->bit_hint_len = pixel_count;
        bit_hint_map = support->bit_hint_map;
        bit_hint_len = support->bit_hint_len;
      }
    }
  }

  if (dither_level > 0.0f) {
    if (!reduce_rgba_custom_bitdepth_dither(rgba, width, height, bits_rgb, bits_alpha, boost_bits_rgb, boost_bits_alpha, dither_level, importance_map, pixel_count, bit_hint_map, bit_hint_len)) {
      return false;
    }
  } else {
    hint_len = importance_map ? pixel_count : 0;
    reduce_rgba_custom_bitdepth_simple(thread_count, rgba, pixel_count, bits_rgb, bits_alpha, importance_map, hint_len, boost_bits_rgb, boost_bits_alpha, bit_hint_map, bit_hint_len);
  }

  return true;
}

static inline uint32_t collect_alpha_levels(const pngx_rgba_image_t *image, float *non_opaque_ratio) {
  uint32_t unique = 0, non_opaque = 0;
  uint8_t alpha;
  size_t i;
  bool level_used[256] = {false};

  if (!image || !image->rgba || image->pixel_count == 0) {
    if (non_opaque_ratio) {
      *non_opaque_ratio = 0.0f;
    }

    return 0;
  }

  for (i = 0; i < image->pixel_count; ++i) {
    alpha = image->rgba[i * 4 + 3];
    if (!level_used[alpha]) {
      level_used[alpha] = true;
      ++unique;
    }

    if (alpha < 255) {
      ++non_opaque;
    }
  }

  if (non_opaque_ratio) {
    *non_opaque_ratio = (image->pixel_count > 0) ? ((float)non_opaque / (float)image->pixel_count) : 0.0f;
  }

  return unique;
}

static inline uint8_t bits_for_level_count(uint32_t levels) {
  uint32_t capacity = 1;
  uint8_t bits = 1;

  if (levels == 0) {
    return 1;
  }

  while (capacity < levels && bits < COLOPRESSO_PNGX_REDUCED_BITS_MAX) {
    ++bits;
    capacity <<= 1;
  }

  return clamp_reduced_bits(bits);
}

static inline void tune_reduced_bitdepth(const pngx_rgba_image_t *image, const pngx_image_stats_t *stats, uint8_t *bits_rgb, uint8_t *bits_alpha) {
  uint32_t alpha_levels;
  uint8_t tuned_rgb, tuned_alpha, level_bits, next_alpha;
  float gradient, saturation, vibrant, opaque, translucent, non_opaque_ratio;

  if (!bits_rgb || !bits_alpha) {
    return;
  }

  tuned_rgb = clamp_reduced_bits(*bits_rgb);
  tuned_alpha = clamp_reduced_bits(*bits_alpha);
  gradient = stats ? stats->gradient_mean : PNGX_REDUCED_TUNE_DEFAULT_GRADIENT;
  saturation = stats ? stats->saturation_mean : PNGX_REDUCED_TUNE_DEFAULT_SATURATION;
  vibrant = stats ? stats->vibrant_ratio : PNGX_REDUCED_TUNE_DEFAULT_VIBRANT;
  opaque = stats ? stats->opaque_ratio : PNGX_REDUCED_TUNE_DEFAULT_OPAQUE;
  translucent = stats ? stats->translucent_ratio : PNGX_REDUCED_TUNE_DEFAULT_TRANSLUCENT;

  if (gradient < PNGX_REDUCED_TUNE_FLAT_GRADIENT_THRESHOLD && saturation < PNGX_REDUCED_TUNE_FLAT_SATURATION_THRESHOLD && vibrant < PNGX_REDUCED_TUNE_FLAT_VIBRANT_THRESHOLD && tuned_rgb > 3) {
    --tuned_rgb;
    if (gradient < PNGX_REDUCED_TUNE_VERY_FLAT_GRADIENT && saturation < PNGX_REDUCED_TUNE_VERY_FLAT_SATURATION && tuned_rgb > 3) {
      --tuned_rgb;
    }
    if (tuned_rgb < 3) {
      tuned_rgb = 3;
    }
  }

  non_opaque_ratio = 0.0f;
  alpha_levels = collect_alpha_levels(image, &non_opaque_ratio);
  if (alpha_levels > 0) {
    level_bits = bits_for_level_count(alpha_levels);
    if (alpha_levels <= PNGX_REDUCED_ALPHA_LEVEL_LIMIT_FEW && non_opaque_ratio < PNGX_REDUCED_ALPHA_RATIO_FEW && tuned_alpha > level_bits) {
      tuned_alpha = (level_bits < 2) ? 2 : level_bits;
    } else if (alpha_levels <= PNGX_REDUCED_ALPHA_NEAR_TRANSPARENT && non_opaque_ratio < PNGX_REDUCED_ALPHA_RATIO_LOW && tuned_alpha > (uint8_t)(level_bits + 1)) {
      tuned_alpha = (uint8_t)(level_bits + 1);
    } else if (alpha_levels <= 16 && non_opaque_ratio < PNGX_REDUCED_ALPHA_RATIO_MINIMAL && tuned_alpha > (uint8_t)(level_bits + 2)) {
      tuned_alpha = (uint8_t)(level_bits + 2);
    }

    if (opaque > PNGX_REDUCED_ALPHA_OPAQUE_LIMIT && level_bits <= 2 && tuned_alpha > 2) {
      tuned_alpha = 2;
    } else if (translucent < PNGX_REDUCED_ALPHA_TRANSLUCENT_LIMIT && tuned_alpha > level_bits) {
      next_alpha = (uint8_t)(tuned_alpha - 1);
      if (next_alpha < level_bits) {
        next_alpha = level_bits;
      }
      tuned_alpha = next_alpha;
    }
  }

  if (tuned_alpha < COLOPRESSO_PNGX_REDUCED_BITS_MIN) {
    tuned_alpha = COLOPRESSO_PNGX_REDUCED_BITS_MIN;
  }

  *bits_rgb = tuned_rgb;
  *bits_alpha = tuned_alpha;
}

static inline bool apply_reduced_rgba32_prepass(pngx_rgba_image_t *image, const pngx_options_t *opts, pngx_quant_support_t *support, pngx_image_stats_t *stats) {
  const uint8_t *importance = NULL;
  uint8_t bits_rgb = COLOPRESSO_PNGX_DEFAULT_REDUCED_BITS_RGB, bits_alpha = COLOPRESSO_PNGX_DEFAULT_REDUCED_ALPHA_BITS;
  size_t importance_len = 0;
  float dither = 0.0f;

  if (!image || !image->rgba || image->pixel_count == 0) {
    return true;
  }

  if (opts) {
    bits_rgb = clamp_reduced_bits(opts->lossy_reduced_bits_rgb);
    bits_alpha = clamp_reduced_bits(opts->lossy_reduced_alpha_bits);
    if (opts->lossy_dither_auto) {
      dither = resolve_quant_dither(opts, stats);
    } else {
      dither = clamp_float(opts->lossy_dither_level, 0.0f, 1.0f);
    }
    if (support && support->importance_map && support->importance_map_len >= image->pixel_count) {
      importance = support->importance_map;
      importance_len = support->importance_map_len;
    }
  }

  return reduce_rgba_custom_bitdepth(opts->thread_count, image->rgba, image->width, image->height, bits_rgb, bits_alpha, dither, importance, importance_len, support);
}

static inline void head_heap_sift_up(uint64_t *heap, size_t index) {
  uint64_t parent_value;
  size_t parent;

  while (index > 0) {
    parent = (index - 1) / 2;
    parent_value = heap[parent];

    if (parent_value <= heap[index]) {
      break;
    }

    heap[parent] = heap[index];
    heap[index] = parent_value;
    index = parent;
  }
}

static inline void head_heap_sift_down(uint64_t *heap, size_t size, size_t index) {
  uint64_t tmp;
  size_t left, right, smallest;

  while (true) {
    left = index * 2 + 1;
    right = left + 1;
    smallest = index;

    if (left < size && heap[left] < heap[smallest]) {
      smallest = left;
    }

    if (right < size && heap[right] < heap[smallest]) {
      smallest = right;
    }

    if (smallest == index) {
      break;
    }

    tmp = heap[index];
    heap[index] = heap[smallest];
    heap[smallest] = tmp;
    index = smallest;
  }
}

static inline float histogram_head_dominance(const color_histogram_t *hist, size_t head_limit) {
  uint64_t heap[PNGX_REDUCED_HEAD_DOMINANCE_LIMIT], weight, total_weight = 0, head_sum = 0;
  size_t heap_size = 0, capacity, i;

  if (!hist || hist->count == 0 || head_limit == 0) {
    return 0.0f;
  }

  capacity = head_limit;
  if (capacity > PNGX_REDUCED_HEAD_DOMINANCE_LIMIT) {
    capacity = PNGX_REDUCED_HEAD_DOMINANCE_LIMIT;
  }

  for (i = 0; i < hist->count; ++i) {
    weight = hist->entries[i].count ? hist->entries[i].count : 1;
    total_weight += weight;

    if (capacity == 0) {
      continue;
    }

    if (heap_size < capacity) {
      heap[heap_size] = weight;
      ++heap_size;
      head_heap_sift_up(heap, heap_size - 1);
    } else if (weight > heap[0]) {
      heap[0] = weight;
      head_heap_sift_down(heap, heap_size, 0);
    }
  }

  if (total_weight == 0 || heap_size == 0) {
    return 0.0f;
  }

  for (i = 0; i < heap_size; ++i) {
    head_sum += heap[i];
  }

  return (float)head_sum / (float)total_weight;
}

static inline float histogram_low_weight_ratio(const color_histogram_t *hist, uint32_t weight_threshold) {
  const color_entry_t *entry;
  uint64_t total_weight = 0, low_weight = 0, weight;
  size_t i;

  if (!hist || hist->count == 0 || weight_threshold == 0) {
    return 0.0f;
  }

  for (i = 0; i < hist->count; ++i) {
    entry = &hist->entries[i];
    weight = entry->count ? entry->count : 1;
    total_weight += weight;
    if (entry->count <= weight_threshold) {
      low_weight += weight;
    }
  }

  if (total_weight == 0) {
    return 0.0f;
  }

  return (float)low_weight / (float)total_weight;
}

static inline float histogram_detail_pressure(const color_histogram_t *hist, uint8_t base_bits_rgb, uint8_t base_bits_alpha) {
  const color_entry_t *entry;
  uint64_t total_weight = 0, detail_weight = 0, weight;
  size_t i;

  if (!hist || hist->count == 0) {
    return 0.0f;
  }

  for (i = 0; i < hist->count; ++i) {
    entry = &hist->entries[i];
    weight = entry->count ? entry->count : 1;
    total_weight += weight;
    if (entry->detail_bits_rgb > base_bits_rgb || entry->detail_bits_alpha > base_bits_alpha) {
      detail_weight += weight;
    }
  }

  if (total_weight == 0) {
    return 0.0f;
  }

  return (float)detail_weight / (float)total_weight;
}

static inline float stats_flatness_factor(const pngx_image_stats_t *stats) {
  float gradient = stats ? stats->gradient_mean : PNGX_REDUCED_STATS_FLAT_DEFAULT_GRADIENT, saturation = stats ? stats->saturation_mean : PNGX_REDUCED_STATS_FLAT_DEFAULT_SATURATION,
        vibrant = stats ? stats->vibrant_ratio : PNGX_REDUCED_STATS_FLAT_DEFAULT_VIBRANT;
  float gradient_term = clamp_float((PNGX_REDUCED_STATS_FLAT_GRADIENT_REF - gradient) / PNGX_REDUCED_STATS_FLAT_GRADIENT_REF, 0.0f, 1.0f),
        saturation_term = clamp_float((PNGX_REDUCED_STATS_FLAT_SATURATION_REF - saturation) / PNGX_REDUCED_STATS_FLAT_SATURATION_REF, 0.0f, 1.0f),
        vibrant_term = clamp_float((PNGX_REDUCED_VIBRANT_RATIO_LOW - vibrant) / PNGX_REDUCED_VIBRANT_RATIO_LOW, 0.0f, 1.0f);

  return (gradient_term * PNGX_REDUCED_STATS_FLAT_GRADIENT_WEIGHT) + (saturation_term * PNGX_REDUCED_STATS_FLAT_SATURATION_WEIGHT) + (vibrant_term * PNGX_REDUCED_STATS_FLAT_VIBRANT_WEIGHT);
}

static inline float stats_alpha_simplicity(const pngx_image_stats_t *stats) {
  float opaque = stats ? stats->opaque_ratio : PNGX_REDUCED_ALPHA_SIMPLE_DEFAULT_OPAQUE, translucent = stats ? stats->translucent_ratio : PNGX_REDUCED_ALPHA_SIMPLE_DEFAULT_TRANSLUCENT;
  float opaque_term = clamp_float((opaque - PNGX_REDUCED_ALPHA_SIMPLE_OPAQUE_REF) / PNGX_REDUCED_ALPHA_SIMPLE_OPAQUE_RANGE, 0.0f, 1.0f);
  float translucent_term = clamp_float((PNGX_REDUCED_ALPHA_SIMPLE_TRANSLUCENT_REF - translucent) / PNGX_REDUCED_ALPHA_SIMPLE_TRANSLUCENT_RANGE, 0.0f, 1.0f);

  return (opaque_term * PNGX_REDUCED_ALPHA_SIMPLE_OPAQUE_WEIGHT) + (translucent_term * PNGX_REDUCED_ALPHA_SIMPLE_TRANSLUCENT_WEIGHT);
}

static inline uint32_t resolve_reduced_passthrough_threshold(uint32_t grid_cap, const pngx_image_stats_t *stats) {
  uint32_t threshold;
  float gradient = stats ? stats->gradient_mean : PNGX_REDUCED_PASSTHROUGH_DEFAULT_GRADIENT, saturation = stats ? stats->saturation_mean : PNGX_REDUCED_PASSTHROUGH_DEFAULT_SATURATION,
        vibrant = stats ? stats->vibrant_ratio : PNGX_REDUCED_PASSTHROUGH_DEFAULT_VIBRANT;
  float ratio =
      PNGX_REDUCED_PASSTHROUGH_RATIO_BASE +
      clamp_float((gradient * PNGX_REDUCED_PASSTHROUGH_GRADIENT_WEIGHT) + (saturation * PNGX_REDUCED_PASSTHROUGH_SATURATION_WEIGHT) + (vibrant * PNGX_REDUCED_PASSTHROUGH_VIBRANT_WEIGHT), 0.0f, 1.0f) *
          PNGX_REDUCED_PASSTHROUGH_RATIO_GAIN;

  if (ratio > PNGX_REDUCED_PASSTHROUGH_RATIO_CAP) {
    ratio = PNGX_REDUCED_PASSTHROUGH_RATIO_CAP;
  }

  threshold = (uint32_t)((float)grid_cap * ratio + PNGX_REDUCED_ROUNDING_OFFSET);
  if (threshold < PNGX_REDUCED_RGBA32_PASSTHROUGH_MIN_COLORS) {
    threshold = PNGX_REDUCED_RGBA32_PASSTHROUGH_MIN_COLORS;
  }
  if (threshold > grid_cap) {
    threshold = grid_cap;
  }

  return threshold;
}

static inline uint32_t compute_auto_trim_limit(const color_histogram_t *hist, size_t pixel_count, uint32_t actual_colors, uint8_t bits_rgb, uint8_t bits_alpha, const pngx_image_stats_t *stats) {
  uint32_t low_weight_threshold, limit;
  double density;
  float low_weight_ratio, detail_pressure, head_dominance, flatness, alpha_simple, density_f, trim = 0.0f, head_term, tail_term;

  if (!hist || hist->count == 0 || pixel_count == 0 || actual_colors <= (uint32_t)(COLOPRESSO_PNGX_REDUCED_COLORS_MIN + PNGX_REDUCED_TRIM_MIN_COLOR_MARGIN)) {
    return 0;
  }

  density = (double)hist->count / (double)pixel_count;
  density_f = (float)density;
  low_weight_threshold = (uint32_t)(pixel_count / PNGX_REDUCED_LOW_WEIGHT_DIVISOR);
  if (low_weight_threshold < PNGX_REDUCED_LOW_WEIGHT_MIN) {
    low_weight_threshold = PNGX_REDUCED_LOW_WEIGHT_MIN;
  } else if (low_weight_threshold > PNGX_REDUCED_LOW_WEIGHT_MAX) {
    low_weight_threshold = PNGX_REDUCED_LOW_WEIGHT_MAX;
  }

  low_weight_ratio = histogram_low_weight_ratio(hist, low_weight_threshold);
  detail_pressure = histogram_detail_pressure(hist, bits_rgb, bits_alpha);
  head_dominance = histogram_head_dominance(hist, PNGX_REDUCED_HEAD_DOMINANCE_LIMIT);
  flatness = stats_flatness_factor(stats);
  alpha_simple = stats_alpha_simplicity(stats);

  if (head_dominance > PNGX_REDUCED_TRIM_HEAD_DOMINANCE_THRESHOLD && detail_pressure < PNGX_REDUCED_TRIM_DETAIL_PRESSURE_HEAD_LIMIT) {
    head_term =
        clamp_float((head_dominance - PNGX_REDUCED_TRIM_HEAD_DOMINANCE_THRESHOLD) * (PNGX_REDUCED_TRIM_HEAD_WEIGHT + flatness * PNGX_REDUCED_TRIM_FLATNESS_WEIGHT), 0.0f, PNGX_REDUCED_TRIM_HEAD_CLAMP);
    trim += head_term;
  }

  if (low_weight_ratio > PNGX_REDUCED_TRIM_TAIL_RATIO_THRESHOLD && detail_pressure < PNGX_REDUCED_TRIM_DETAIL_PRESSURE_TAIL_LIMIT) {
    tail_term = (low_weight_ratio - PNGX_REDUCED_TRIM_TAIL_RATIO_THRESHOLD) * (PNGX_REDUCED_TRIM_TAIL_BASE_WEIGHT + PNGX_REDUCED_TRIM_TAIL_DETAIL_WEIGHT * (1.0f - detail_pressure));
    trim += clamp_float(tail_term, 0.0f, PNGX_REDUCED_TRIM_TAIL_CLAMP);
  }

  if (density_f < PNGX_REDUCED_TRIM_DENSITY_THRESHOLD) {
    trim += clamp_float((PNGX_REDUCED_TRIM_DENSITY_THRESHOLD - density_f) * PNGX_REDUCED_TRIM_DENSITY_SCALE, 0.0f, PNGX_REDUCED_TRIM_DENSITY_CLAMP);
  }

  if (alpha_simple > PNGX_REDUCED_TRIM_ALPHA_SIMPLE_THRESHOLD && detail_pressure < PNGX_REDUCED_TRIM_DETAIL_PRESSURE_HEAD_LIMIT) {
    trim += clamp_float(alpha_simple * PNGX_REDUCED_TRIM_ALPHA_SIMPLE_SCALE, 0.0f, PNGX_REDUCED_TRIM_ALPHA_SIMPLE_CLAMP);
  }

  if (flatness > PNGX_REDUCED_TRIM_FLATNESS_THRESHOLD && detail_pressure < PNGX_REDUCED_TRIM_DETAIL_PRESSURE_FLAT_LIMIT) {
    trim += clamp_float((flatness - PNGX_REDUCED_TRIM_FLATNESS_THRESHOLD) * PNGX_REDUCED_TRIM_FLATNESS_SCALE, 0.0f, PNGX_REDUCED_TRIM_FLATNESS_CLAMP);
  }

  trim = clamp_float(trim, 0.0f, PNGX_REDUCED_TRIM_TOTAL_CLAMP);
  if (trim < PNGX_REDUCED_TRIM_MIN_TRIGGER) {
    return 0;
  }

  limit = (uint32_t)((float)actual_colors * (1.0f - trim) + PNGX_REDUCED_ROUNDING_OFFSET);
  if (limit < (uint32_t)COLOPRESSO_PNGX_REDUCED_COLORS_MIN) {
    limit = (uint32_t)COLOPRESSO_PNGX_REDUCED_COLORS_MIN;
  }

  if (actual_colors <= limit || (actual_colors - limit) < PNGX_REDUCED_TRIM_MIN_COLOR_DIFF) {
    return 0;
  }

  return limit;
}

static inline uint32_t resolve_reduced_rgba32_target(const color_histogram_t *hist, size_t pixel_count, int32_t hint, uint8_t bits_rgb, uint8_t bits_alpha, const pngx_image_stats_t *stats) {
  uint64_t rgb_cap = 0, alpha_cap = 0;
  uint32_t target, grid_cap = COLOPRESSO_PNGX_REDUCED_COLORS_MAX, low_weight_threshold;
  size_t unique_colors = hist ? hist->count : 0, unlocked = hist ? hist->unlocked_count : 0;
  float low_weight_ratio = 0.0f, detail_pressure = 0.0f, reduction_scale, boost_scale, flatness_score = 0.0f, alpha_simple = 0.0f, head_dominance = 0.0f, tail_cut, dominance_cut, gentle_cut,
        applied_cut, tail_gain, dominance_gain, relief, gradient_relief, softness_relief, detail_relief, combined_cut, density_gap, flatness_reduction, alpha_reduction, span_ratio;
  double base, density;

  if (unique_colors == 0) {
    return 0;
  }

  density = (pixel_count > 0) ? ((double)unique_colors / (double)pixel_count) : 0.0;

  bits_rgb = clamp_reduced_bits(bits_rgb);
  bits_alpha = clamp_reduced_bits(bits_alpha);
  if (bits_rgb < 8) {
    rgb_cap = (uint64_t)1 << (bits_rgb * 3);
  }
  if (bits_alpha < 8) {
    alpha_cap = (uint64_t)1 << bits_alpha;
  }
  if (rgb_cap == 0) {
    rgb_cap = COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
  }
  if (alpha_cap == 0) {
    alpha_cap = 1;
  }
  if (rgb_cap > (uint64_t)COLOPRESSO_PNGX_REDUCED_COLORS_MAX) {
    rgb_cap = COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
  }
  if (alpha_cap > (uint64_t)COLOPRESSO_PNGX_REDUCED_COLORS_MAX) {
    alpha_cap = COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
  }
  if (rgb_cap * alpha_cap < (uint64_t)COLOPRESSO_PNGX_REDUCED_COLORS_MAX) {
    grid_cap = (uint32_t)(rgb_cap * alpha_cap);
  }

  if (hint > 0) {
    target = (uint32_t)hint;
    if (target < COLOPRESSO_PNGX_REDUCED_COLORS_MIN) {
      target = COLOPRESSO_PNGX_REDUCED_COLORS_MIN;
    }
    if (target > COLOPRESSO_PNGX_REDUCED_COLORS_MAX) {
      target = COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
    }
  } else {
    target = (uint32_t)unique_colors;
    if (unique_colors > PNGX_REDUCED_TARGET_UNIQUE_COLOR_THRESHOLD) {
      base = sqrt((double)unique_colors) * PNGX_REDUCED_TARGET_UNIQUE_BASE_SCALE;
      if (density < PNGX_REDUCED_TARGET_DENSITY_LOW_THRESHOLD) {
        base *= PNGX_REDUCED_TARGET_DENSITY_LOW_SCALE;
      } else if (density > PNGX_REDUCED_TARGET_DENSITY_HIGH_THRESHOLD) {
        base *= PNGX_REDUCED_TARGET_DENSITY_HIGH_SCALE;
      }

      if (base < PNGX_REDUCED_TARGET_BASE_MIN) {
        base = PNGX_REDUCED_TARGET_BASE_MIN;
      }

      if (base > (double)COLOPRESSO_PNGX_REDUCED_COLORS_MAX) {
        base = (double)COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
      }

      target = (uint32_t)(base + PNGX_REDUCED_ROUNDING_OFFSET);
    }
  }

  if (pixel_count > 0 && hist) {
    low_weight_threshold = (uint32_t)(pixel_count / PNGX_REDUCED_LOW_WEIGHT_DIVISOR);
    if (low_weight_threshold < PNGX_REDUCED_LOW_WEIGHT_MIN) {
      low_weight_threshold = PNGX_REDUCED_LOW_WEIGHT_MIN;
    } else if (low_weight_threshold > PNGX_REDUCED_LOW_WEIGHT_MAX) {
      low_weight_threshold = PNGX_REDUCED_LOW_WEIGHT_MAX;
    }

    low_weight_ratio = histogram_low_weight_ratio(hist, low_weight_threshold);
    detail_pressure = histogram_detail_pressure(hist, bits_rgb, bits_alpha);
    head_dominance = histogram_head_dominance(hist, PNGX_REDUCED_TARGET_HEAD_DOMINANCE_BUCKETS);

    if (low_weight_ratio > PNGX_REDUCED_TARGET_LOW_WEIGHT_REDUCTION_START) {
      reduction_scale = 1.0f - clamp_float((low_weight_ratio - PNGX_REDUCED_TARGET_LOW_WEIGHT_REDUCTION_START) *
                                               (PNGX_REDUCED_TARGET_LOW_WEIGHT_REDUCTION_BASE + (PNGX_REDUCED_TARGET_LOW_WEIGHT_REDUCTION_DETAIL * (1.0f - detail_pressure))),
                                           0.0f, PNGX_REDUCED_TARGET_LOW_WEIGHT_REDUCTION_CLAMP);
      target = (uint32_t)((float)target * reduction_scale + PNGX_REDUCED_ROUNDING_OFFSET);
    }

    if (low_weight_ratio > PNGX_REDUCED_TARGET_TAIL_RATIO_THRESHOLD && detail_pressure < PNGX_REDUCED_TARGET_DETAIL_PRESSURE_TAIL_LIMIT) {
      tail_cut = clamp_float((low_weight_ratio - PNGX_REDUCED_TARGET_TAIL_RATIO_THRESHOLD) * (PNGX_REDUCED_TARGET_TAIL_WIDTH_BASE - detail_pressure) * PNGX_REDUCED_TARGET_TAIL_WIDTH_SCALE, 0.0f,
                             PNGX_REDUCED_TARGET_TAIL_CUT_CLAMP);
      target = (uint32_t)((float)target * (1.0f - tail_cut) + PNGX_REDUCED_ROUNDING_OFFSET);
    }

    if (detail_pressure > PNGX_REDUCED_TARGET_DETAIL_PRESSURE_BOOST) {
      boost_scale = 1.0f + clamp_float((detail_pressure - PNGX_REDUCED_TARGET_DETAIL_PRESSURE_BOOST) * PNGX_REDUCED_TARGET_DETAIL_BOOST_SCALE, 0.0f, PNGX_REDUCED_TARGET_DETAIL_BOOST_CLAMP);
      target = (uint32_t)((float)target * boost_scale + PNGX_REDUCED_ROUNDING_OFFSET);
    }

    if (head_dominance > PNGX_REDUCED_TARGET_HEAD_DOMINANCE_THRESHOLD && detail_pressure < PNGX_REDUCED_TARGET_DETAIL_PRESSURE_HEAD_LIMIT) {
      gradient_relief =
          stats ? clamp_float((PNGX_REDUCED_TARGET_GRADIENT_RELIEF_REF - stats->gradient_mean) / PNGX_REDUCED_TARGET_GRADIENT_RELIEF_REF, 0.0f, 1.0f) : PNGX_REDUCED_TARGET_GRADIENT_RELIEF_DEFAULT;
      dominance_cut = clamp_float((head_dominance - PNGX_REDUCED_TARGET_HEAD_DOMINANCE_THRESHOLD) * (PNGX_REDUCED_TARGET_HEAD_CUT_BASE + PNGX_REDUCED_TARGET_HEAD_CUT_RELIEF * gradient_relief), 0.0f,
                                  PNGX_REDUCED_TARGET_HEAD_CUT_CLAMP);
      target = (uint32_t)((float)target * (1.0f - dominance_cut) + PNGX_REDUCED_ROUNDING_OFFSET);
    }

    if (head_dominance > PNGX_REDUCED_TARGET_HEAD_DOMINANCE_STRONG && low_weight_ratio > PNGX_REDUCED_TARGET_LOW_WEIGHT_RATIO_STRONG &&
        detail_pressure < PNGX_REDUCED_TARGET_DETAIL_PRESSURE_STRONG_LIMIT) {
      gradient_relief = stats ? clamp_float((PNGX_REDUCED_TARGET_GRADIENT_RELIEF_SECONDARY_REF - stats->gradient_mean) / PNGX_REDUCED_TARGET_GRADIENT_RELIEF_SECONDARY_REF, 0.0f, 1.0f)
                              : PNGX_REDUCED_TARGET_GRADIENT_RELIEF_SECONDARY_DEFAULT;
      softness_relief = stats ? clamp_float((PNGX_REDUCED_TARGET_SATURATION_RELIEF_REF - stats->saturation_mean) / PNGX_REDUCED_TARGET_SATURATION_RELIEF_REF, 0.0f, 1.0f)
                              : PNGX_REDUCED_TARGET_SATURATION_RELIEF_DEFAULT;
      relief = clamp_float((gradient_relief * PNGX_REDUCED_TARGET_RELIEF_GRADIENT_WEIGHT + softness_relief * PNGX_REDUCED_TARGET_RELIEF_SATURATION_WEIGHT) * PNGX_REDUCED_TARGET_RELIEF_SCALE, 0.0f,
                           PNGX_REDUCED_TARGET_RELIEF_CLAMP);
      dominance_gain = clamp_float((head_dominance - PNGX_REDUCED_TARGET_HEAD_DOMINANCE_STRONG) * PNGX_REDUCED_TARGET_DOMINANCE_GAIN_SCALE, 0.0f, PNGX_REDUCED_TARGET_DOMINANCE_GAIN_CLAMP);
      tail_gain = clamp_float((low_weight_ratio - PNGX_REDUCED_TARGET_LOW_WEIGHT_RATIO_STRONG) *
                                  (PNGX_REDUCED_TARGET_TAIL_GAIN_BASE + PNGX_REDUCED_TARGET_TAIL_GAIN_RELIEF * (gradient_relief + softness_relief)),
                              0.0f, PNGX_REDUCED_TARGET_TAIL_GAIN_CLAMP);
      detail_relief = clamp_float((PNGX_REDUCED_TARGET_DETAIL_RELIEF_BASE - detail_pressure) * PNGX_REDUCED_TARGET_DETAIL_RELIEF_SCALE, 0.0f, PNGX_REDUCED_TARGET_DETAIL_RELIEF_CLAMP);
      combined_cut = clamp_float((dominance_gain + tail_gain) * (PNGX_REDUCED_TARGET_COMBINED_CUT_BASE + relief + detail_relief), 0.0f, PNGX_REDUCED_TARGET_COMBINED_CUT_CLAMP);
      target = (uint32_t)((float)target * (1.0f - combined_cut) + PNGX_REDUCED_ROUNDING_OFFSET);
    }

    if (density < PNGX_REDUCED_TARGET_DENSITY_THRESHOLD && detail_pressure < PNGX_REDUCED_TARGET_DETAIL_PRESSURE_DENSITY_LIMIT) {
      density_gap = clamp_float((PNGX_REDUCED_TARGET_DENSITY_THRESHOLD - (float)density) * PNGX_REDUCED_TARGET_DENSITY_GAP_SCALE, 0.0f, PNGX_REDUCED_TARGET_DENSITY_GAP_CLAMP);
      target = (uint32_t)((float)target * (1.0f - density_gap) + PNGX_REDUCED_ROUNDING_OFFSET);
    }
  }

  if (stats) {
    flatness_score = stats_flatness_factor(stats);
    if (flatness_score > PNGX_REDUCED_TARGET_FLATNESS_THRESHOLD && detail_pressure < PNGX_REDUCED_TARGET_DETAIL_PRESSURE_FLAT_LIMIT) {
      flatness_reduction = clamp_float(flatness_score * PNGX_REDUCED_TARGET_FLATNESS_SCALE, 0.0f, PNGX_REDUCED_TARGET_FLATNESS_CLAMP);
      target = (uint32_t)((float)target * (1.0f - flatness_reduction) + PNGX_REDUCED_ROUNDING_OFFSET);
    }

    alpha_simple = stats_alpha_simplicity(stats);
    if (alpha_simple > PNGX_REDUCED_TARGET_ALPHA_SIMPLE_THRESHOLD && detail_pressure < PNGX_REDUCED_TARGET_DETAIL_PRESSURE_ALPHA_LIMIT) {
      alpha_reduction = clamp_float(alpha_simple * PNGX_REDUCED_TARGET_ALPHA_SIMPLE_SCALE, 0.0f, PNGX_REDUCED_TARGET_ALPHA_SIMPLE_CLAMP);
      target = (uint32_t)((float)target * (1.0f - alpha_reduction) + PNGX_REDUCED_ROUNDING_OFFSET);
    }
  }

  if (hint <= 0 && unique_colors > PNGX_REDUCED_TARGET_GENTLE_MIN_COLORS && unique_colors <= PNGX_REDUCED_TARGET_GENTLE_MAX_COLORS &&
      detail_pressure < PNGX_REDUCED_TARGET_DETAIL_PRESSURE_GENTLE_LIMIT) {
    span_ratio = ((float)unique_colors - (float)PNGX_REDUCED_TARGET_GENTLE_MIN_COLORS) / PNGX_REDUCED_TARGET_GENTLE_COLOR_RANGE;
    if (span_ratio < 0.0f) {
      span_ratio = 0.0f;
    } else if (span_ratio > 1.0f) {
      span_ratio = 1.0f;
    }

    gentle_cut = clamp_float((1.0f - detail_pressure) * PNGX_REDUCED_TARGET_GENTLE_SCALE, 0.0f, PNGX_REDUCED_TARGET_GENTLE_CLAMP);
    applied_cut = span_ratio * gentle_cut;
    target = (uint32_t)((float)target * (1.0f - applied_cut) + PNGX_REDUCED_ROUNDING_OFFSET);
  }

  if (target > unique_colors) {
    target = (uint32_t)unique_colors;
  }

  if (target < (uint32_t)COLOPRESSO_PNGX_REDUCED_COLORS_MIN) {
    target = COLOPRESSO_PNGX_REDUCED_COLORS_MIN;
  }

  if (target > grid_cap) {
    target = grid_cap;
  }

  if (unlocked > 0 && target > (uint32_t)unlocked) {
    target = (uint32_t)unlocked;
  }

  if (unlocked == 0) {
    target = (uint32_t)unique_colors;
  }

  return target;
}

bool pngx_quantize_reduced_rgba32(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint32_t *resolved_target, uint32_t *applied_colors, uint8_t **out_data, size_t *out_size) {
  uint32_t target = 0, actual = 0, manual_limit = 0, auto_trim_limit = 0, grid_cap;
  uint8_t bits_rgb, bits_alpha;
  size_t grid_unique, passthrough_threshold;
  bool wrote, manual_target = false, success, auto_target, grid_passthrough, auto_trim_applied = false;
  pngx_rgba_image_t image;
  pngx_quant_support_t support = {0};
  pngx_image_stats_t stats;
  pngx_options_t tuned_opts;
  color_histogram_t histogram;

  if (!png_data || png_size == 0 || !opts || !out_data || !out_size) {
    return false;
  }

  if (!load_rgba_image(png_data, png_size, &image)) {
    return false;
  }

  histogram.entries = NULL;
  histogram.count = 0;
  histogram.unlocked_count = 0;
  tuned_opts = *opts;

  image_stats_reset(&stats);

  if (!prepare_quant_support(&image, &tuned_opts, &support, &stats)) {
    rgba_image_reset(&image);
    return false;
  }

  tune_reduced_bitdepth(&image, &stats, &tuned_opts.lossy_reduced_bits_rgb, &tuned_opts.lossy_reduced_alpha_bits);
  if (!apply_reduced_rgba32_prepass(&image, &tuned_opts, &support, &stats)) {
    color_histogram_reset(&histogram);
    quant_support_reset(&support);
    rgba_image_reset(&image);

    return false;
  }

  bits_rgb = clamp_reduced_bits(tuned_opts.lossy_reduced_bits_rgb);
  bits_alpha = clamp_reduced_bits(tuned_opts.lossy_reduced_alpha_bits);
  grid_cap = compute_grid_capacity(bits_rgb, bits_alpha);
  if (grid_cap == 0) {
    grid_cap = COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
  }
  if (opts->lossy_reduced_colors >= (int32_t)COLOPRESSO_PNGX_REDUCED_COLORS_MIN) {
    manual_target = true;
    manual_limit = (uint32_t)opts->lossy_reduced_colors;
    if (manual_limit > grid_cap) {
      manual_limit = grid_cap;
    }
  }

  grid_unique = count_unique_rgba(image.rgba, image.pixel_count);
  grid_cap = compute_grid_capacity(bits_rgb, bits_alpha);
  auto_target = (opts->lossy_reduced_colors <= 0);
  grid_passthrough = false;

  if (grid_cap == 0) {
    grid_cap = COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
  }

  passthrough_threshold = resolve_reduced_passthrough_threshold(grid_cap, &stats);

  if (grid_unique > (size_t)grid_cap) {
    grid_unique = (size_t)grid_cap;
  }

  if (auto_target && grid_unique >= passthrough_threshold) {
    grid_passthrough = true;
  }

  if (grid_passthrough) {
    wrote = false;
    snap_rgba_image_to_bits(opts->thread_count, image.rgba, image.pixel_count, bits_rgb, bits_alpha);

    if (resolved_target) {
      *resolved_target = (uint32_t)grid_unique;
    }

    if (applied_colors) {
      *applied_colors = (uint32_t)grid_unique;
    }

    wrote = create_rgba_png(image.rgba, image.pixel_count, image.width, image.height, out_data, out_size);
    if (wrote) {
      colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Reduced RGBA32 grid passthrough kept %zu colors (capacity=%u)", grid_unique, grid_cap);
    }

    quant_support_reset(&support);
    rgba_image_reset(&image);

    return wrote;
  }

  if (!build_color_histogram(&image, &tuned_opts, &support, &histogram)) {
    quant_support_reset(&support);
    rgba_image_reset(&image);

    return false;
  }

  target = resolve_reduced_rgba32_target(&histogram, image.pixel_count, opts->lossy_reduced_colors, bits_rgb, bits_alpha, &stats);
  if (histogram.unlocked_count == 0 || target == 0) {
    actual = (uint32_t)histogram.count;
    snap_rgba_image_to_bits(opts->thread_count, image.rgba, image.pixel_count, bits_rgb, bits_alpha);
  } else {
    if (!apply_reduced_rgba32_quantization(opts->thread_count, &histogram, &image, target, bits_rgb, bits_alpha, &actual)) {
      color_histogram_reset(&histogram);
      quant_support_reset(&support);
      rgba_image_reset(&image);

      return false;
    }
  }

  if (!manual_target) {
    auto_trim_limit = compute_auto_trim_limit(&histogram, image.pixel_count, actual, bits_rgb, bits_alpha, &stats);
    if (auto_trim_limit > 0 && auto_trim_limit < actual) {
      if (enforce_manual_reduced_limit(opts->thread_count, &image, auto_trim_limit, bits_rgb, bits_alpha, &actual)) {
        auto_trim_applied = true;
        colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Reduced RGBA32 auto trim applied %u -> %u colors", target, auto_trim_limit);
      } else {
        colopresso_log(CPRES_LOG_LEVEL_WARNING, "PNGX: Reduced RGBA32 auto trim request failed (limit=%u)", auto_trim_limit);
        auto_trim_limit = 0;
      }
    } else {
      auto_trim_limit = 0;
    }
  }

  if (manual_target) {
    if (!enforce_manual_reduced_limit(opts->thread_count, &image, manual_limit, bits_rgb, bits_alpha, &actual)) {
      color_histogram_reset(&histogram);
      quant_support_reset(&support);
      rgba_image_reset(&image);

      return false;
    }
  }

  success = create_rgba_png(image.rgba, image.pixel_count, image.width, image.height, out_data, out_size);

  if (resolved_target) {
    if (manual_target) {
      *resolved_target = manual_limit;
    } else if (auto_trim_applied && auto_trim_limit > 0) {
      *resolved_target = auto_trim_limit;
    } else if (histogram.unlocked_count == 0 || target == 0) {
      *resolved_target = (uint32_t)histogram.count;
    } else {
      *resolved_target = target;
    }
  }

  if (applied_colors) {
    *applied_colors = actual;
  }

  color_histogram_reset(&histogram);
  quant_support_reset(&support);
  rgba_image_reset(&image);

  return success;
}
