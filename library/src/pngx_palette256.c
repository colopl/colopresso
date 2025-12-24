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

#include <stdlib.h>
#include <string.h>

#include <png.h>
#include <zlib.h>

#include "internal/log.h"
#include "internal/pngx_common.h"

typedef struct {
  pngx_rgba_image_t image;
  pngx_quant_support_t support;
  pngx_image_stats_t stats;
  pngx_options_t tuned_opts;
  float resolved_dither;
  bool prefer_uniform;
  bool initialized;
} palette256_context_t;

static palette256_context_t g_palette256_ctx = {0};

static inline void alpha_bleed_rgb_from_opaque(uint8_t *rgba, uint32_t width, uint32_t height, const pngx_options_t *opts) {
  uint32_t *seed_rgb, x, y, best_rgb;
  uint16_t *dist, best, d, max_distance;
  uint8_t opaque_threshold, soft_limit;
  size_t pixel_count, i, base, idx;
  bool has_seed;

  if (!rgba || width == 0 || height == 0) {
    return;
  }

  if (!opts || !opts->palette256_alpha_bleed_enable) {
    return;
  }

  max_distance = opts->palette256_alpha_bleed_max_distance;
  opaque_threshold = opts->palette256_alpha_bleed_opaque_threshold;
  soft_limit = opts->palette256_alpha_bleed_soft_limit;

  if ((size_t)width > SIZE_MAX / (size_t)height) {
    return;
  }
  pixel_count = (size_t)width * (size_t)height;

  dist = (uint16_t *)malloc(sizeof(uint16_t) * pixel_count);
  seed_rgb = (uint32_t *)malloc(sizeof(uint32_t) * pixel_count);
  if (!dist || !seed_rgb) {
    free(dist);
    free(seed_rgb);
    return;
  }

  has_seed = false;
  for (i = 0; i < pixel_count; ++i) {
    base = i * 4;
    if (rgba[base + 3] == 0) {
      rgba[base + 0] = 0;
      rgba[base + 1] = 0;
      rgba[base + 2] = 0;
    }

    if (rgba[base + 3] >= opaque_threshold) {
      dist[i] = 0;
      seed_rgb[i] = ((uint32_t)rgba[base + 0] << 16) | ((uint32_t)rgba[base + 1] << 8) | (uint32_t)rgba[base + 2];
      has_seed = true;
    } else {
      dist[i] = UINT16_MAX;
      seed_rgb[i] = 0;
    }
  }

  if (!has_seed) {
    free(dist);
    free(seed_rgb);
    return;
  }

  for (i = 0; i < 3; ++i) {
    for (y = 0; y < height; ++y) {
      for (x = 0; x < width; ++x) {
        idx = (size_t)y * (size_t)width + (size_t)x;
        best = dist[idx];
        best_rgb = seed_rgb[idx];

        if (x > 0 && dist[idx - 1] != UINT16_MAX && (uint16_t)(dist[idx - 1] + 1) < best) {
          best = (uint16_t)(dist[idx - 1] + 1);
          best_rgb = seed_rgb[idx - 1];
        }
        if (y > 0 && dist[idx - (size_t)width] != UINT16_MAX && (uint16_t)(dist[idx - (size_t)width] + 1) < best) {
          best = (uint16_t)(dist[idx - (size_t)width] + 1);
          best_rgb = seed_rgb[idx - (size_t)width];
        }
        if (x > 0 && y > 0 && dist[idx - (size_t)width - 1] != UINT16_MAX && (uint16_t)(dist[idx - (size_t)width - 1] + 1) < best) {
          best = (uint16_t)(dist[idx - (size_t)width - 1] + 1);
          best_rgb = seed_rgb[idx - (size_t)width - 1];
        }
        if (x + 1 < width && y > 0 && dist[idx - (size_t)width + 1] != UINT16_MAX && (uint16_t)(dist[idx - (size_t)width + 1] + 1) < best) {
          best = (uint16_t)(dist[idx - (size_t)width + 1] + 1);
          best_rgb = seed_rgb[idx - (size_t)width + 1];
        }

        dist[idx] = best;
        seed_rgb[idx] = best_rgb;
      }
    }

    for (y = height; y-- > 0;) {
      for (x = width; x-- > 0;) {
        idx = (size_t)y * (size_t)width + (size_t)x;
        best = dist[idx];
        best_rgb = seed_rgb[idx];

        if (x + 1 < width && dist[idx + 1] != UINT16_MAX && (uint16_t)(dist[idx + 1] + 1) < best) {
          best = (uint16_t)(dist[idx + 1] + 1);
          best_rgb = seed_rgb[idx + 1];
        }
        if (y + 1 < height && dist[idx + (size_t)width] != UINT16_MAX && (uint16_t)(dist[idx + (size_t)width] + 1) < best) {
          best = (uint16_t)(dist[idx + (size_t)width] + 1);
          best_rgb = seed_rgb[idx + (size_t)width];
        }
        if (x + 1 < width && y + 1 < height && dist[idx + (size_t)width + 1] != UINT16_MAX && (uint16_t)(dist[idx + (size_t)width + 1] + 1) < best) {
          best = (uint16_t)(dist[idx + (size_t)width + 1] + 1);
          best_rgb = seed_rgb[idx + (size_t)width + 1];
        }
        if (x > 0 && y + 1 < height && dist[idx + (size_t)width - 1] != UINT16_MAX && (uint16_t)(dist[idx + (size_t)width - 1] + 1) < best) {
          best = (uint16_t)(dist[idx + (size_t)width - 1] + 1);
          best_rgb = seed_rgb[idx + (size_t)width - 1];
        }

        dist[idx] = best;
        seed_rgb[idx] = best_rgb;
      }
    }
  }

  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      idx = (size_t)y * (size_t)width + (size_t)x;
      d = dist[idx];
      base = idx * 4;

      if (rgba[base + 3] <= soft_limit && d != UINT16_MAX && d <= max_distance) {
        rgba[base + 0] = (uint8_t)((seed_rgb[idx] >> 16) & 0xFFu);
        rgba[base + 1] = (uint8_t)((seed_rgb[idx] >> 8) & 0xFFu);
        rgba[base + 2] = (uint8_t)(seed_rgb[idx] & 0xFFu);
      }
    }
  }

  free(dist);
  free(seed_rgb);
}

