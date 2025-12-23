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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avif/avif.h>
#include <png.h>
#include <webp/encode.h>

#include <colopresso.h>
#include <colopresso/portable.h>

#include "internal/log.h"
#include "internal/png.h"

#include "internal/avif.h"
#include "internal/pngx.h"
#include "internal/webp.h"

extern void cpres_config_init_defaults(cpres_config_t *config) {
  if (!config) {
    return;
  }

  memset(config, 0, sizeof(*config));

  config->webp_quality = COLOPRESSO_WEBP_DEFAULT_QUALITY;
  config->webp_lossless = COLOPRESSO_WEBP_DEFAULT_LOSSLESS;
  config->webp_method = COLOPRESSO_WEBP_DEFAULT_METHOD;
  config->webp_target_size = COLOPRESSO_WEBP_DEFAULT_TARGET_SIZE;
  config->webp_target_psnr = COLOPRESSO_WEBP_DEFAULT_TARGET_PSNR;
  config->webp_segments = COLOPRESSO_WEBP_DEFAULT_SEGMENTS;
  config->webp_sns_strength = COLOPRESSO_WEBP_DEFAULT_SNS_STRENGTH;
  config->webp_filter_strength = COLOPRESSO_WEBP_DEFAULT_FILTER_STRENGTH;
  config->webp_filter_sharpness = COLOPRESSO_WEBP_DEFAULT_FILTER_SHARPNESS;
  config->webp_filter_type = COLOPRESSO_WEBP_DEFAULT_FILTER_TYPE;
  config->webp_autofilter = COLOPRESSO_WEBP_DEFAULT_AUTOFILTER;
  config->webp_alpha_compression = COLOPRESSO_WEBP_DEFAULT_ALPHA_COMPRESSION;
  config->webp_alpha_filtering = COLOPRESSO_WEBP_DEFAULT_ALPHA_FILTERING;
  config->webp_alpha_quality = COLOPRESSO_WEBP_DEFAULT_ALPHA_QUALITY;
  config->webp_pass = COLOPRESSO_WEBP_DEFAULT_PASS;
  config->webp_preprocessing = COLOPRESSO_WEBP_DEFAULT_PREPROCESSING;
  config->webp_partitions = COLOPRESSO_WEBP_DEFAULT_PARTITIONS;
  config->webp_partition_limit = COLOPRESSO_WEBP_DEFAULT_PARTITION_LIMIT;
  config->webp_emulate_jpeg_size = COLOPRESSO_WEBP_DEFAULT_EMULATE_JPEG_SIZE;
  config->webp_thread_level = COLOPRESSO_WEBP_DEFAULT_THREAD_LEVEL;
  config->webp_low_memory = COLOPRESSO_WEBP_DEFAULT_LOW_MEMORY;
  config->webp_near_lossless = COLOPRESSO_WEBP_DEFAULT_NEAR_LOSSLESS;
  config->webp_exact = COLOPRESSO_WEBP_DEFAULT_EXACT;
  config->webp_use_delta_palette = COLOPRESSO_WEBP_DEFAULT_USE_DELTA_PALETTE;
  config->webp_use_sharp_yuv = COLOPRESSO_WEBP_DEFAULT_USE_SHARP_YUV;

  config->avif_quality = COLOPRESSO_AVIF_DEFAULT_QUALITY;
  config->avif_alpha_quality = COLOPRESSO_AVIF_DEFAULT_ALPHA_QUALITY;
  config->avif_lossless = COLOPRESSO_AVIF_DEFAULT_LOSSLESS;
  config->avif_speed = COLOPRESSO_AVIF_DEFAULT_SPEED;
  config->avif_threads = COLOPRESSO_AVIF_DEFAULT_THREADS;

  config->pngx_level = COLOPRESSO_PNGX_DEFAULT_LEVEL;
  config->pngx_strip_safe = COLOPRESSO_PNGX_DEFAULT_STRIP_SAFE;
  config->pngx_optimize_alpha = COLOPRESSO_PNGX_DEFAULT_OPTIMIZE_ALPHA;
  config->pngx_lossy_enable = COLOPRESSO_PNGX_DEFAULT_LOSSY_ENABLE;
  config->pngx_lossy_type = COLOPRESSO_PNGX_DEFAULT_LOSSY_TYPE;
  config->pngx_lossy_max_colors = COLOPRESSO_PNGX_DEFAULT_LOSSY_MAX_COLORS;
  config->pngx_lossy_reduced_colors = COLOPRESSO_PNGX_DEFAULT_REDUCED_COLORS;
  config->pngx_lossy_reduced_bits_rgb = COLOPRESSO_PNGX_DEFAULT_REDUCED_BITS_RGB;
  config->pngx_lossy_reduced_alpha_bits = COLOPRESSO_PNGX_DEFAULT_REDUCED_ALPHA_BITS;
  config->pngx_lossy_quality_min = COLOPRESSO_PNGX_DEFAULT_LOSSY_QUALITY_MIN;
  config->pngx_lossy_quality_max = COLOPRESSO_PNGX_DEFAULT_LOSSY_QUALITY_MAX;
  config->pngx_lossy_speed = COLOPRESSO_PNGX_DEFAULT_LOSSY_SPEED;
  config->pngx_lossy_dither_level = COLOPRESSO_PNGX_DEFAULT_LOSSY_DITHER_LEVEL;
  config->pngx_saliency_map_enable = COLOPRESSO_PNGX_DEFAULT_SALIENCY_MAP_ENABLE;
  config->pngx_chroma_anchor_enable = COLOPRESSO_PNGX_DEFAULT_CHROMA_ANCHOR_ENABLE;
  config->pngx_adaptive_dither_enable = COLOPRESSO_PNGX_DEFAULT_ADAPTIVE_DITHER_ENABLE;
  config->pngx_gradient_boost_enable = COLOPRESSO_PNGX_DEFAULT_GRADIENT_BOOST_ENABLE;
  config->pngx_chroma_weight_enable = COLOPRESSO_PNGX_DEFAULT_CHROMA_WEIGHT_ENABLE;
  config->pngx_postprocess_smooth_enable = COLOPRESSO_PNGX_DEFAULT_POSTPROCESS_SMOOTH_ENABLE;
  config->pngx_postprocess_smooth_importance_cutoff = COLOPRESSO_PNGX_DEFAULT_POSTPROCESS_SMOOTH_IMPORTANCE_CUTOFF;
  config->pngx_palette256_gradient_profile_enable = COLOPRESSO_PNGX_DEFAULT_PALETTE256_GRADIENT_PROFILE_ENABLE;
  config->pngx_palette256_gradient_dither_floor = COLOPRESSO_PNGX_DEFAULT_PALETTE256_GRADIENT_DITHER_FLOOR;
  config->pngx_palette256_alpha_bleed_enable = COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_ENABLE;
  config->pngx_palette256_alpha_bleed_max_distance = COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_MAX_DISTANCE;
  config->pngx_palette256_alpha_bleed_opaque_threshold = COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_OPAQUE_THRESHOLD;
  config->pngx_palette256_alpha_bleed_soft_limit = COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_SOFT_LIMIT;
  config->pngx_palette256_profile_opaque_ratio_threshold = COLOPRESSO_PNGX_DEFAULT_PALETTE256_PROFILE_OPAQUE_RATIO_THRESHOLD;
  config->pngx_palette256_profile_gradient_mean_max = COLOPRESSO_PNGX_DEFAULT_PALETTE256_PROFILE_GRADIENT_MEAN_MAX;
  config->pngx_palette256_profile_saturation_mean_max = COLOPRESSO_PNGX_DEFAULT_PALETTE256_PROFILE_SATURATION_MEAN_MAX;
  config->pngx_palette256_tune_opaque_ratio_threshold = COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_OPAQUE_RATIO_THRESHOLD;
  config->pngx_palette256_tune_gradient_mean_max = COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_GRADIENT_MEAN_MAX;
  config->pngx_palette256_tune_saturation_mean_max = COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_SATURATION_MEAN_MAX;
  config->pngx_palette256_tune_speed_max = COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_SPEED_MAX;
  config->pngx_palette256_tune_quality_min_floor = COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_QUALITY_MIN_FLOOR;
  config->pngx_palette256_tune_quality_max_target = COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_QUALITY_MAX_TARGET;
  config->pngx_threads = COLOPRESSO_PNGX_DEFAULT_THREADS;
}

