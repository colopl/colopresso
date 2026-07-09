/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2025-2026 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

#include <node_api.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <colopresso.h>

#define COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT "invalid_argument"
#define COLOPRESSO_NATIVE_ERROR_CONVERSION_FAILED "conversion_failed"
#define COLOPRESSO_NATIVE_ERROR_OUTPUT_NOT_SMALLER "output_larger_than_input"

typedef struct {
  napi_env env;
  napi_async_work async_work;
  napi_deferred deferred;
  char format_id[16];
  uint8_t *input_data;
  size_t input_size;
  uint8_t *output_data;
  size_t output_size;
  cpres_config_t config;
  cpres_rgba_color_t *protected_colors;
  int requested_threads;
  bool has_requested_threads;
  cpres_error_t error;
  char error_code[64];
  char error_message[256];
} colopresso_convert_work_t;

static int clamp_int(int value, int min_value, int max_value) {
  if (value < min_value) {
    return min_value;
  }
  if (value > max_value) {
    return max_value;
  }
  return value;
}

static float clamp_float_value(double value, float min_value, float max_value) {
  if (value < (double)min_value) {
    return min_value;
  }
  if (value > (double)max_value) {
    return max_value;
  }
  return (float)value;
}

static void set_work_error(colopresso_convert_work_t *work, const char *code, const char *message) {
  if (!work) {
    return;
  }

  if (code) {
    snprintf(work->error_code, sizeof(work->error_code), "%s", code);
  }
  if (message) {
    snprintf(work->error_message, sizeof(work->error_message), "%s", message);
  }
}

static napi_value throw_type_error(napi_env env, const char *message) {
  napi_throw_type_error(env, COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT, message);
  return NULL;
}

static bool get_named_property(napi_env env, napi_value object, const char *name, napi_value *out_value) {
  bool has_property;
  napi_valuetype value_type;

  if (!object || !name || !out_value) {
    return false;
  }

  has_property = false;
  if (napi_has_named_property(env, object, name, &has_property) != napi_ok || !has_property) {
    return false;
  }
  if (napi_get_named_property(env, object, name, out_value) != napi_ok) {
    return false;
  }
  if (napi_typeof(env, *out_value, &value_type) != napi_ok) {
    return false;
  }

  return value_type != napi_undefined && value_type != napi_null;
}

static bool read_double_property(napi_env env, napi_value object, const char *name, double *out_value) {
  napi_value value;
  napi_valuetype value_type;

  if (!get_named_property(env, object, name, &value)) {
    return false;
  }
  if (napi_typeof(env, value, &value_type) != napi_ok || value_type != napi_number) {
    return false;
  }

  return napi_get_value_double(env, value, out_value) == napi_ok;
}

static bool read_int_property(napi_env env, napi_value object, const char *name, int *out_value) {
  double value;

  if (!read_double_property(env, object, name, &value)) {
    return false;
  }

  *out_value = (int)value;
  return true;
}

static bool read_bool_property(napi_env env, napi_value object, const char *name, bool *out_value) {
  napi_value value;
  napi_valuetype value_type;

  if (!get_named_property(env, object, name, &value)) {
    return false;
  }
  if (napi_typeof(env, value, &value_type) != napi_ok) {
    return false;
  }
  if (value_type == napi_boolean) {
    return napi_get_value_bool(env, value, out_value) == napi_ok;
  }
  if (value_type == napi_number) {
    double numeric_value;
    if (napi_get_value_double(env, value, &numeric_value) != napi_ok) {
      return false;
    }
    *out_value = numeric_value != 0.0;
    return true;
  }

  return false;
}

static void apply_double_property(napi_env env, napi_value options, const char *name, float *target) {
  double value;

  if (read_double_property(env, options, name, &value)) {
    *target = (float)value;
  }
}

static void apply_int_property(napi_env env, napi_value options, const char *name, int *target) {
  int value;

  if (read_int_property(env, options, name, &value)) {
    *target = value;
  }
}

static void apply_bool_property(napi_env env, napi_value options, const char *name, bool *target) {
  bool value;

  if (read_bool_property(env, options, name, &value)) {
    *target = value;
  }
}