static inline void sanitize_transparent_palette(cpres_rgba_color_t *palette, size_t palette_len) {
  size_t i;

  if (!palette || palette_len == 0) {
    return;
  }

  for (i = 0; i < palette_len; ++i) {
    if (palette[i].a == 0) {
      palette[i].r = 0;
      palette[i].g = 0;
      palette[i].b = 0;
    }
  }
}

static inline bool is_smooth_gradient_profile(const pngx_image_stats_t *stats, const pngx_options_t *opts) {
  float opaque_ratio_threshold, gradient_mean_max, saturation_mean_max;

  if (!stats) {
    return false;
  }

  opaque_ratio_threshold = (opts ? opts->palette256_profile_opaque_ratio_threshold : -1.0f);
  if (opaque_ratio_threshold < 0.0f) {
    opaque_ratio_threshold = PNGX_PALETTE256_GRADIENT_PROFILE_OPAQUE_RATIO_THRESHOLD;
  }

  gradient_mean_max = (opts ? opts->palette256_profile_gradient_mean_max : -1.0f);
  if (gradient_mean_max < 0.0f) {
    gradient_mean_max = PNGX_PALETTE256_GRADIENT_PROFILE_GRADIENT_MEAN_MAX;
  }

  saturation_mean_max = (opts ? opts->palette256_profile_saturation_mean_max : -1.0f);
  if (saturation_mean_max < 0.0f) {
    saturation_mean_max = PNGX_PALETTE256_GRADIENT_PROFILE_SATURATION_MEAN_MAX;
  }

  if (stats->opaque_ratio > opaque_ratio_threshold && stats->gradient_mean < gradient_mean_max && stats->saturation_mean < saturation_mean_max) {
    return true;
  }

  return false;
}