extern cpres_error_t cpres_encode_webp_memory(const uint8_t *png_data, size_t png_size, uint8_t **webp_data, size_t *webp_size, const cpres_config_t *config) {
  uint32_t width, height;
  cpres_error_t error;
  uint8_t *rgba_data;
  size_t encoded_size;

  if (!png_data || !webp_data || !webp_size || !config) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  if (png_size == 0) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  if (png_size > COLOPRESSO_PNG_MAX_MEMORY_INPUT_SIZE) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  *webp_data = NULL;
  *webp_size = 0;
  encoded_size = 0;

  rgba_data = NULL;
  error = png_decode_from_memory(png_data, png_size, &rgba_data, &width, &height);
  if (error != CPRES_OK) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "PNG decode from memory failed: %s", cpres_error_string(error));
    return error;
  }

  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNG decoded from memory - %dx%d pixels", width, height);

  error = webp_encode_rgba_to_memory(rgba_data, width, height, webp_data, &encoded_size, config);
  if (error == CPRES_OK) {
    if (*webp_data && encoded_size >= png_size) {
      colopresso_log(CPRES_LOG_LEVEL_WARNING, "WebP: Encoded output larger than input (%zu > %zu)", encoded_size, png_size);
      cpres_free(*webp_data);
      *webp_data = NULL;
      error = CPRES_ERROR_OUTPUT_NOT_SMALLER;
    }
    *webp_size = encoded_size;
  }

  free(rgba_data);

  return error;
}