static bool read_input_bytes(napi_env env, napi_value value, uint8_t **out_data, size_t *out_size) {
  bool is_buffer, is_typed_array, is_array_buffer;
  void *source_data;
  size_t source_size, typed_array_length, byte_offset;
  napi_typedarray_type typed_array_type;
  napi_value array_buffer;

  if (!out_data || !out_size) {
    return false;
  }

  *out_data = NULL;
  *out_size = 0;
  is_buffer = false;
  is_typed_array = false;
  is_array_buffer = false;
  source_data = NULL;
  source_size = 0;

  if (napi_is_buffer(env, value, &is_buffer) == napi_ok && is_buffer) {
    if (napi_get_buffer_info(env, value, &source_data, &source_size) != napi_ok) {
      return false;
    }
  } else if (napi_is_typedarray(env, value, &is_typed_array) == napi_ok && is_typed_array) {
    if (napi_get_typedarray_info(env, value, &typed_array_type, &typed_array_length, &source_data, &array_buffer, &byte_offset) != napi_ok) {
      return false;
    }
    (void)array_buffer;
    (void)byte_offset;
    if (typed_array_type != napi_uint8_array && typed_array_type != napi_uint8_clamped_array) {
      return false;
    }
    source_size = typed_array_length;
  } else if (napi_is_arraybuffer(env, value, &is_array_buffer) == napi_ok && is_array_buffer) {
    if (napi_get_arraybuffer_info(env, value, &source_data, &source_size) != napi_ok) {
      return false;
    }
  } else {
    return false;
  }

  if (!source_data || source_size == 0 || source_size > COLOPRESSO_PNG_MAX_MEMORY_INPUT_SIZE) {
    return false;
  }

  *out_data = (uint8_t *)malloc(source_size);
  if (!*out_data) {
    return false;
  }
  memcpy(*out_data, source_data, source_size);
  *out_size = source_size;

  return true;
}

static bool apply_protected_colors(napi_env env, napi_value options, colopresso_convert_work_t *work) {
  napi_value colors_value, color_value;
  bool is_array;
  uint32_t colors_length, index;

  if (!get_named_property(env, options, "pngx_protected_colors", &colors_value)) {
    return true;
  }

  is_array = false;
  if (napi_is_array(env, colors_value, &is_array) != napi_ok || !is_array) {
    set_work_error(work, COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT, "pngx_protected_colors must be an array");
    return false;
  }
  if (napi_get_array_length(env, colors_value, &colors_length) != napi_ok) {
    set_work_error(work, COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT, "failed to read pngx_protected_colors");
    return false;
  }
  if (colors_length == 0) {
    return true;
  }
  if (colors_length > 256) {
    set_work_error(work, COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT, "pngx_protected_colors must contain at most 256 colors");
    return false;
  }

  work->protected_colors = (cpres_rgba_color_t *)calloc(colors_length, sizeof(cpres_rgba_color_t));
  if (!work->protected_colors) {
    set_work_error(work, COLOPRESSO_NATIVE_ERROR_CONVERSION_FAILED, "failed to allocate protected color buffer");
    return false;
  }

  for (index = 0; index < colors_length; ++index) {
    int red, green, blue, alpha;

    if (napi_get_element(env, colors_value, index, &color_value) != napi_ok) {
      set_work_error(work, COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT, "failed to read protected color");
      return false;
    }

    red = 0;
    green = 0;
    blue = 0;
    alpha = 255;
    read_int_property(env, color_value, "r", &red);
    read_int_property(env, color_value, "g", &green);
    read_int_property(env, color_value, "b", &blue);
    read_int_property(env, color_value, "a", &alpha);

    work->protected_colors[index].r = (uint8_t)clamp_int(red, 0, 255);
    work->protected_colors[index].g = (uint8_t)clamp_int(green, 0, 255);
    work->protected_colors[index].b = (uint8_t)clamp_int(blue, 0, 255);
    work->protected_colors[index].a = (uint8_t)clamp_int(alpha, 0, 255);
  }

  work->config.pngx_protected_colors = work->protected_colors;
  work->config.pngx_protected_colors_count = (int)colors_length;

  return true;
}