static inline void tune_quant_params_for_image(PngxBridgeQuantParams *params, const pngx_options_t *opts, const pngx_image_stats_t *stats) {
  uint8_t quality_min, quality_max;
  int32_t speed_max, quality_min_floor, quality_max_target;
  float opaque_ratio_threshold, gradient_mean_max, saturation_mean_max;

  if (!params || !opts || !stats) {
    return;
  }

  opaque_ratio_threshold = opts->palette256_tune_opaque_ratio_threshold;
  if (opaque_ratio_threshold < 0.0f) {
    opaque_ratio_threshold = PNGX_PALETTE256_TUNE_OPAQUE_RATIO_THRESHOLD;
  }
  gradient_mean_max = opts->palette256_tune_gradient_mean_max;
  if (gradient_mean_max < 0.0f) {
    gradient_mean_max = PNGX_PALETTE256_TUNE_GRADIENT_MEAN_MAX;
  }
  saturation_mean_max = opts->palette256_tune_saturation_mean_max;
  if (saturation_mean_max < 0.0f) {
    saturation_mean_max = PNGX_PALETTE256_TUNE_SATURATION_MEAN_MAX;
  }

  speed_max = opts->palette256_tune_speed_max;
  if (speed_max < 0) {
    speed_max = PNGX_PALETTE256_TUNE_SPEED_MAX;
  }
  quality_min_floor = opts->palette256_tune_quality_min_floor;
  if (quality_min_floor < 0) {
    quality_min_floor = PNGX_PALETTE256_TUNE_QUALITY_MIN_FLOOR;
  }
  quality_max_target = opts->palette256_tune_quality_max_target;
  if (quality_max_target < 0) {
    quality_max_target = PNGX_PALETTE256_TUNE_QUALITY_MAX_TARGET;
  }

  if (stats->opaque_ratio > opaque_ratio_threshold && stats->gradient_mean < gradient_mean_max && stats->saturation_mean < saturation_mean_max) {
    if (params->speed > speed_max) {
      params->speed = speed_max;
    }

    quality_min = params->quality_min;
    quality_max = params->quality_max;

    if (quality_max < (uint8_t)quality_max_target) {
      quality_max = (uint8_t)quality_max_target;
    }
    if (quality_min < (uint8_t)quality_min_floor) {
      quality_min = (uint8_t)quality_min_floor;
    }
    if (quality_min > quality_max) {
      quality_min = quality_max;
    }

    params->quality_min = quality_min;
    params->quality_max = quality_max;
  }
}

static inline void postprocess_indices(uint8_t *indices, uint32_t width, uint32_t height, const cpres_rgba_color_t *palette, size_t palette_len, const pngx_quant_support_t *support,
                                       const pngx_options_t *opts) {
  uint32_t x, y, dist_sq;
  uint8_t base_color, importance, neighbor_colors[4], neighbor_used, *reference, candidate;
  size_t pixel_count, idx;
  float cutoff;
  bool neighbor_has_base;

  if (!indices || !support || !opts || width == 0 || height == 0) {
    return;
  }

  if (!opts->postprocess_smooth_enable) {
    return;
  }

  if (opts->lossy_dither_level >= PNGX_POSTPROCESS_DISABLE_DITHER_THRESHOLD) {
    return;
  }

  if (!support->importance_map || support->importance_map_len < (size_t)width * (size_t)height) {
    return;
  }

  cutoff = opts->postprocess_smooth_importance_cutoff;
  if (cutoff >= 0.0f) {
    if (cutoff > 1.0f) {
      cutoff = 1.0f;
    }
  } else {
    cutoff = -1.0f;
  }

  pixel_count = (size_t)width * (size_t)height;

  reference = (uint8_t *)malloc(pixel_count);
  if (!reference) {
    return;
  }

  memcpy(reference, indices, pixel_count);

  for (y = 0; y < height; ++y) {
    for (x = 0; x < width; ++x) {
      idx = (size_t)y * (size_t)width + (size_t)x;
      base_color = reference[idx];
      importance = support->importance_map[idx];
      neighbor_used = 0;
      neighbor_has_base = false;

      if (cutoff >= 0.0f && ((float)importance / 255.0f) >= cutoff) {
        continue;
      }

      if (x > 0) {
        neighbor_colors[neighbor_used] = reference[idx - 1];
        if (neighbor_colors[neighbor_used] == base_color) {
          neighbor_has_base = true;
        }
        ++neighbor_used;
      }
      if (x + 1 < width) {
        neighbor_colors[neighbor_used] = reference[idx + 1];
        if (neighbor_colors[neighbor_used] == base_color) {
          neighbor_has_base = true;
        }
        ++neighbor_used;
      }
      if (y > 0) {
        neighbor_colors[neighbor_used] = reference[idx - (size_t)width];
        if (neighbor_colors[neighbor_used] == base_color) {
          neighbor_has_base = true;
        }
        ++neighbor_used;
      }
      if (y + 1 < height) {
        neighbor_colors[neighbor_used] = reference[idx + (size_t)width];
        if (neighbor_colors[neighbor_used] == base_color) {
          neighbor_has_base = true;
        }
        ++neighbor_used;
      }

      if (neighbor_used < 3) {
        continue;
      }

      candidate = neighbor_colors[0];
      if (neighbor_used == 3) {
        if (!(neighbor_colors[1] == candidate && neighbor_colors[2] == candidate)) {
          continue;
        }
      } else {
        if (!(neighbor_colors[1] == candidate && neighbor_colors[2] == candidate && neighbor_colors[3] == candidate)) {
          continue;
        }
      }

      if (candidate == base_color) {
        continue;
      }

      if (neighbor_has_base) {
        continue;
      }

      if (palette && palette_len > 0 && (size_t)base_color < palette_len && (size_t)candidate < palette_len) {
        dist_sq = color_distance_sq(&palette[base_color], &palette[candidate]);
        if (dist_sq > PNGX_POSTPROCESS_MAX_COLOR_DISTANCE_SQ) {
          continue;
        }
      }

      indices[idx] = candidate;
    }
  }

  free(reference);
}

