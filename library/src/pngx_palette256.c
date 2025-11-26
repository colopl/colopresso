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

static inline void postprocess_indices(uint8_t *indices, uint32_t width, uint32_t height, const pngx_quant_support_t *support, const pngx_options_t *opts) {
  uint32_t x, y;
  uint8_t base_color, importance, neighbor_colors[4], neighbor_used, *reference;
  size_t pixel_count, idx;
  float cutoff;

  if (!indices || !support || !opts || width == 0 || height == 0) {
    return;
  }

  if (!opts->postprocess_smooth_enable) {
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

      if (cutoff >= 0.0f && ((float)importance / 255.0f) >= cutoff) {
        continue;
      }

      if (x > 0) {
        neighbor_colors[neighbor_used] = reference[idx - 1];
        ++neighbor_used;
      }
      if (x + 1 < width) {
        neighbor_colors[neighbor_used] = reference[idx + 1];
        ++neighbor_used;
      }
      if (y > 0) {
        neighbor_colors[neighbor_used] = reference[idx - (size_t)width];
        ++neighbor_used;
      }
      if (y + 1 < height) {
        neighbor_colors[neighbor_used] = reference[idx + (size_t)width];
        ++neighbor_used;
      }

      if (neighbor_used < 3) {
        continue;
      }

      if (neighbor_used == 3) {
        if (neighbor_colors[0] == neighbor_colors[1] && neighbor_colors[1] == neighbor_colors[2]) {
          if (neighbor_colors[0] != base_color) {
            indices[idx] = neighbor_colors[0];
          }
        }
      } else {
        /* neighbor_used == 4 */
        if (neighbor_colors[0] == neighbor_colors[1]) {
          if (neighbor_colors[0] == neighbor_colors[2] || neighbor_colors[0] == neighbor_colors[3]) {
            if (neighbor_colors[0] != base_color) {
              indices[idx] = neighbor_colors[0];
            }
          }
        } else if (neighbor_colors[0] == neighbor_colors[2]) {
          if (neighbor_colors[0] == neighbor_colors[3]) {
            if (neighbor_colors[0] != base_color) {
              indices[idx] = neighbor_colors[0];
            }
          }
        } else if (neighbor_colors[1] == neighbor_colors[2]) {
          if (neighbor_colors[1] == neighbor_colors[3]) {
            if (neighbor_colors[1] != base_color) {
              indices[idx] = neighbor_colors[1];
            }
          }
        }
      }
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
  params->min_posterization = 0;
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
  size_t required, capacity;
  uint8_t *new_data;

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

static inline bool pngx_create_palette_png(const uint8_t *indices, size_t indices_len, const cpres_rgba_color_t *palette, size_t palette_len, uint32_t width, uint32_t height, uint8_t **out_data,
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

bool create_rgba_png(const uint8_t *rgba, size_t pixel_count, uint32_t width, uint32_t height, uint8_t **out_data, size_t *out_size) {
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
  PngxBridgeQuantParams params, fallback_params;
  PngxBridgeQuantOutput output;
  PngxBridgeQuantStatus status;
  pngx_options_t tuned_opts;
  pngx_quant_support_t support;
  pngx_image_stats_t stats;
  pngx_rgba_image_t image;
  size_t pixel_count;
  float resolved_dither;
  bool relaxed_quality, success;

  if (!png_data || png_size == 0 || !opts || !out_data || !out_size) {
    return false;
  }

  if (quant_quality) {
    *quant_quality = -1;
  }

  if (!load_rgba_image(png_data, png_size, &image)) {
    return false;
  }

  pixel_count = image.pixel_count;
  image_stats_reset(&stats);
  memset(&support, 0, sizeof(support));
  if (!prepare_quant_support(&image, opts, &support, &stats)) {
    rgba_image_reset(&image);
    quant_support_reset(&support);

    return false;
  }

  build_fixed_palette(opts, &support, &tuned_opts);
  resolved_dither = resolve_quant_dither(opts, &stats);

  fill_quant_params(&params, &tuned_opts, support.importance_map, support.importance_map_len);
  params.dithering_level = resolved_dither;
  output.palette = NULL;
  output.palette_len = 0;
  output.indices = NULL;
  output.indices_len = 0;
  output.quality = -1;
  relaxed_quality = false;

  status = pngx_bridge_quantize((const cpres_rgba_color_t *)image.rgba, pixel_count, image.width, image.height, &params, &output);
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

    status = pngx_bridge_quantize((const cpres_rgba_color_t *)image.rgba, pixel_count, image.width, image.height, &fallback_params, &output);
    pngx_set_last_error((int)status);
    if (status == PNGX_BRIDGE_QUANT_STATUS_OK) {
      relaxed_quality = true;
    }
  }

  if (status != PNGX_BRIDGE_QUANT_STATUS_OK) {
    free_quant_output(&output);
    rgba_image_reset(&image);
    quant_support_reset(&support);
    if (status == PNGX_BRIDGE_QUANT_STATUS_QUALITY_TOO_LOW) {
      cpres_log(CPRES_LOG_LEVEL_WARNING, "PNGX: Quantization quality too low");
    }

    return false;
  }

  if (relaxed_quality) {
    cpres_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Relaxed quantization quality floor");
  }

  if (quant_quality) {
    *quant_quality = output.quality;
  }

  success = false;
  if (output.indices && output.indices_len == pixel_count && output.palette && output.palette_len > 0 && output.palette_len <= 256) {
    postprocess_indices(output.indices, image.width, image.height, &support, &tuned_opts);

    success = pngx_create_palette_png(output.indices, output.indices_len, output.palette, output.palette_len, image.width, image.height, out_data, out_size);
  }

  free_quant_output(&output);
  rgba_image_reset(&image);
  quant_support_reset(&support);

  return success;
}