static void apply_webp_options(napi_env env, napi_value options, cpres_config_t *config) {
  apply_double_property(env, options, "quality", &config->webp_quality);
  apply_bool_property(env, options, "lossless", &config->webp_lossless);
  apply_int_property(env, options, "method", &config->webp_method);
  apply_int_property(env, options, "target_size", &config->webp_target_size);
  apply_double_property(env, options, "target_psnr", &config->webp_target_psnr);
  apply_int_property(env, options, "segments", &config->webp_segments);
  apply_int_property(env, options, "sns_strength", &config->webp_sns_strength);
  apply_int_property(env, options, "filter_strength", &config->webp_filter_strength);
  apply_int_property(env, options, "filter_sharpness", &config->webp_filter_sharpness);
  apply_int_property(env, options, "filter_type", &config->webp_filter_type);
  apply_bool_property(env, options, "autofilter", &config->webp_autofilter);
  apply_bool_property(env, options, "alpha_compression", &config->webp_alpha_compression);
  apply_int_property(env, options, "alpha_filtering", &config->webp_alpha_filtering);
  apply_int_property(env, options, "alpha_quality", &config->webp_alpha_quality);
  apply_int_property(env, options, "pass", &config->webp_pass);
  apply_int_property(env, options, "preprocessing", &config->webp_preprocessing);
  apply_int_property(env, options, "partitions", &config->webp_partitions);
  apply_int_property(env, options, "partition_limit", &config->webp_partition_limit);
  apply_bool_property(env, options, "emulate_jpeg_size", &config->webp_emulate_jpeg_size);
  apply_bool_property(env, options, "low_memory", &config->webp_low_memory);
  apply_int_property(env, options, "near_lossless", &config->webp_near_lossless);
  apply_bool_property(env, options, "exact", &config->webp_exact);
  apply_bool_property(env, options, "use_delta_palette", &config->webp_use_delta_palette);
  apply_bool_property(env, options, "use_sharp_yuv", &config->webp_use_sharp_yuv);
}

static void apply_avif_options(napi_env env, napi_value options, cpres_config_t *config) {
  double value;
  bool bool_value;

  if (read_double_property(env, options, "avif_quality", &value) || read_double_property(env, options, "quality", &value)) {
    config->avif_quality = (float)value;
  }
  if (read_int_property(env, options, "avif_alpha_quality", &config->avif_alpha_quality) || read_int_property(env, options, "alpha_quality", &config->avif_alpha_quality)) {
    /* NOP */
  }
  if (read_bool_property(env, options, "avif_lossless", &bool_value) || read_bool_property(env, options, "lossless", &bool_value)) {
    config->avif_lossless = bool_value;
  }
  if (read_int_property(env, options, "avif_speed", &config->avif_speed) || read_int_property(env, options, "speed", &config->avif_speed)) {
    /* NOP */
  }
}