static inline void fill_quant_params(PngxBridgeQuantParams *params, const pngx_options_t *opts, const uint8_t *importance_map, size_t importance_map_len) {
  uint8_t quality_min, quality_max;

  if (!params || !opts) {
    return;
  }

  quality_min = clamp_uint8_t(opts->lossy_quality_min, 0, 100);
  quality_max = clamp_uint8_t(opts->lossy_quality_max, quality_min, 100);
  params->speed = clamp_int32_t((int32_t)opts->lossy_speed, 1, 10);
  params->quality_min = quality_min;
  params->quality_max = quality_max;
  params->max_colors = clamp_uint32_t((uint32_t)opts->lossy_max_colors, 2, 256);
  params->min_posterization = -1;
  params->dithering_level = clamp_float(opts->lossy_dither_level, 0.0f, 1.0f);
  params->importance_map = importance_map;
  params->importance_map_len = importance_map_len;
  params->fixed_colors = opts->protected_colors;
  params->fixed_colors_len = (size_t)((opts->protected_colors_count > 0) ? opts->protected_colors_count : 0);
  params->remap = true;
}

static inline void free_quant_output(PngxBridgeQuantOutput *output) {
  if (!output) {
    return;
  }

  if (output->palette) {
    pngx_bridge_free((uint8_t *)output->palette);
    output->palette = NULL;
  }
  output->palette_len = 0;

  if (output->indices) {
    pngx_bridge_free(output->indices);
    output->indices = NULL;
  }

  output->indices_len = 0;
}

static inline bool init_write_struct(png_structp *png_ptr, png_infop *info_ptr) {
  png_structp tmp_png;
  png_infop tmp_info;

  if (!png_ptr || !info_ptr) {
    return false;
  }

  tmp_png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!tmp_png) {
    return false;
  }

  tmp_info = png_create_info_struct(tmp_png);
  if (!tmp_info) {
    png_destroy_write_struct(&tmp_png, NULL);
    return false;
  }

  *png_ptr = tmp_png;
  *info_ptr = tmp_info;

  return true;
}

static inline bool memory_buffer_reserve(png_memory_buffer_t *buffer, size_t additional) {
  uint8_t *new_data;
  size_t required, capacity;

  if (!buffer || additional > SIZE_MAX - buffer->size) {
    return false;
  }

  required = buffer->size + additional;
  capacity = buffer->capacity ? buffer->capacity : 4096;

  while (capacity < required) {
    if (capacity > SIZE_MAX / 2) {
      capacity = required;
      break;
    }

    capacity *= 2;
  }

  new_data = (uint8_t *)realloc(buffer->data, capacity);
  if (!new_data) {
    return false;
  }

  buffer->data = new_data;
  buffer->capacity = capacity;

  return true;
}

