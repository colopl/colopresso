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

#include "internal/log.h"
#include "internal/pngx_common.h"

static inline uint8_t lossy_type_bits(uint8_t lossy_type) {
  switch (lossy_type) {
  case PNGX_LOSSY_TYPE_LIMITED_RGBA4444:
    return PNGX_LIMITED_RGBA4444_BITS;
  default:
    return PNGX_FULL_CHANNEL_BITS;
  }
}

static inline void process_bitdepth_pixel(uint8_t *rgba, png_uint_32 width, png_uint_32 height, uint32_t x, uint32_t y, uint8_t bits_per_channel, float dither_level, float *err_curr, float *err_next,
                                          bool left_to_right) {
  uint8_t channel, quantized;
  size_t pixel_index = ((size_t)y * (size_t)width + (size_t)x) * PNGX_RGBA_CHANNELS, err_index = (size_t)x * PNGX_RGBA_CHANNELS;
  float value, error;

  for (channel = 0; channel < PNGX_RGBA_CHANNELS; ++channel) {
    value = (float)rgba[pixel_index + channel] + err_curr[err_index + channel];
    quantized = quantize_channel_value(value, bits_per_channel);
    error = (value - (float)quantized) * dither_level;

    rgba[pixel_index + channel] = quantized;

    if (dither_level <= 0.0f || error == 0.0f) {
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

static inline void reduce_rgba_bitdepth_dither(uint8_t *rgba, png_uint_32 width, png_uint_32 height, uint8_t bits_per_channel, float dither_level) {
  uint32_t x, y;
  size_t row_stride;
  float *err_curr, *err_next, *tmp;
  bool left_to_right;

  if (!rgba || width == 0 || height == 0 || bits_per_channel >= PNGX_FULL_CHANNEL_BITS) {
    return;
  }

  if (dither_level <= 0.0f) {
    snap_rgba_image_to_bits(rgba, (size_t)width * (size_t)height, bits_per_channel, bits_per_channel);
    return;
  }

  row_stride = (size_t)width * PNGX_RGBA_CHANNELS;
  err_curr = (float *)calloc(row_stride, sizeof(float));
  err_next = (float *)calloc(row_stride, sizeof(float));
  if (!err_curr || !err_next) {
    free(err_curr);
    free(err_next);
    snap_rgba_image_to_bits(rgba, (size_t)width * (size_t)height, bits_per_channel, bits_per_channel);
    return;
  }

  for (y = 0; y < height; ++y) {
    left_to_right = ((y & 1) == 0);
    memset(err_next, 0, row_stride * sizeof(float));
    if (left_to_right) {
      for (x = 0; x < width; ++x) {
        process_bitdepth_pixel(rgba, width, height, x, y, bits_per_channel, dither_level, err_curr, err_next, true);
      }
    } else {
      x = width;
      while (x-- > 0) {
        process_bitdepth_pixel(rgba, width, height, x, y, bits_per_channel, dither_level, err_curr, err_next, false);
      }
    }

    tmp = err_curr;
    err_curr = err_next;
    err_next = tmp;
  }

  free(err_curr);
  free(err_next);
}

static inline void reduce_rgba_bitdepth(uint8_t *rgba, png_uint_32 width, png_uint_32 height, uint8_t bits_per_channel, float dither_level) {
  if (!rgba || width == 0 || height == 0) {
    return;
  }

  if (bits_per_channel >= PNGX_FULL_CHANNEL_BITS) {
    return;
  }

  if (dither_level > 0.0f) {
    reduce_rgba_bitdepth_dither(rgba, width, height, bits_per_channel, dither_level);
  } else {
    snap_rgba_image_to_bits(rgba, (size_t)width * (size_t)height, bits_per_channel, bits_per_channel);
  }
}

bool pngx_quantize_limited4444(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size) {
  pngx_rgba_image_t image;
  float resolved_dither;
  bool success;

  if (!png_data || png_size == 0 || !opts || !out_data || !out_size) {
    return false;
  }

  if (!load_rgba_image(png_data, png_size, &image)) {
    return false;
  }

  if (opts->lossy_dither_auto) {
    resolved_dither = estimate_bitdepth_dither_level(image.rgba, image.width, image.height, lossy_type_bits(opts->lossy_type));
  } else {
    resolved_dither = clamp_float(opts->lossy_dither_level, 0.0f, 1.0f);
  }

  reduce_rgba_bitdepth(image.rgba, image.width, image.height, lossy_type_bits(opts->lossy_type), resolved_dither);

  success = create_rgba_png(image.rgba, image.pixel_count, image.width, image.height, out_data, out_size);
  rgba_image_reset(&image);

  if (success) {
    const char *label = lossy_type_label(opts->lossy_type);
    if (opts->lossy_dither_auto) {
      cpres_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Auto dither %.2f selected for %s", resolved_dither, label);
    } else {
      cpres_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Manual dither %.2f used for %s", resolved_dither, label);
    }
  }

  return success;
}