static void apply_pngx_options(napi_env env, napi_value options, cpres_config_t *config) {
  int lossy_type, shared_max_colors, legacy_reduced_colors, resolved_reduced_colors, raw_dither_level;
  bool has_lossy_type, has_shared_max_colors, has_legacy_reduced_colors, dither_auto_selected;
  double double_value;

  if (read_int_property(env, options, "pngx_level", &config->pngx_level) || read_int_property(env, options, "level", &config->pngx_level)) {
    /* NOP */
  }
  apply_bool_property(env, options, "pngx_strip_safe", &config->pngx_strip_safe);
  apply_bool_property(env, options, "pngx_optimize_alpha", &config->pngx_optimize_alpha);
  apply_bool_property(env, options, "pngx_lossy_enable", &config->pngx_lossy_enable);

  lossy_type = config->pngx_lossy_type;
  shared_max_colors = config->pngx_lossy_max_colors;
  legacy_reduced_colors = config->pngx_lossy_reduced_colors;
  has_lossy_type = read_int_property(env, options, "pngx_lossy_type", &lossy_type);
  has_shared_max_colors = read_int_property(env, options, "pngx_lossy_max_colors", &shared_max_colors);
  has_legacy_reduced_colors = read_int_property(env, options, "pngx_lossy_reduced_colors", &legacy_reduced_colors);

  if (has_lossy_type) {
    config->pngx_lossy_type = lossy_type;
  }
  if (has_shared_max_colors) {
    config->pngx_lossy_max_colors = shared_max_colors;
  }

  resolved_reduced_colors = -1;
  if (lossy_type == CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32) {
    if (has_shared_max_colors && shared_max_colors > 1) {
      resolved_reduced_colors = clamp_int(shared_max_colors, COLOPRESSO_PNGX_REDUCED_COLORS_MIN, COLOPRESSO_PNGX_REDUCED_COLORS_MAX);
    }
  } else if (has_legacy_reduced_colors) {
    resolved_reduced_colors = legacy_reduced_colors;
  }
  config->pngx_lossy_reduced_colors = resolved_reduced_colors;

  apply_int_property(env, options, "pngx_lossy_reduced_bits_rgb", &config->pngx_lossy_reduced_bits_rgb);
  apply_int_property(env, options, "pngx_lossy_reduced_alpha_bits", &config->pngx_lossy_reduced_alpha_bits);
  apply_int_property(env, options, "pngx_lossy_quality_min", &config->pngx_lossy_quality_min);
  apply_int_property(env, options, "pngx_lossy_quality_max", &config->pngx_lossy_quality_max);
  apply_int_property(env, options, "pngx_lossy_speed", &config->pngx_lossy_speed);

  dither_auto_selected = false;
  read_bool_property(env, options, "pngx_lossy_dither_auto", &dither_auto_selected);
  raw_dither_level = 0;
  if (lossy_type == CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444 && dither_auto_selected) {
    config->pngx_lossy_dither_level = -1.0f;
  } else if (read_double_property(env, options, "pngx_lossy_dither_level", &double_value)) {
    if (double_value < 0.0) {
      config->pngx_lossy_dither_level = -1.0f;
    } else if (double_value <= 1.0) {
      config->pngx_lossy_dither_level = clamp_float_value(double_value, 0.0f, 1.0f);
    } else {
      raw_dither_level = clamp_int((int)double_value, 0, 100);
      config->pngx_lossy_dither_level = (float)raw_dither_level / 100.0f;
    }
  }

  apply_bool_property(env, options, "pngx_saliency_map_enable", &config->pngx_saliency_map_enable);
  apply_bool_property(env, options, "pngx_chroma_anchor_enable", &config->pngx_chroma_anchor_enable);
  apply_bool_property(env, options, "pngx_adaptive_dither_enable", &config->pngx_adaptive_dither_enable);
  apply_bool_property(env, options, "pngx_gradient_boost_enable", &config->pngx_gradient_boost_enable);
  apply_bool_property(env, options, "pngx_chroma_weight_enable", &config->pngx_chroma_weight_enable);
  apply_bool_property(env, options, "pngx_postprocess_smooth_enable", &config->pngx_postprocess_smooth_enable);
  apply_double_property(env, options, "pngx_postprocess_smooth_importance_cutoff", &config->pngx_postprocess_smooth_importance_cutoff);
  apply_bool_property(env, options, "pngx_palette256_gradient_profile_enable", &config->pngx_palette256_gradient_profile_enable);
  apply_double_property(env, options, "pngx_palette256_gradient_dither_floor", &config->pngx_palette256_gradient_dither_floor);
  apply_bool_property(env, options, "pngx_palette256_alpha_bleed_enable", &config->pngx_palette256_alpha_bleed_enable);
  apply_int_property(env, options, "pngx_palette256_alpha_bleed_max_distance", &config->pngx_palette256_alpha_bleed_max_distance);
  apply_int_property(env, options, "pngx_palette256_alpha_bleed_opaque_threshold", &config->pngx_palette256_alpha_bleed_opaque_threshold);
  apply_int_property(env, options, "pngx_palette256_alpha_bleed_soft_limit", &config->pngx_palette256_alpha_bleed_soft_limit);
  apply_double_property(env, options, "pngx_palette256_profile_opaque_ratio_threshold", &config->pngx_palette256_profile_opaque_ratio_threshold);
  apply_double_property(env, options, "pngx_palette256_profile_gradient_mean_max", &config->pngx_palette256_profile_gradient_mean_max);
  apply_double_property(env, options, "pngx_palette256_profile_saturation_mean_max", &config->pngx_palette256_profile_saturation_mean_max);
  apply_double_property(env, options, "pngx_palette256_tune_opaque_ratio_threshold", &config->pngx_palette256_tune_opaque_ratio_threshold);
  apply_double_property(env, options, "pngx_palette256_tune_gradient_mean_max", &config->pngx_palette256_tune_gradient_mean_max);
  apply_double_property(env, options, "pngx_palette256_tune_saturation_mean_max", &config->pngx_palette256_tune_saturation_mean_max);
  apply_int_property(env, options, "pngx_palette256_tune_speed_max", &config->pngx_palette256_tune_speed_max);
  apply_int_property(env, options, "pngx_palette256_tune_quality_min_floor", &config->pngx_palette256_tune_quality_min_floor);
  apply_int_property(env, options, "pngx_palette256_tune_quality_max_target", &config->pngx_palette256_tune_quality_max_target);
}