/* Excluded from coverage as this is only called in rare cases such as malloc failure. */
/* LCOV_EXCL_START */
static inline void memory_buffer_reset(png_memory_buffer_t *buffer) {
  if (!buffer) {
    return;
  }

  free(buffer->data);
  buffer->data = NULL;
  buffer->size = 0;
  buffer->capacity = 0;
}
/* LCOV_EXCL_STOP */

static inline void memory_write(png_structp png_ptr, png_bytep data, png_size_t length) {
  png_memory_buffer_t *buffer = (png_memory_buffer_t *)png_get_io_ptr(png_ptr);

  if (!buffer || !memory_buffer_reserve(buffer, length)) {
    png_error(png_ptr, "png write failure");
    return;
  }

  memcpy(buffer->data + buffer->size, data, length);

  buffer->size += length;
}

static inline bool finalize_memory_png(png_memory_buffer_t *buffer, uint8_t **out_data, size_t *out_size) {
  uint8_t *shrunk;

  if (!buffer || !out_data || !out_size) {
    return false;
  }

  if (buffer->data) {
    shrunk = (uint8_t *)realloc(buffer->data, buffer->size);
    if (shrunk) {
      buffer->data = shrunk;
      buffer->capacity = buffer->size;
    }
  }

  *out_data = buffer->data;
  *out_size = buffer->size;
  if (!buffer->data || buffer->size == 0) {
    memory_buffer_reset(buffer);
    return false;
  }

  return true;
}

static inline png_bytep *build_row_pointers(const uint8_t *data, size_t row_stride, uint32_t height) {
  png_bytep *row_pointers;
  uint32_t y;

  if (!data || row_stride == 0 || height == 0) {
    return NULL;
  }

  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
  if (!row_pointers) {
    return NULL;
  }

  for (y = 0; y < height; ++y) {
    row_pointers[y] = (png_bytep)(data + (size_t)y * row_stride);
  }

  return row_pointers;
}

extern bool pngx_create_palette_png(const uint8_t *indices, size_t indices_len, const cpres_rgba_color_t *palette, size_t palette_len, uint32_t width, uint32_t height, uint8_t **out_data,
                                    size_t *out_size) {
  png_color palette_data[256];
  png_byte alpha_data[256];
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row_pointers;
  png_memory_buffer_t buffer;
  size_t expected_len, i;
  int num_trans;

  if (!indices || !palette || !out_data || !out_size || width == 0 || height == 0) {
    return false;
  }

  expected_len = (size_t)width * (size_t)height;
  if (indices_len != expected_len || palette_len == 0 || palette_len > 256) {
    return false;
  }

  if (!init_write_struct(&png_ptr, &info_ptr)) {
    return false;
  }

  buffer.data = NULL;
  buffer.size = 0;
  buffer.capacity = 0;
  row_pointers = NULL;

  if (setjmp(png_jmpbuf(png_ptr)) != 0) {
    memory_buffer_reset(&buffer);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(row_pointers);
    return false;
  }

  png_set_write_fn(png_ptr, &buffer, memory_write, NULL);
  png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
  png_set_compression_strategy(png_ptr, Z_FILTERED);
  png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_ALL_FILTERS);
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  num_trans = 0;
  for (i = 0; i < palette_len; ++i) {
    palette_data[i].red = palette[i].r;
    palette_data[i].green = palette[i].g;
    palette_data[i].blue = palette[i].b;
    alpha_data[i] = palette[i].a;
    if (palette[i].a != 255) {
      num_trans = (int)(i + 1);
    }
  }

  png_set_PLTE(png_ptr, info_ptr, palette_data, (int)palette_len);
  if (num_trans > 0) {
    png_set_tRNS(png_ptr, info_ptr, alpha_data, num_trans, NULL);
  }

  png_write_info(png_ptr, info_ptr);

  row_pointers = build_row_pointers(indices, (size_t)width, height);
  if (!row_pointers) {
    png_error(png_ptr, "png row allocation failed");
    return false;
  }

  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, NULL);

  free(row_pointers);
  png_destroy_write_struct(&png_ptr, &info_ptr);

  return finalize_memory_png(&buffer, out_data, out_size);
}