extern cpres_error_t cpres_encode_avif_memory(const uint8_t *png_data, size_t png_size, uint8_t **avif_data, size_t *avif_size, const cpres_config_t *config) {
  uint32_t width, height;
  uint8_t *rgba_data;
  size_t encoded_size;
  cpres_error_t error;

  if (!png_data || !avif_data || !avif_size || !config) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }
  if (png_size == 0) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  if (png_size > COLOPRESSO_PNG_MAX_MEMORY_INPUT_SIZE) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  *avif_data = NULL;
  *avif_size = 0;
  encoded_size = 0;

  rgba_data = NULL;
  error = png_decode_from_memory(png_data, png_size, &rgba_data, &width, &height);
  if (error != CPRES_OK) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "PNG decode (AVIF) from memory failed: %s", cpres_error_string(error));
    return error;
  }

  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNG decoded (AVIF) from memory - %dx%d pixels", width, height);

  error = avif_encode_rgba_to_memory(rgba_data, width, height, avif_data, &encoded_size, config);
  if (error == CPRES_OK) {
    if (*avif_data && encoded_size >= png_size) {
      colopresso_log(CPRES_LOG_LEVEL_WARNING, "AVIF: Encoded output larger than input (%zu > %zu)", encoded_size, png_size);
      cpres_free(*avif_data);
      *avif_data = NULL;
      error = CPRES_ERROR_OUTPUT_NOT_SMALLER;
    }
    *avif_size = encoded_size;
  }
  free(rgba_data);

  return error;
}