static bool resolve_thread_count(napi_env env, napi_value options, colopresso_convert_work_t *work, int argument_threads, bool has_argument_threads) {
  uint32_t default_threads, max_threads;
  int requested_threads;

  if (!cpres_is_threads_enabled()) {
    work->config.webp_thread_level = 0;
    work->config.avif_threads = 1;
    work->config.pngx_threads = 1;
    work->requested_threads = 1;
    work->has_requested_threads = true;
    return true;
  }

  max_threads = cpres_get_max_thread_count();
  if (max_threads == 0) {
    max_threads = 1;
  }
  default_threads = cpres_get_default_thread_count();
  if (default_threads == 0) {
    default_threads = 1;
  }
  if (default_threads > max_threads) {
    default_threads = max_threads;
  }

  requested_threads = 0;
  if (has_argument_threads) {
    requested_threads = argument_threads;
  } else if (read_int_property(env, options, "conversion_threads", &requested_threads) || read_int_property(env, options, "pngx_threads", &requested_threads)) {
    /* NOP */
  } else {
    requested_threads = 0;
  }

  if (requested_threads < 0) {
    set_work_error(work, COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT, "thread count must be >= 0");
    return false;
  }

  if (requested_threads == 0) {
    requested_threads = (int)default_threads;
  } else if ((uint32_t)requested_threads > max_threads) {
    set_work_error(work, COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT, "thread count exceeds the detected logical thread count");
    return false;
  }

  work->config.webp_thread_level = requested_threads > 1 ? 1 : 0;
  work->config.avif_threads = requested_threads;
  work->config.pngx_threads = requested_threads;
  work->requested_threads = requested_threads;
  work->has_requested_threads = true;

  return true;
}

static bool apply_options(napi_env env, napi_value options, colopresso_convert_work_t *work) {
  napi_valuetype options_type;

  if (!work) {
    return false;
  }

  if (!options || napi_typeof(env, options, &options_type) != napi_ok || options_type == napi_undefined || options_type == napi_null) {
    return true;
  }
  if (options_type != napi_object) {
    set_work_error(work, COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT, "options must be an object");
    return false;
  }

  apply_webp_options(env, options, &work->config);
  apply_avif_options(env, options, &work->config);
  apply_pngx_options(env, options, &work->config);

  return apply_protected_colors(env, options, work);
}

static void execute_convert(napi_env env, void *data) {
  colopresso_convert_work_t *work;

  (void)env;

  work = (colopresso_convert_work_t *)data;
  if (!work) {
    return;
  }

  if (strcmp(work->format_id, "webp") == 0) {
    work->error = cpres_encode_webp_memory(work->input_data, work->input_size, &work->output_data, &work->output_size, &work->config);
  } else if (strcmp(work->format_id, "avif") == 0) {
    work->error = cpres_encode_avif_memory(work->input_data, work->input_size, &work->output_data, &work->output_size, &work->config);
  } else if (strcmp(work->format_id, "pngx") == 0) {
    work->error = cpres_encode_pngx_memory(work->input_data, work->input_size, &work->output_data, &work->output_size, &work->config);
  } else {
    work->error = CPRES_ERROR_INVALID_FORMAT;
  }

  if (work->error != CPRES_OK) {
    const char *message = cpres_error_string(work->error);
    const char *code = work->error == CPRES_ERROR_OUTPUT_NOT_SMALLER ? COLOPRESSO_NATIVE_ERROR_OUTPUT_NOT_SMALLER : COLOPRESSO_NATIVE_ERROR_CONVERSION_FAILED;
    set_work_error(work, code, message ? message : "conversion failed");
  }
}

static void cleanup_convert_work(colopresso_convert_work_t *work) {
  if (!work) {
    return;
  }
  if (work->async_work) {
    napi_delete_async_work(work->env, work->async_work);
  }
  free(work->input_data);
  if (work->output_data) {
    cpres_free(work->output_data);
  }
  free(work->protected_colors);
  free(work);
}