extern bool create_rgba_png(const uint8_t *rgba, size_t pixel_count, uint32_t width, uint32_t height, uint8_t **out_data, size_t *out_size) {
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row_pointers;
  png_memory_buffer_t buffer;
  size_t expected_pixels;

  if (!rgba || !out_data || !out_size || width == 0 || height == 0) {
    return false;
  }

  expected_pixels = (size_t)width * (size_t)height;
  if (pixel_count != expected_pixels) {
    return false;
  }

  if (!init_write_struct(&png_ptr, &info_ptr)) {
    return false;
  }

  buffer.data = NULL;
  buffer.size = 0;
  buffer.capacity = 0;
  row_pointers = NULL;

  if (setjmp(png_jmpbuf(png_ptr)) != 0) {
    memory_buffer_reset(&buffer);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    free(row_pointers);
    return false;
  }

  png_set_write_fn(png_ptr, &buffer, memory_write, NULL);
  png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);
  png_set_compression_strategy(png_ptr, Z_FILTERED);
  png_set_filter(png_ptr, PNG_FILTER_TYPE_BASE, PNG_ALL_FILTERS);
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, info_ptr);

  row_pointers = build_row_pointers(rgba, (size_t)width * PNGX_RGBA_CHANNELS, height);
  if (!row_pointers) {
    png_error(png_ptr, "png row allocation failed");

    return false;
  }

  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, NULL);

  free(row_pointers);
  png_destroy_write_struct(&png_ptr, &info_ptr);

  return finalize_memory_png(&buffer, out_data, out_size);
}

bool pngx_quantize_palette256(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size, int *quant_quality) {
  PngxBridgeQuantParams params = {0}, fallback_params = {0};
  PngxBridgeQuantOutput output = {0};
  PngxBridgeQuantStatus status;
  uint8_t *rgba, *importance_map, *fixed_colors, quality_min, quality_max;
  uint32_t width, height, max_colors;
  int32_t speed;
  size_t pixel_count, importance_map_len, fixed_colors_len;
  float dither_level;
  bool relaxed_quality, success;

  if (!png_data || png_size == 0 || !opts || !out_data || !out_size) {
    return false;
  }

  if (quant_quality) {
    *quant_quality = -1;
  }

  if (!pngx_palette256_prepare(png_data, png_size, opts, &rgba, &width, &height, &importance_map, &importance_map_len, &speed, &quality_min, &quality_max, &max_colors, &dither_level, &fixed_colors,
                               &fixed_colors_len)) {
    return false;
  }

  pixel_count = (size_t)width * (size_t)height;

  params.speed = speed;
  params.quality_min = quality_min;
  params.quality_max = quality_max;
  params.max_colors = max_colors;
  params.min_posterization = -1;
  params.dithering_level = dither_level;
  params.importance_map = importance_map;
  params.importance_map_len = importance_map_len;
  params.fixed_colors = (const cpres_rgba_color_t *)fixed_colors;
  params.fixed_colors_len = fixed_colors_len;
  params.remap = true;

  output.quality = -1;
  relaxed_quality = false;

  status = pngx_bridge_quantize((const cpres_rgba_color_t *)rgba, pixel_count, width, height, &params, &output);
  pngx_set_last_error((int)status);

  if (status == PNGX_BRIDGE_QUANT_STATUS_QUALITY_TOO_LOW && params.quality_min > 0) {
    fallback_params = params;
    fallback_params.quality_min = 0;

    if (fallback_params.quality_max < fallback_params.quality_min) {
      fallback_params.quality_max = fallback_params.quality_min;
    }

    output.palette = NULL;
    output.palette_len = 0;
    output.indices = NULL;
    output.indices_len = 0;
    output.quality = -1;

    status = pngx_bridge_quantize((const cpres_rgba_color_t *)rgba, pixel_count, width, height, &fallback_params, &output);
    pngx_set_last_error((int)status);
    if (status == PNGX_BRIDGE_QUANT_STATUS_OK) {
      relaxed_quality = true;
    }
  }

  if (status != PNGX_BRIDGE_QUANT_STATUS_OK) {
    free_quant_output(&output);
    pngx_palette256_cleanup();
    if (status == PNGX_BRIDGE_QUANT_STATUS_QUALITY_TOO_LOW) {
      colopresso_log(CPRES_LOG_LEVEL_WARNING, "PNGX: Quantization quality too low");
    }

    return false;
  }

  if (relaxed_quality) {
    colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Relaxed quantization quality floor");
  }

  if (quant_quality) {
    *quant_quality = output.quality;
  }

  success = false;
  if (output.indices && output.indices_len == pixel_count && output.palette && output.palette_len > 0 && output.palette_len <= 256) {
    success = pngx_palette256_finalize(output.indices, output.indices_len, output.palette, output.palette_len, out_data, out_size);
  } else {
    pngx_palette256_cleanup();
  }

  free_quant_output(&output);

  return success;
}