extern cpres_error_t cpres_encode_pngx_memory(const uint8_t *png_data, size_t png_size, uint8_t **optimized_data, size_t *optimized_size, const cpres_config_t *config) {
  pngx_options_t opts;
  uint8_t *lossless_data, *quant_data, *quant_optimized, *final_data;
  size_t lossless_size, quant_size, quant_optimized_size, final_size, candidate_size;
  bool lossless_ok, quant_ok, quant_lossless_ok, quant_is_rgba_lossy, final_is_quantized;
  int quant_quality, threads;

  if (!png_data || png_size == 0 || !optimized_data || !optimized_size || !config) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  if (png_size > COLOPRESSO_PNG_MAX_MEMORY_INPUT_SIZE) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  *optimized_data = NULL;
  *optimized_size = 0;

  lossless_data = NULL;
  lossless_size = 0;
  quant_data = NULL;
  quant_size = 0;
  quant_optimized = NULL;
  quant_optimized_size = 0;
  final_data = NULL;
  final_size = 0;
  candidate_size = 0;
  lossless_ok = false;
  quant_ok = false;
  quant_lossless_ok = false;
  quant_is_rgba_lossy = false;
  final_is_quantized = false;
  quant_quality = -1;
  threads = 0;

  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Starting optimization - input size: %zu bytes", png_size);

  pngx_fill_pngx_options(&opts, config);

  threads = config ? config->pngx_threads : 0;
#if !defined(PNGX_BRIDGE_WASM_SEPARATION)
  if (threads >= 0) {
    pngx_bridge_init_threads(threads);
  }
#else
  (void)threads;
#endif

  quant_is_rgba_lossy = (opts.lossy_type == PNGX_LOSSY_TYPE_LIMITED_RGBA4444 || opts.lossy_type == PNGX_LOSSY_TYPE_REDUCED_RGBA32);

  quant_ok = pngx_should_attempt_quantization(&opts) && pngx_run_quantization(png_data, png_size, &opts, &quant_data, &quant_size, &quant_quality);
  if (quant_ok) {
    colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Quantization produced %zu bytes (quality=%d)", quant_size, quant_quality);
  }

  lossless_ok = pngx_run_lossless_optimization(png_data, png_size, &opts, &lossless_data, &lossless_size);
  if (!lossless_ok) {
    lossless_data = (uint8_t *)malloc(png_size);
    if (!lossless_data) {
      free(quant_data);
      return CPRES_ERROR_OUT_OF_MEMORY;
    }
    memcpy(lossless_data, png_data, png_size);
    lossless_size = png_size;
  }
  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Lossless optimization produced %zu bytes", lossless_size);

  final_data = lossless_data;
  final_size = lossless_size;
  final_is_quantized = false;

  if (quant_ok) {
    if (!quant_is_rgba_lossy) {
      quant_lossless_ok = pngx_run_lossless_optimization(quant_data, quant_size, &opts, &quant_optimized, &quant_optimized_size);
      if (quant_lossless_ok) {
        colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Lossless optimization on quantized data produced %zu bytes", quant_optimized_size);
        free(quant_data);
        quant_data = NULL;
      } else {
        quant_optimized = quant_data;
        quant_optimized_size = quant_size;
        quant_data = NULL;
      }
    } else {
      quant_optimized = quant_data;
      quant_optimized_size = quant_size;
      quant_data = NULL;
    }
  }

  if (quant_ok) {
    candidate_size = quant_optimized_size;
    if (quant_is_rgba_lossy || pngx_quantization_better(lossless_size, candidate_size)) {
      final_data = quant_optimized;
      final_size = candidate_size;
      final_is_quantized = true;
      free(lossless_data);
      colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Selected quantized result (%zu bytes)", final_size);
    } else {
      free(quant_optimized);
      quant_optimized = NULL;
      colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: Selected lossless result (%zu bytes)", final_size);
    }
  }

  if (!final_data) {
    return CPRES_ERROR_ENCODE_FAILED;
  }

  if (final_size >= png_size) {
    if (!(quant_is_rgba_lossy && final_is_quantized)) {
      colopresso_log(CPRES_LOG_LEVEL_WARNING, "PNGX: Optimized output larger than input (%zu > %zu)", final_size, png_size);
      free(final_data);
      *optimized_data = NULL;
      *optimized_size = final_size;
      return CPRES_ERROR_OUTPUT_NOT_SMALLER;
    }

    colopresso_log(CPRES_LOG_LEVEL_WARNING, "PNGX: RGBA lossy output larger than input (%zu > %zu) but forcing write per RGBA mode", final_size, png_size);
  }

  *optimized_data = final_data;
  *optimized_size = final_size;

  return CPRES_OK;
}

extern void cpres_free(uint8_t *data) {
  if (data) {
    free(data);
  }
}

extern const char *cpres_error_string(cpres_error_t error) {
  switch (error) {
  case CPRES_OK:
    return "Success";
  case CPRES_ERROR_FILE_NOT_FOUND:
    return "File not found";
  case CPRES_ERROR_INVALID_PNG:
    return "Invalid PNG file";
  case CPRES_ERROR_INVALID_FORMAT:
    return "Invalid WebP file";
  case CPRES_ERROR_OUT_OF_MEMORY:
    return "Out of memory";
  case CPRES_ERROR_ENCODE_FAILED:
    return "Encoding failed";
  case CPRES_ERROR_DECODE_FAILED:
    return "Decoding failed";
  case CPRES_ERROR_IO:
    return "I/O error";
  case CPRES_ERROR_INVALID_PARAMETER:
    return "Invalid parameter";
  case CPRES_ERROR_OUTPUT_NOT_SMALLER:
    return "Output image would be larger than input";
  default:
    return "Unknown error";
  }
}

extern uint32_t cpres_get_version(void) { return (uint32_t)COLOPRESSO_VERSION; }

extern uint32_t cpres_get_libwebp_version(void) { return (uint32_t)WebPGetEncoderVersion(); }

extern uint32_t cpres_get_libpng_version(void) { return (uint32_t)png_access_version_number(); }

extern uint32_t cpres_get_libavif_version(void) { return (uint32_t)AVIF_VERSION; }

extern uint32_t cpres_get_pngx_oxipng_version(void) {
#if !defined(PNGX_BRIDGE_WASM_SEPARATION)
  return pngx_bridge_oxipng_version();
#else
  return 0;
#endif
}

extern uint32_t cpres_get_pngx_libimagequant_version(void) {
#if !defined(PNGX_BRIDGE_WASM_SEPARATION)
  return pngx_bridge_libimagequant_version();
#else
  return 0;
#endif
}

extern uint32_t cpres_get_buildtime(void) {
#ifdef COLOPRESSO_BUILDTIME
  return (uint32_t)COLOPRESSO_BUILDTIME;
#else
  return 0;
#endif
}
