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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <webp/encode.h>

#include <colopresso.h>

#include "internal/log.h"
#include "internal/webp.h"

static int g_last_webp_error = 0;

int webp_get_last_error(void) { return g_last_webp_error; }

void webp_set_last_error(int error_code) { g_last_webp_error = error_code; }

static void apply_webp_config(WebPConfig *webp_config, const cpres_config_t *config) {
  webp_config->quality = config->webp_quality;
  webp_config->target_size = config->webp_target_size;
  webp_config->target_PSNR = config->webp_target_psnr;
  webp_config->method = config->webp_method;
  webp_config->segments = config->webp_segments;
  webp_config->sns_strength = config->webp_sns_strength;
  webp_config->filter_strength = config->webp_filter_strength;
  webp_config->filter_sharpness = config->webp_filter_sharpness;
  webp_config->filter_type = config->webp_filter_type;
  webp_config->autofilter = config->webp_autofilter;
  webp_config->alpha_compression = config->webp_alpha_compression;
  webp_config->alpha_filtering = config->webp_alpha_filtering;
  webp_config->alpha_quality = config->webp_alpha_quality;
  webp_config->pass = config->webp_pass;
  webp_config->preprocessing = config->webp_preprocessing;
  webp_config->partitions = config->webp_partitions;
  webp_config->partition_limit = config->webp_partition_limit;
  webp_config->emulate_jpeg_size = config->webp_emulate_jpeg_size;
  webp_config->thread_level = (config->webp_thread_level > 0) ? 1 : 0;
  webp_config->low_memory = config->webp_low_memory;
  webp_config->near_lossless = config->webp_near_lossless;
  webp_config->exact = config->webp_exact;
  webp_config->use_delta_palette = config->webp_use_delta_palette;
  webp_config->use_sharp_yuv = config->webp_use_sharp_yuv;
  webp_config->lossless = config->webp_lossless;
}

cpres_error_t webp_encode_rgba_to_memory(uint8_t *rgba_data, uint32_t width, uint32_t height, uint8_t **webp_data, size_t *webp_size, const cpres_config_t *config) {
  WebPConfig webp_config;
  WebPPicture picture;
  WebPMemoryWriter writer;

  if (!rgba_data || !webp_data || !webp_size || !config) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  memset(&webp_config, 0, sizeof(webp_config));
  memset(&picture, 0, sizeof(picture));
  memset(&writer, 0, sizeof(writer));

  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "Starting WebP encoding to memory - %dx%d pixels", width, height);

  if (!WebPConfigPreset(&webp_config, WEBP_PRESET_DEFAULT, config->webp_quality)) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  apply_webp_config(&webp_config, config);

  if (!WebPValidateConfig(&webp_config)) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  if (!WebPPictureInit(&picture)) {
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  picture.width = (int)width;
  picture.height = (int)height;
  picture.use_argb = 1;

  if (!WebPPictureImportRGBA(&picture, rgba_data, width * 4)) {
    WebPPictureFree(&picture);
    return CPRES_ERROR_ENCODE_FAILED;
  }

  WebPMemoryWriterInit(&writer);
  picture.writer = WebPMemoryWrite;
  picture.custom_ptr = &writer;

  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "Starting WebP encoding (memory)...");

  if (!WebPEncode(&webp_config, &picture)) {
    webp_set_last_error(picture.error_code);
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "WebP encoding failed - error code: %d", picture.error_code);
    WebPMemoryWriterClear(&writer);
    WebPPictureFree(&picture);
    return CPRES_ERROR_ENCODE_FAILED;
  }
  webp_set_last_error(0);

  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "WebP encoding successful - size: %zu bytes", writer.size);

  *webp_data = (uint8_t *)malloc(writer.size);
  if (!*webp_data) {
    WebPMemoryWriterClear(&writer);
    WebPPictureFree(&picture);
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  memcpy(*webp_data, writer.mem, writer.size);
  *webp_size = writer.size;

  WebPMemoryWriterClear(&writer);
  WebPPictureFree(&picture);

  return CPRES_OK;
}