bool pngx_palette256_prepare(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_rgba, uint32_t *out_width, uint32_t *out_height, uint8_t **out_importance_map,
                             size_t *out_importance_map_len, int32_t *out_speed, uint8_t *out_quality_min, uint8_t *out_quality_max, uint32_t *out_max_colors, float *out_dither_level,
                             uint8_t **out_fixed_colors, size_t *out_fixed_colors_len) {
  float estimated_dither, gradient_dither_floor;
  PngxBridgeQuantParams params = {0};

  if (!png_data || png_size == 0 || !opts || !out_rgba || !out_width || !out_height) {
    return false;
  }

  if (g_palette256_ctx.initialized) {
    rgba_image_reset(&g_palette256_ctx.image);
    quant_support_reset(&g_palette256_ctx.support);
    memset(&g_palette256_ctx, 0, sizeof(g_palette256_ctx));
  }

  if (!load_rgba_image(png_data, png_size, &g_palette256_ctx.image)) {
    return false;
  }

  alpha_bleed_rgb_from_opaque(g_palette256_ctx.image.rgba, g_palette256_ctx.image.width, g_palette256_ctx.image.height, opts);

  image_stats_reset(&g_palette256_ctx.stats);
  if (!prepare_quant_support(&g_palette256_ctx.image, opts, &g_palette256_ctx.support, &g_palette256_ctx.stats)) {
    rgba_image_reset(&g_palette256_ctx.image);
    quant_support_reset(&g_palette256_ctx.support);
    return false;
  }

  g_palette256_ctx.tuned_opts = *opts;
  g_palette256_ctx.prefer_uniform = opts->palette256_gradient_profile_enable ? is_smooth_gradient_profile(&g_palette256_ctx.stats, &g_palette256_ctx.tuned_opts) : false;

  if (g_palette256_ctx.prefer_uniform) {
    g_palette256_ctx.tuned_opts.saliency_map_enable = false;
    g_palette256_ctx.tuned_opts.chroma_anchor_enable = false;
    g_palette256_ctx.tuned_opts.postprocess_smooth_enable = false;
  } else {
    build_fixed_palette(opts, &g_palette256_ctx.support, &g_palette256_ctx.tuned_opts);
  }

  g_palette256_ctx.resolved_dither = resolve_quant_dither(opts, &g_palette256_ctx.stats);

  if (opts->lossy_dither_auto) {
    estimated_dither = estimate_bitdepth_dither_level(g_palette256_ctx.image.rgba, g_palette256_ctx.image.width, g_palette256_ctx.image.height, 8);
    if (estimated_dither > g_palette256_ctx.resolved_dither) {
      g_palette256_ctx.resolved_dither = estimated_dither;
    }
  }

  gradient_dither_floor = g_palette256_ctx.tuned_opts.palette256_gradient_profile_dither_floor;
  if (gradient_dither_floor < 0.0f) {
    gradient_dither_floor = PNGX_PALETTE256_GRADIENT_PROFILE_DITHER_FLOOR;
  }

  if (g_palette256_ctx.prefer_uniform && g_palette256_ctx.resolved_dither < gradient_dither_floor) {
    g_palette256_ctx.resolved_dither = gradient_dither_floor;
  }

  g_palette256_ctx.tuned_opts.lossy_dither_level = g_palette256_ctx.resolved_dither;

  fill_quant_params(&params, &g_palette256_ctx.tuned_opts, g_palette256_ctx.prefer_uniform ? NULL : g_palette256_ctx.support.importance_map,
                    g_palette256_ctx.prefer_uniform ? 0 : g_palette256_ctx.support.importance_map_len);
  params.dithering_level = g_palette256_ctx.resolved_dither;
  tune_quant_params_for_image(&params, &g_palette256_ctx.tuned_opts, &g_palette256_ctx.stats);

  *out_rgba = g_palette256_ctx.image.rgba;
  *out_width = g_palette256_ctx.image.width;
  *out_height = g_palette256_ctx.image.height;

  if (out_importance_map && out_importance_map_len) {
    if (!g_palette256_ctx.prefer_uniform && g_palette256_ctx.support.importance_map) {
      *out_importance_map = g_palette256_ctx.support.importance_map;
      *out_importance_map_len = g_palette256_ctx.support.importance_map_len;
    } else {
      *out_importance_map = NULL;
      *out_importance_map_len = 0;
    }
  }

  if (out_speed)
    *out_speed = params.speed;
  if (out_quality_min)
    *out_quality_min = params.quality_min;
  if (out_quality_max)
    *out_quality_max = params.quality_max;
  if (out_max_colors)
    *out_max_colors = params.max_colors;
  if (out_dither_level)
    *out_dither_level = params.dithering_level;

  if (out_fixed_colors && out_fixed_colors_len) {
    if (params.fixed_colors && params.fixed_colors_len > 0) {
      *out_fixed_colors = (uint8_t *)params.fixed_colors;
      *out_fixed_colors_len = params.fixed_colors_len;
    } else {
      *out_fixed_colors = NULL;
      *out_fixed_colors_len = 0;
    }
  }

  g_palette256_ctx.initialized = true;
  return true;
}

