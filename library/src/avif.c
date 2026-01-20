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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <avif/avif.h>

#include <colopresso.h>

#include "internal/avif.h"
#include "internal/log.h"

static int g_avif_last_error = 0;

int avif_get_last_error(void) { return g_avif_last_error; }

void avif_set_last_error(int error_code) { g_avif_last_error = error_code; }

static inline void apply_avif_config(avifEncoder *encoder, const cpres_config_t *config) {
  float quality;
  int alpha_quality, threads, speed;

  if (!encoder || !config) {
    return;
  }

  quality = config->avif_quality;
  alpha_quality = config->avif_alpha_quality;
  threads = config->avif_threads;

  if (quality < 0) {
    quality = 0;
  } else if (quality > 100) {
    quality = 100;
  }
  if (alpha_quality < 0) {
    alpha_quality = 0;
  } else if (alpha_quality > 100) {
    alpha_quality = 100;
  }

  encoder->quality = quality;
  encoder->qualityAlpha = alpha_quality;

  if (threads > 0) {
    encoder->maxThreads = threads;
  }

  speed = config->avif_speed;
  if (speed < 0 || speed > 10) {
    if (speed < 0) {
      speed = 0;
    } else if (speed > 10) {
      speed = 10;
    }
  }
  encoder->speed = speed;

  if (config->avif_lossless) {
    encoder->quality = AVIF_QUALITY_LOSSLESS;
    encoder->qualityAlpha = AVIF_QUALITY_LOSSLESS;
  }
}

static avifImage *create_avif_image_from_rgba(uint8_t *rgba_data, uint32_t width, uint32_t height) {
  avifImage *image = NULL;
  avifRGBImage rgb;

  if (!rgba_data || width == 0 || height == 0) {
    return image;
  }

  image = avifImageCreate((uint32_t)width, (uint32_t)height, 8, AVIF_PIXEL_FORMAT_YUV444);
  if (!image) {
    return NULL;
  }

  avifRGBImageSetDefaults(&rgb, image);
  rgb.format = AVIF_RGB_FORMAT_RGBA;
  rgb.depth = 8;
  rgb.chromaUpsampling = AVIF_CHROMA_UPSAMPLING_AUTOMATIC;
  rgb.pixels = rgba_data;
  rgb.rowBytes = width * 4;

  if (avifImageRGBToYUV(image, &rgb) != AVIF_RESULT_OK) {
    avifImageDestroy(image);
    return NULL;
  }

  return image;
}

static inline cpres_error_t encode_avif_common(uint8_t *rgba_data, uint32_t width, uint32_t height, avifRWData *output, const cpres_config_t *config) {
  avifImage *image = NULL;
  avifEncoder *encoder = NULL;
  avifResult result;

  if (!rgba_data || !output || !config) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  image = create_avif_image_from_rgba(rgba_data, width, height);
  if (!image) {
    avif_set_last_error(AVIF_RESULT_OUT_OF_MEMORY);
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  encoder = avifEncoderCreate();
  if (!encoder) {
    avifImageDestroy(image);
    avif_set_last_error(AVIF_RESULT_OUT_OF_MEMORY);
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  apply_avif_config(encoder, config);

  result = avifEncoderAddImage(encoder, image, 1, AVIF_ADD_IMAGE_FLAG_SINGLE);
  if (result != AVIF_RESULT_OK) {
    avif_set_last_error(result);
    avifEncoderDestroy(encoder);
    avifImageDestroy(image);
    if (result == AVIF_RESULT_OUT_OF_MEMORY) {
      return CPRES_ERROR_OUT_OF_MEMORY;
    }
    return CPRES_ERROR_ENCODE_FAILED;
  }

  result = avifEncoderFinish(encoder, output);

  if (result != AVIF_RESULT_OK) {
    avif_set_last_error(result);
    avifEncoderDestroy(encoder);
    avifImageDestroy(image);
    if (result == AVIF_RESULT_OUT_OF_MEMORY) {
      return CPRES_ERROR_OUT_OF_MEMORY;
    }
    return CPRES_ERROR_ENCODE_FAILED;
  }

  avif_set_last_error(AVIF_RESULT_OK);
  avifEncoderDestroy(encoder);
  avifImageDestroy(image);
  return CPRES_OK;
}

extern cpres_error_t avif_encode_rgba_to_memory(uint8_t *rgba_data, uint32_t width, uint32_t height, uint8_t **avif_data, size_t *avif_size, const cpres_config_t *config) {
  avifRWData encoded = AVIF_DATA_EMPTY;
  cpres_error_t err;

  if (!rgba_data || !avif_data || !avif_size || !config) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  *avif_data = NULL;
  *avif_size = 0;

  err = encode_avif_common(rgba_data, width, height, &encoded, config);
  if (err != CPRES_OK) {
    if (encoded.data) {
      avifRWDataFree(&encoded);
    }
    return err;
  }

  *avif_size = encoded.size;
  *avif_data = (uint8_t *)malloc(encoded.size);
  if (!*avif_data) {
    avifRWDataFree(&encoded);
    return CPRES_ERROR_OUT_OF_MEMORY;
  }
  memcpy(*avif_data, encoded.data, encoded.size);
  avifRWDataFree(&encoded);

  return CPRES_OK;
}