static void reject_convert_work(colopresso_convert_work_t *work) {
  napi_value message_value, error_value, code_value, input_size_value, output_size_value;

  napi_create_string_utf8(work->env, work->error_message[0] ? work->error_message : "conversion failed", NAPI_AUTO_LENGTH, &message_value);
  napi_create_error(work->env, NULL, message_value, &error_value);
  napi_create_string_utf8(work->env, work->error_code[0] ? work->error_code : COLOPRESSO_NATIVE_ERROR_CONVERSION_FAILED, NAPI_AUTO_LENGTH, &code_value);
  napi_set_named_property(work->env, error_value, "code", code_value);
  napi_create_double(work->env, (double)work->input_size, &input_size_value);
  napi_set_named_property(work->env, error_value, "inputSize", input_size_value);
  if (work->output_size > 0) {
    napi_create_double(work->env, (double)work->output_size, &output_size_value);
    napi_set_named_property(work->env, error_value, "outputSize", output_size_value);
  }
  napi_reject_deferred(work->env, work->deferred, error_value);
}

static void complete_convert(napi_env env, napi_status status, void *data) {
  colopresso_convert_work_t *work;
  napi_value result, output_buffer, input_size_value, output_size_value, thread_count_value;

  work = (colopresso_convert_work_t *)data;
  if (!work) {
    return;
  }

  if (status != napi_ok || work->error != CPRES_OK || !work->output_data) {
    if (status != napi_ok && work->error == CPRES_OK) {
      set_work_error(work, COLOPRESSO_NATIVE_ERROR_CONVERSION_FAILED, "native conversion was cancelled");
    }
    reject_convert_work(work);
    cleanup_convert_work(work);
    return;
  }

  napi_create_object(env, &result);
  napi_create_buffer_copy(env, work->output_size, work->output_data, NULL, &output_buffer);
  napi_set_named_property(env, result, "outputBytes", output_buffer);
  napi_create_double(env, (double)work->input_size, &input_size_value);
  napi_set_named_property(env, result, "inputSize", input_size_value);
  napi_create_double(env, (double)work->output_size, &output_size_value);
  napi_set_named_property(env, result, "outputSize", output_size_value);
  napi_create_int32(env, work->requested_threads, &thread_count_value);
  napi_set_named_property(env, result, "threadCount", thread_count_value);

  napi_resolve_deferred(env, work->deferred, result);
  cleanup_convert_work(work);
}

static napi_value get_version_info(napi_env env, napi_callback_info info) {
  napi_value result, value;
  const char *compiler_version, *rust_version;

  (void)info;

  napi_create_object(env, &result);
  napi_create_uint32(env, cpres_get_version(), &value);
  napi_set_named_property(env, result, "version", value);
  napi_create_uint32(env, cpres_get_libwebp_version(), &value);
  napi_set_named_property(env, result, "libwebpVersion", value);
  napi_create_uint32(env, cpres_get_libpng_version(), &value);
  napi_set_named_property(env, result, "libpngVersion", value);
  napi_create_uint32(env, cpres_get_libavif_version(), &value);
  napi_set_named_property(env, result, "libavifVersion", value);
  napi_create_uint32(env, cpres_get_pngx_oxipng_version(), &value);
  napi_set_named_property(env, result, "pngxOxipngVersion", value);
  napi_create_uint32(env, cpres_get_pngx_libimagequant_version(), &value);
  napi_set_named_property(env, result, "pngxLibimagequantVersion", value);
  napi_create_uint32(env, cpres_get_buildtime(), &value);
  napi_set_named_property(env, result, "buildtime", value);

  compiler_version = cpres_get_compiler_version_string();
  napi_create_string_utf8(env, compiler_version ? compiler_version : "unknown", NAPI_AUTO_LENGTH, &value);
  napi_set_named_property(env, result, "compilerVersionString", value);
  rust_version = cpres_get_rust_version_string();
  napi_create_string_utf8(env, rust_version ? rust_version : "unknown", NAPI_AUTO_LENGTH, &value);
  napi_set_named_property(env, result, "rustVersionString", value);

  return result;
}

static napi_value get_thread_info(napi_env env, napi_callback_info info) {
  napi_value result, value;

  (void)info;

  napi_create_object(env, &result);
  napi_get_boolean(env, cpres_is_threads_enabled(), &value);
  napi_set_named_property(env, result, "enabled", value);
  napi_create_uint32(env, cpres_get_default_thread_count(), &value);
  napi_set_named_property(env, result, "defaultThreads", value);
  napi_create_uint32(env, cpres_get_max_thread_count(), &value);
  napi_set_named_property(env, result, "maxThreads", value);

  return result;
}