bool pngx_palette256_finalize(const uint8_t *indices, size_t indices_len, const cpres_rgba_color_t *palette, size_t palette_len, uint8_t **out_data, size_t *out_size) {
  uint8_t *mutable_indices;
  cpres_rgba_color_t *mutable_palette;
  bool success;

  if (!g_palette256_ctx.initialized) {
    return false;
  }

  if (!indices || indices_len == 0 || !palette || palette_len == 0 || palette_len > 256 || !out_data || !out_size) {
    rgba_image_reset(&g_palette256_ctx.image);
    quant_support_reset(&g_palette256_ctx.support);
    memset(&g_palette256_ctx, 0, sizeof(g_palette256_ctx));

    return false;
  }

  if (indices_len != g_palette256_ctx.image.pixel_count) {
    rgba_image_reset(&g_palette256_ctx.image);
    quant_support_reset(&g_palette256_ctx.support);
    memset(&g_palette256_ctx, 0, sizeof(g_palette256_ctx));

    return false;
  }

  mutable_palette = (cpres_rgba_color_t *)malloc(sizeof(cpres_rgba_color_t) * palette_len);
  if (!mutable_palette) {
    rgba_image_reset(&g_palette256_ctx.image);
    quant_support_reset(&g_palette256_ctx.support);
    memset(&g_palette256_ctx, 0, sizeof(g_palette256_ctx));

    return false;
  }
  memcpy(mutable_palette, palette, sizeof(cpres_rgba_color_t) * palette_len);
  sanitize_transparent_palette(mutable_palette, palette_len);

  mutable_indices = (uint8_t *)malloc(indices_len);
  if (!mutable_indices) {
    free(mutable_palette);
    rgba_image_reset(&g_palette256_ctx.image);
    quant_support_reset(&g_palette256_ctx.support);
    memset(&g_palette256_ctx, 0, sizeof(g_palette256_ctx));
    return false;
  }

  memcpy(mutable_indices, indices, indices_len);

  postprocess_indices(mutable_indices, g_palette256_ctx.image.width, g_palette256_ctx.image.height, mutable_palette, palette_len, &g_palette256_ctx.support, &g_palette256_ctx.tuned_opts);

  success = pngx_create_palette_png(mutable_indices, indices_len, mutable_palette, palette_len, g_palette256_ctx.image.width, g_palette256_ctx.image.height, out_data, out_size);

  free(mutable_indices);
  free(mutable_palette);

  rgba_image_reset(&g_palette256_ctx.image);
  quant_support_reset(&g_palette256_ctx.support);
  memset(&g_palette256_ctx, 0, sizeof(g_palette256_ctx));

  return success;
}

void pngx_palette256_cleanup(void) {
  if (g_palette256_ctx.initialized) {
    rgba_image_reset(&g_palette256_ctx.image);
    quant_support_reset(&g_palette256_ctx.support);
    memset(&g_palette256_ctx, 0, sizeof(g_palette256_ctx));
  }
}
