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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "internal/log.h"
#include "internal/pngx_common.h"

static int g_pngx_last_error = 0;

int pngx_get_last_error(void) { return g_pngx_last_error; }

void pngx_set_last_error(int error_code) { g_pngx_last_error = error_code; }

bool pngx_run_quantization(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size, int *quant_quality) {
  const char *label;
  uint32_t resolved_colors = 0;
  uint32_t applied_colors = 0;
  bool success;

  if (!png_data || png_size == 0 || !opts || !out_data || !out_size) {
    return false;
  }

  *out_data = NULL;
  *out_size = 0;
  if (quant_quality) {
    *quant_quality = -1;
  }

  if (opts->lossy_type == PNGX_LOSSY_TYPE_REDUCED_RGBA32) {
    success = pngx_quantize_reduced_rgba32(png_data, png_size, opts, &resolved_colors, &applied_colors, out_data, out_size);
    if (success) {
      label = lossy_type_label(opts->lossy_type);
      cpres_log(CPRES_LOG_LEVEL_DEBUG, "PNGX: %s target %u colors -> %u unique", label, resolved_colors, applied_colors);
    }
    return success;
  }

  if (opts->lossy_type == PNGX_LOSSY_TYPE_LIMITED_RGBA4444) {
    return pngx_quantize_limited4444(png_data, png_size, opts, out_data, out_size);
  }

  return pngx_quantize_palette256(png_data, png_size, opts, out_data, out_size, quant_quality);
}

