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

#ifdef __EMSCRIPTEN__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <colopresso.h>

#include <emscripten.h>

#include "internal/avif.h"
#include "internal/pngx.h"
#include "internal/webp.h"

static cpres_error_t g_last_error = CPRES_OK;

EMSCRIPTEN_KEEPALIVE
cpres_config_t *emscripten_config_create(void) {
  cpres_config_t *config;

  config = (cpres_config_t *)malloc(sizeof(cpres_config_t));
  if (config) {
    cpres_config_init_defaults(config);
  }
  return config;
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_free(cpres_config_t *config) {
  if (config) {
    free(config);
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_quality(cpres_config_t *config, float quality) {
  if (config) {
    config->webp_quality = quality;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_lossless(cpres_config_t *config, int lossless) {
  if (config) {
    config->webp_lossless = lossless ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_method(cpres_config_t *config, int method) {
  if (config) {
    config->webp_method = method;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_target_size(cpres_config_t *config, int target_size) {
  if (config) {
    config->webp_target_size = target_size;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_target_psnr(cpres_config_t *config, float target_psnr) {
  if (config) {
    config->webp_target_psnr = target_psnr;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_segments(cpres_config_t *config, int segments) {
  if (config) {
    config->webp_segments = segments;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_sns_strength(cpres_config_t *config, int sns_strength) {
  if (config) {
    config->webp_sns_strength = sns_strength;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_filter_strength(cpres_config_t *config, int filter_strength) {
  if (config) {
    config->webp_filter_strength = filter_strength;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_filter_sharpness(cpres_config_t *config, int filter_sharpness) {
  if (config) {
    config->webp_filter_sharpness = filter_sharpness;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_filter_type(cpres_config_t *config, int filter_type) {
  if (config) {
    config->webp_filter_type = filter_type;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_autofilter(cpres_config_t *config, int autofilter) {
  if (config) {
    config->webp_autofilter = autofilter ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_alpha_compression(cpres_config_t *config, int alpha_compression) {
  if (config) {
    config->webp_alpha_compression = alpha_compression ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_alpha_filtering(cpres_config_t *config, int alpha_filtering) {
  if (config) {
    config->webp_alpha_filtering = alpha_filtering;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_alpha_quality(cpres_config_t *config, int alpha_quality) {
  if (config) {
    config->webp_alpha_quality = alpha_quality;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_pass(cpres_config_t *config, int pass) {
  if (config) {
    config->webp_pass = pass;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_preprocessing(cpres_config_t *config, int preprocessing) {
  if (config) {
    config->webp_preprocessing = preprocessing;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_partitions(cpres_config_t *config, int partitions) {
  if (config) {
    config->webp_partitions = partitions;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_partition_limit(cpres_config_t *config, int partition_limit) {
  if (config) {
    config->webp_partition_limit = partition_limit;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_emulate_jpeg_size(cpres_config_t *config, int emulate_jpeg_size) {
  if (config) {
    config->webp_emulate_jpeg_size = emulate_jpeg_size ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_low_memory(cpres_config_t *config, int low_memory) {
  if (config) {
    config->webp_low_memory = low_memory ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_near_lossless(cpres_config_t *config, int near_lossless) {
  if (config) {
    config->webp_near_lossless = near_lossless;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_exact(cpres_config_t *config, int exact) {
  if (config) {
    config->webp_exact = exact ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_use_delta_palette(cpres_config_t *config, int use_delta_palette) {
  if (config) {
    config->webp_use_delta_palette = use_delta_palette ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_webp_use_sharp_yuv(cpres_config_t *config, int use_sharp_yuv) {
  if (config) {
    config->webp_use_sharp_yuv = use_sharp_yuv ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_avif_quality(cpres_config_t *config, float quality) {
  if (config) {
    config->avif_quality = quality;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_avif_alpha_quality(cpres_config_t *config, int alpha_quality) {
  if (config) {
    config->avif_alpha_quality = alpha_quality;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_avif_lossless(cpres_config_t *config, int lossless) {
  if (config) {
    config->avif_lossless = lossless ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_avif_speed(cpres_config_t *config, int speed) {
  if (config) {
    config->avif_speed = speed;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_level(cpres_config_t *config, int level) {
  if (config) {
    config->pngx_level = level;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_strip_safe(cpres_config_t *config, int strip_safe) {
  if (config) {
    config->pngx_strip_safe = strip_safe ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_optimize_alpha(cpres_config_t *config, int optimize_alpha) {
  if (config) {
    config->pngx_optimize_alpha = optimize_alpha ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_lossy_enable(cpres_config_t *config, int lossy_enable) {
  if (config) {
    config->pngx_lossy_enable = lossy_enable ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_lossy_type(cpres_config_t *config, int lossy_type) {
  if (config) {
    config->pngx_lossy_type = lossy_type;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_lossy_max_colors(cpres_config_t *config, int max_colors) {
  if (config) {
    config->pngx_lossy_max_colors = max_colors;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_reduced_colors(cpres_config_t *config, int reduced_colors) {
  if (config) {
    config->pngx_lossy_reduced_colors = reduced_colors;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_reduced_bits_rgb(cpres_config_t *config, int bits) {
  if (config) {
    config->pngx_lossy_reduced_bits_rgb = bits;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_reduced_alpha_bits(cpres_config_t *config, int bits) {
  if (config) {
    config->pngx_lossy_reduced_alpha_bits = bits;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_lossy_quality_min(cpres_config_t *config, int quality_min) {
  if (config) {
    config->pngx_lossy_quality_min = quality_min;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_lossy_quality_max(cpres_config_t *config, int quality_max) {
  if (config) {
    config->pngx_lossy_quality_max = quality_max;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_lossy_speed(cpres_config_t *config, int speed) {
  if (config) {
    config->pngx_lossy_speed = speed;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_lossy_dither_level(cpres_config_t *config, float dither_level) {
  if (config) {
    config->pngx_lossy_dither_level = dither_level;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_protected_colors(cpres_config_t *config, cpres_rgba_color_t *colors, int count) {
  if (config) {
    config->pngx_protected_colors = colors;
    config->pngx_protected_colors_count = count;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_saliency_map_enable(cpres_config_t *config, int enabled) {
  if (config) {
    config->pngx_saliency_map_enable = enabled ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_chroma_anchor_enable(cpres_config_t *config, int enabled) {
  if (config) {
    config->pngx_chroma_anchor_enable = enabled ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_adaptive_dither_enable(cpres_config_t *config, int enabled) {
  if (config) {
    config->pngx_adaptive_dither_enable = enabled ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_gradient_boost_enable(cpres_config_t *config, int enabled) {
  if (config) {
    config->pngx_gradient_boost_enable = enabled ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_chroma_weight_enable(cpres_config_t *config, int enabled) {
  if (config) {
    config->pngx_chroma_weight_enable = enabled ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_postprocess_smooth_enable(cpres_config_t *config, int enabled) {
  if (config) {
    config->pngx_postprocess_smooth_enable = enabled ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_postprocess_smooth_importance_cutoff(cpres_config_t *config, float cutoff) {
  if (!config) {
    return;
  }
  if (cutoff < 0.0f) {
    config->pngx_postprocess_smooth_importance_cutoff = -1.0f;
  } else if (cutoff <= 1.0f) {
    config->pngx_postprocess_smooth_importance_cutoff = cutoff;
  } else {
    config->pngx_postprocess_smooth_importance_cutoff = 1.0f;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_gradient_profile_enable(cpres_config_t *config, int enabled) {
  if (config) {
    config->pngx_palette256_gradient_profile_enable = enabled ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_gradient_dither_floor(cpres_config_t *config, float dither_floor) {
  if (!config) {
    return;
  }
  if (dither_floor < 0.0f) {
    config->pngx_palette256_gradient_dither_floor = -1.0f;
  } else if (dither_floor <= 1.0f) {
    config->pngx_palette256_gradient_dither_floor = dither_floor;
  } else {
    config->pngx_palette256_gradient_dither_floor = 1.0f;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_alpha_bleed_enable(cpres_config_t *config, int enabled) {
  if (config) {
    config->pngx_palette256_alpha_bleed_enable = enabled ? true : false;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_alpha_bleed_max_distance(cpres_config_t *config, int max_distance) {
  if (!config) {
    return;
  }
  if (max_distance < 0) {
    max_distance = 0;
  } else if (max_distance > 65535) {
    max_distance = 65535;
  }
  config->pngx_palette256_alpha_bleed_max_distance = max_distance;
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_alpha_bleed_opaque_threshold(cpres_config_t *config, int opaque_threshold) {
  if (!config) {
    return;
  }
  if (opaque_threshold < 0) {
    opaque_threshold = 0;
  } else if (opaque_threshold > 255) {
    opaque_threshold = 255;
  }
  config->pngx_palette256_alpha_bleed_opaque_threshold = opaque_threshold;
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_alpha_bleed_soft_limit(cpres_config_t *config, int soft_limit) {
  if (!config) {
    return;
  }
  if (soft_limit < 0) {
    soft_limit = 0;
  } else if (soft_limit > 255) {
    soft_limit = 255;
  }
  config->pngx_palette256_alpha_bleed_soft_limit = soft_limit;
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_profile_opaque_ratio_threshold(cpres_config_t *config, float threshold) {
  if (!config) {
    return;
  }
  if (threshold < 0.0f) {
    config->pngx_palette256_profile_opaque_ratio_threshold = -1.0f;
  } else if (threshold <= 1.0f) {
    config->pngx_palette256_profile_opaque_ratio_threshold = threshold;
  } else {
    config->pngx_palette256_profile_opaque_ratio_threshold = 1.0f;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_profile_gradient_mean_max(cpres_config_t *config, float value) {
  if (!config) {
    return;
  }
  if (value < 0.0f) {
    config->pngx_palette256_profile_gradient_mean_max = -1.0f;
  } else if (value <= 1.0f) {
    config->pngx_palette256_profile_gradient_mean_max = value;
  } else {
    config->pngx_palette256_profile_gradient_mean_max = 1.0f;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_profile_saturation_mean_max(cpres_config_t *config, float value) {
  if (!config) {
    return;
  }
  if (value < 0.0f) {
    config->pngx_palette256_profile_saturation_mean_max = -1.0f;
  } else if (value <= 1.0f) {
    config->pngx_palette256_profile_saturation_mean_max = value;
  } else {
    config->pngx_palette256_profile_saturation_mean_max = 1.0f;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_tune_opaque_ratio_threshold(cpres_config_t *config, float threshold) {
  if (!config) {
    return;
  }
  if (threshold < 0.0f) {
    config->pngx_palette256_tune_opaque_ratio_threshold = -1.0f;
  } else if (threshold <= 1.0f) {
    config->pngx_palette256_tune_opaque_ratio_threshold = threshold;
  } else {
    config->pngx_palette256_tune_opaque_ratio_threshold = 1.0f;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_tune_gradient_mean_max(cpres_config_t *config, float value) {
  if (!config) {
    return;
  }
  if (value < 0.0f) {
    config->pngx_palette256_tune_gradient_mean_max = -1.0f;
  } else if (value <= 1.0f) {
    config->pngx_palette256_tune_gradient_mean_max = value;
  } else {
    config->pngx_palette256_tune_gradient_mean_max = 1.0f;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_tune_saturation_mean_max(cpres_config_t *config, float value) {
  if (!config) {
    return;
  }
  if (value < 0.0f) {
    config->pngx_palette256_tune_saturation_mean_max = -1.0f;
  } else if (value <= 1.0f) {
    config->pngx_palette256_tune_saturation_mean_max = value;
  } else {
    config->pngx_palette256_tune_saturation_mean_max = 1.0f;
  }
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_tune_speed_max(cpres_config_t *config, int speed_max) {
  if (!config) {
    return;
  }
  if (speed_max < 0) {
    config->pngx_palette256_tune_speed_max = -1;
    return;
  }
  if (speed_max < 1) {
    speed_max = 1;
  } else if (speed_max > 10) {
    speed_max = 10;
  }
  config->pngx_palette256_tune_speed_max = speed_max;
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_tune_quality_min_floor(cpres_config_t *config, int value) {
  if (!config) {
    return;
  }
  if (value < 0) {
    config->pngx_palette256_tune_quality_min_floor = -1;
    return;
  }
  if (value > 100) {
    value = 100;
  }
  config->pngx_palette256_tune_quality_min_floor = value;
}

EMSCRIPTEN_KEEPALIVE
void emscripten_config_pngx_palette256_tune_quality_max_target(cpres_config_t *config, int value) {
  if (!config) {
    return;
  }
  if (value < 0) {
    config->pngx_palette256_tune_quality_max_target = -1;
    return;
  }
  if (value > 100) {
    value = 100;
  }
  config->pngx_palette256_tune_quality_max_target = value;
}

EMSCRIPTEN_KEEPALIVE
cpres_error_t emscripten_get_last_error(void) { return g_last_error; }

EMSCRIPTEN_KEEPALIVE
int emscripten_get_last_webp_error(void) { return cpres_webp_get_last_error(); }

EMSCRIPTEN_KEEPALIVE
int emscripten_get_last_avif_error(void) { return cpres_avif_get_last_error(); }

EMSCRIPTEN_KEEPALIVE
const char *emscripten_get_error_string(cpres_error_t error) { return cpres_error_string(error); }

EMSCRIPTEN_KEEPALIVE
uint8_t *emscripten_convert_png_to_webp(const uint8_t *png_data, size_t png_size, const cpres_config_t *config, size_t *out_size) {
  cpres_error_t result;
  uint8_t *webp_data = NULL;
  size_t webp_size = 0;

  if (!out_size) {
    g_last_error = CPRES_ERROR_INVALID_PARAMETER;
    return NULL;
  }
  *out_size = 0;

  if (!png_data || !out_size || !config) {
    g_last_error = CPRES_ERROR_INVALID_PARAMETER;
    return NULL;
  }

  result = cpres_encode_webp_memory(png_data, png_size, &webp_data, &webp_size, config);
  g_last_error = result;

  if (result != CPRES_OK) {
    if (result == CPRES_ERROR_OUTPUT_NOT_SMALLER && webp_size > 0) {
      *out_size = (size_t)webp_size;
    }
    return NULL;
  }

  *out_size = webp_size;
  return webp_data;
}

EMSCRIPTEN_KEEPALIVE
uint8_t *emscripten_convert_png_to_avif(const uint8_t *png_data, size_t png_size, const cpres_config_t *config, size_t *out_size) {
  cpres_error_t result;
  uint8_t *avif_data = NULL;
  size_t avif_size = 0;

  if (!out_size) {
    g_last_error = CPRES_ERROR_INVALID_PARAMETER;
    return NULL;
  }
  *out_size = 0;

  if (!png_data || !out_size || !config) {
    g_last_error = CPRES_ERROR_INVALID_PARAMETER;
    return NULL;
  }

  result = cpres_encode_avif_memory(png_data, png_size, &avif_data, &avif_size, config);
  g_last_error = result;

  if (result != CPRES_OK) {
    if (result == CPRES_ERROR_OUTPUT_NOT_SMALLER && avif_size > 0) {
      *out_size = (size_t)avif_size;
    }
    return NULL;
  }

  *out_size = avif_size;
  return avif_data;
}

EMSCRIPTEN_KEEPALIVE
uint8_t *emscripten_convert_png_to_pngx(const uint8_t *png_data, size_t png_size, const cpres_config_t *config, size_t *out_size) {
  cpres_error_t result;
  uint8_t *pngx_data = NULL;
  size_t pngx_size = 0;

  if (!out_size) {
    g_last_error = CPRES_ERROR_INVALID_PARAMETER;
    return NULL;
  }
  *out_size = 0;

  if (!png_data || !out_size || !config) {
    g_last_error = CPRES_ERROR_INVALID_PARAMETER;
    return NULL;
  }

  result = cpres_encode_pngx_memory(png_data, png_size, &pngx_data, &pngx_size, config);
  g_last_error = result;

  if (result != CPRES_OK) {
    if (result == CPRES_ERROR_OUTPUT_NOT_SMALLER && pngx_size > 0) {
      *out_size = (size_t)pngx_size;
    }
    return NULL;
  }

  *out_size = pngx_size;
  return pngx_data;
}

EMSCRIPTEN_KEEPALIVE
uint32_t emscripten_get_version(void) { return cpres_get_version(); }

EMSCRIPTEN_KEEPALIVE
uint32_t emscripten_get_libwebp_version(void) { return cpres_get_libwebp_version(); }

EMSCRIPTEN_KEEPALIVE
uint32_t emscripten_get_libpng_version(void) { return cpres_get_libpng_version(); }

EMSCRIPTEN_KEEPALIVE
uint32_t emscripten_get_libavif_version(void) { return cpres_get_libavif_version(); }

EMSCRIPTEN_KEEPALIVE
uint32_t emscripten_get_pngx_oxipng_version(void) { return cpres_get_pngx_oxipng_version(); }

EMSCRIPTEN_KEEPALIVE
uint32_t emscripten_get_pngx_libimagequant_version(void) { return cpres_get_pngx_libimagequant_version(); }

EMSCRIPTEN_KEEPALIVE
uint32_t emscripten_get_buildtime(void) { return cpres_get_buildtime(); }

#endif /* __EMSCRIPTEN__ */