static napi_value convert(napi_env env, napi_callback_info info) {
  napi_value args[4], promise, resource_name;
  size_t argc, format_length;
  napi_valuetype thread_arg_type;
  colopresso_convert_work_t *work;
  const char *thread_error_message;
  int argument_threads;
  bool has_argument_threads;

  argc = 4;
  if (napi_get_cb_info(env, info, &argc, args, NULL, NULL) != napi_ok || argc < 3) {
    return throw_type_error(env, "convert(formatId, options, inputBytes, threadCount?) requires at least 3 arguments");
  }

  work = (colopresso_convert_work_t *)calloc(1, sizeof(colopresso_convert_work_t));
  if (!work) {
    napi_throw_error(env, COLOPRESSO_NATIVE_ERROR_CONVERSION_FAILED, "failed to allocate conversion work");
    return NULL;
  }

  work->env = env;
  work->requested_threads = 1;
  work->error = CPRES_OK;
  cpres_config_init_defaults(&work->config);

  if (napi_get_value_string_utf8(env, args[0], work->format_id, sizeof(work->format_id), &format_length) != napi_ok || format_length == 0 || format_length >= sizeof(work->format_id)) {
    cleanup_convert_work(work);
    return throw_type_error(env, "formatId must be a supported string");
  }
  if (strcmp(work->format_id, "webp") != 0 && strcmp(work->format_id, "avif") != 0 && strcmp(work->format_id, "pngx") != 0) {
    cleanup_convert_work(work);
    return throw_type_error(env, "formatId must be webp, avif, or pngx");
  }

  if (!read_input_bytes(env, args[2], &work->input_data, &work->input_size)) {
    cleanup_convert_work(work);
    return throw_type_error(env, "inputBytes must be a non-empty Buffer, Uint8Array, or ArrayBuffer within the supported size limit");
  }

  if (!apply_options(env, args[1], work)) {
    napi_value error_message;
    snprintf(work->error_message, sizeof(work->error_message), "%s", work->error_message[0] ? work->error_message : "invalid options");
    napi_create_string_utf8(env, work->error_message, NAPI_AUTO_LENGTH, &error_message);
    cleanup_convert_work(work);
    napi_throw_type_error(env, COLOPRESSO_NATIVE_ERROR_INVALID_ARGUMENT, "invalid options");
    return NULL;
  }

  argument_threads = 0;
  has_argument_threads = false;
  if (argc >= 4 && napi_typeof(env, args[3], &thread_arg_type) == napi_ok && thread_arg_type != napi_undefined && thread_arg_type != napi_null) {
    double numeric_threads;
    if (thread_arg_type != napi_number || napi_get_value_double(env, args[3], &numeric_threads) != napi_ok) {
      cleanup_convert_work(work);
      return throw_type_error(env, "threadCount must be a number when provided");
    }
    argument_threads = (int)numeric_threads;
    has_argument_threads = true;
  }

  if (!resolve_thread_count(env, args[1], work, argument_threads, has_argument_threads)) {
    thread_error_message = work->error_message[0] ? work->error_message : "invalid thread count";
    cleanup_convert_work(work);
    return throw_type_error(env, thread_error_message);
  }

  if (napi_create_promise(env, &work->deferred, &promise) != napi_ok) {
    cleanup_convert_work(work);
    napi_throw_error(env, COLOPRESSO_NATIVE_ERROR_CONVERSION_FAILED, "failed to create promise");
    return NULL;
  }
  napi_create_string_utf8(env, "colopresso.convert", NAPI_AUTO_LENGTH, &resource_name);
  if (napi_create_async_work(env, NULL, resource_name, execute_convert, complete_convert, work, &work->async_work) != napi_ok) {
    cleanup_convert_work(work);
    napi_throw_error(env, COLOPRESSO_NATIVE_ERROR_CONVERSION_FAILED, "failed to create async work");
    return NULL;
  }
  if (napi_queue_async_work(env, work->async_work) != napi_ok) {
    cleanup_convert_work(work);
    napi_throw_error(env, COLOPRESSO_NATIVE_ERROR_CONVERSION_FAILED, "failed to queue async work");
    return NULL;
  }

  return promise;
}

static napi_value init_module(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
      {"getVersionInfo", NULL, get_version_info, NULL, NULL, NULL, napi_default, NULL},
      {"getThreadInfo", NULL, get_thread_info, NULL, NULL, NULL, napi_default, NULL},
      {"convert", NULL, convert, NULL, NULL, NULL, napi_default, NULL},
  };

  napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);

  return exports;
}

NAPI_MODULE(colopresso_native, init_module)