void pngx_fill_pngx_options(pngx_options_t *opts, const cpres_config_t *config) {
  uint16_t lossy_max_colors = COLOPRESSO_PNGX_DEFAULT_LOSSY_MAX_COLORS;
  uint8_t level = COLOPRESSO_PNGX_DEFAULT_LEVEL, lossy_quality_min = COLOPRESSO_PNGX_DEFAULT_LOSSY_QUALITY_MIN, lossy_quality_max = COLOPRESSO_PNGX_DEFAULT_LOSSY_QUALITY_MAX,
          lossy_speed = COLOPRESSO_PNGX_DEFAULT_LOSSY_SPEED, lossy_type = COLOPRESSO_PNGX_DEFAULT_LOSSY_TYPE, lossy_reduced_bits_rgb = COLOPRESSO_PNGX_DEFAULT_REDUCED_BITS_RGB,
          lossy_reduced_bits_alpha = COLOPRESSO_PNGX_DEFAULT_REDUCED_ALPHA_BITS, tmp;
  int32_t lossy_reduced_colors = COLOPRESSO_PNGX_DEFAULT_REDUCED_COLORS, clamped;
  bool strip_safe = COLOPRESSO_PNGX_DEFAULT_STRIP_SAFE, optimize_alpha = COLOPRESSO_PNGX_DEFAULT_OPTIMIZE_ALPHA, lossy_enable = COLOPRESSO_PNGX_DEFAULT_LOSSY_ENABLE, lossy_dither_auto = false,
       saliency_map_enable = COLOPRESSO_PNGX_DEFAULT_SALIENCY_MAP_ENABLE, chroma_anchor_enable = COLOPRESSO_PNGX_DEFAULT_CHROMA_ANCHOR_ENABLE,
       adaptive_dither_enable = COLOPRESSO_PNGX_DEFAULT_ADAPTIVE_DITHER_ENABLE, gradient_boost_enable = COLOPRESSO_PNGX_DEFAULT_GRADIENT_BOOST_ENABLE,
       chroma_weight_enable = COLOPRESSO_PNGX_DEFAULT_CHROMA_WEIGHT_ENABLE, postprocess_smooth_enable = COLOPRESSO_PNGX_DEFAULT_POSTPROCESS_SMOOTH_ENABLE;
  float lossy_dither_level = COLOPRESSO_PNGX_DEFAULT_LOSSY_DITHER_LEVEL, postprocess_smooth_importance_cutoff = COLOPRESSO_PNGX_DEFAULT_POSTPROCESS_SMOOTH_IMPORTANCE_CUTOFF;

  if (!opts) {
    return;
  }

  if (config) {
    if (config->pngx_level >= 0 && config->pngx_level <= 6) {
      level = (uint8_t)config->pngx_level;
    }
    strip_safe = config->pngx_strip_safe;
    optimize_alpha = config->pngx_optimize_alpha;
    lossy_enable = config->pngx_lossy_enable;
    if (config->pngx_lossy_type >= CPRES_PNGX_LOSSY_TYPE_PALETTE256 && config->pngx_lossy_type <= CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32) {
      lossy_type = (uint8_t)config->pngx_lossy_type;
    }

    if (config->pngx_lossy_max_colors > 0 && config->pngx_lossy_max_colors <= 256) {
      lossy_max_colors = (uint16_t)config->pngx_lossy_max_colors;
    }
    if (config->pngx_lossy_reduced_colors == COLOPRESSO_PNGX_DEFAULT_REDUCED_COLORS) {
      lossy_reduced_colors = COLOPRESSO_PNGX_DEFAULT_REDUCED_COLORS;
    } else if (config->pngx_lossy_reduced_colors >= (int32_t)COLOPRESSO_PNGX_REDUCED_COLORS_MIN) {
      clamped = config->pngx_lossy_reduced_colors;
      if (clamped > (int32_t)COLOPRESSO_PNGX_REDUCED_COLORS_MAX) {
        clamped = (int32_t)COLOPRESSO_PNGX_REDUCED_COLORS_MAX;
      }

      lossy_reduced_colors = clamped;
    }
    if (config->pngx_lossy_reduced_bits_rgb >= COLOPRESSO_PNGX_REDUCED_BITS_MIN && config->pngx_lossy_reduced_bits_rgb <= COLOPRESSO_PNGX_REDUCED_BITS_MAX) {
      lossy_reduced_bits_rgb = (uint8_t)config->pngx_lossy_reduced_bits_rgb;
    }
    if (config->pngx_lossy_reduced_alpha_bits >= COLOPRESSO_PNGX_REDUCED_BITS_MIN && config->pngx_lossy_reduced_alpha_bits <= COLOPRESSO_PNGX_REDUCED_BITS_MAX) {
      lossy_reduced_bits_alpha = (uint8_t)config->pngx_lossy_reduced_alpha_bits;
    }
    if (config->pngx_lossy_quality_min >= 0 && config->pngx_lossy_quality_min <= 100) {
      lossy_quality_min = (uint8_t)config->pngx_lossy_quality_min;
    }
    if (config->pngx_lossy_quality_max >= 0 && config->pngx_lossy_quality_max <= 100) {
      lossy_quality_max = (uint8_t)config->pngx_lossy_quality_max;
    }
    if (lossy_quality_max < lossy_quality_min) {
      tmp = lossy_quality_min;
      lossy_quality_min = lossy_quality_max;
      lossy_quality_max = tmp;
    }
    if (config->pngx_lossy_speed >= 1 && config->pngx_lossy_speed <= 10) {
      lossy_speed = (uint8_t)config->pngx_lossy_speed;
    }
    if (config->pngx_lossy_dither_level < 0.0f) {
      lossy_dither_auto = true;
    } else if (config->pngx_lossy_dither_level <= 1.0f) {
      lossy_dither_level = config->pngx_lossy_dither_level;
    }
    if (config->pngx_protected_colors && config->pngx_protected_colors_count > 0) {
      opts->protected_colors = config->pngx_protected_colors;
      opts->protected_colors_count = config->pngx_protected_colors_count;
    } else {
      opts->protected_colors = NULL;
      opts->protected_colors_count = 0;
    }

    saliency_map_enable = config->pngx_saliency_map_enable;
    chroma_anchor_enable = config->pngx_chroma_anchor_enable;
    adaptive_dither_enable = config->pngx_adaptive_dither_enable;
    gradient_boost_enable = config->pngx_gradient_boost_enable;
    chroma_weight_enable = config->pngx_chroma_weight_enable;
    postprocess_smooth_enable = config->pngx_postprocess_smooth_enable;
    postprocess_smooth_importance_cutoff = config->pngx_postprocess_smooth_importance_cutoff;
  } else {
    opts->protected_colors = NULL;
    opts->protected_colors_count = 0;
  }

  lossy_quality_min = clamp_uint8_t(lossy_quality_min, 0, 100);
  lossy_quality_max = clamp_uint8_t(lossy_quality_max, lossy_quality_min, 100);
  lossy_speed = clamp_uint8_t(lossy_speed, 1, 10);
  lossy_max_colors = clamp_uint16_t(lossy_max_colors, 2, 256);
  lossy_dither_level = clamp_float(lossy_dither_level, 0.0f, 1.0f);
  lossy_reduced_bits_rgb = clamp_reduced_bits(lossy_reduced_bits_rgb);
  lossy_reduced_bits_alpha = clamp_reduced_bits(lossy_reduced_bits_alpha);
  if (postprocess_smooth_importance_cutoff < 0.0f) {
    postprocess_smooth_importance_cutoff = -1.0f;
  } else {
    postprocess_smooth_importance_cutoff = clamp_float(postprocess_smooth_importance_cutoff, 0.0f, 1.0f);
  }

  opts->bridge.optimization_level = level;
  opts->bridge.strip_safe = strip_safe;
  opts->bridge.optimize_alpha = optimize_alpha;
  opts->lossy_enable = lossy_enable;
  opts->lossy_type = lossy_type;
  opts->lossy_max_colors = lossy_max_colors;
  opts->lossy_reduced_colors = lossy_reduced_colors;
  opts->lossy_reduced_bits_rgb = lossy_reduced_bits_rgb;
  opts->lossy_reduced_alpha_bits = lossy_reduced_bits_alpha;
  opts->lossy_quality_min = lossy_quality_min;
  opts->lossy_quality_max = lossy_quality_max;
  opts->lossy_speed = lossy_speed;
  opts->lossy_dither_level = lossy_dither_level;
  opts->lossy_dither_auto = lossy_dither_auto;
  opts->saliency_map_enable = saliency_map_enable;
  opts->chroma_anchor_enable = chroma_anchor_enable;
  opts->adaptive_dither_enable = adaptive_dither_enable;
  opts->gradient_boost_enable = gradient_boost_enable;
  opts->chroma_weight_enable = chroma_weight_enable;
  opts->postprocess_smooth_enable = postprocess_smooth_enable;
  opts->postprocess_smooth_importance_cutoff = postprocess_smooth_importance_cutoff;
}

bool pngx_run_lossless_optimization(const uint8_t *png_data, size_t png_size, const pngx_options_t *opts, uint8_t **out_data, size_t *out_size) {
  PngxBridgeLosslessOptions lossless;
  PngxBridgeResult result;

  if (!png_data || png_size == 0 || !opts || !out_data || !out_size) {
    return false;
  }

  *out_data = NULL;
  *out_size = 0;

  memset(&lossless, 0, sizeof(lossless));
  lossless.optimization_level = opts->bridge.optimization_level;
  lossless.strip_safe = opts->bridge.strip_safe;
  lossless.optimize_alpha = opts->bridge.optimize_alpha;
  result = pngx_bridge_optimize_lossless(png_data, png_size, out_data, out_size, &lossless);
  pngx_set_last_error((int)result);

  if (result != PNGX_BRIDGE_RESULT_SUCCESS) {
    return false;
  }

  return true;
}

bool pngx_should_attempt_quantization(const pngx_options_t *opts) {
  if (!opts) {
    return false;
  }

  return opts->lossy_enable;
}

bool pngx_quantization_better(size_t baseline_size, size_t candidate_size) { return candidate_size == 0 ? false : (baseline_size == 0 ? true : candidate_size < baseline_size); }
