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

#include <sys/stat.h>
#include <sys/types.h>

#include <colopresso.h>
#include <colopresso/portable.h>

#include "internal/avif.h"
#include "internal/log.h"
#include "internal/png.h"
#include "internal/webp.h"

#if COLOPRESSO_WITH_FILE_OPS

static inline bool cpres_get_file_size_bytes(const char *path, size_t *size_out) {
  struct stat st;

  if (!path || !size_out) {
    return false;
  }

  if (stat(path, &st) != 0 || st.st_size < 0) {
    return false;
  }

  *size_out = (size_t)st.st_size;

  return true;
}

extern cpres_error_t cpres_read_file_to_memory(const char *path, uint8_t **data_out, size_t *size_out) {
  FILE *fp;
  uint64_t file_size64;
  uint8_t *buffer;
  size_t read_size, file_size;

  if (!path || !data_out || !size_out) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  *data_out = NULL;
  *size_out = 0;

  fp = fopen(path, "rb");
  if (!fp) {
    return CPRES_ERROR_FILE_NOT_FOUND;
  }

  if (!colopresso_fseeko(fp, 0, SEEK_END)) {
    fclose(fp);
    return CPRES_ERROR_IO;
  }

  if (!colopresso_ftello(fp, &file_size64)) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "ftello failed for '%s': errno=%d", path, errno);
    fclose(fp);
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  if (file_size64 <= 0 || file_size64 > (uint64_t)SIZE_MAX) {
    fclose(fp);
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  if (!colopresso_fseeko(fp, 0, SEEK_SET)) {
    fclose(fp);
    return CPRES_ERROR_IO;
  }

  file_size = (size_t)file_size64;

  buffer = (uint8_t *)malloc(file_size);
  if (!buffer) {
    fclose(fp);
    return CPRES_ERROR_OUT_OF_MEMORY;
  }

  read_size = fread(buffer, 1, file_size, fp);
  fclose(fp);

  if (read_size != file_size) {
    free(buffer);
    return CPRES_ERROR_IO;
  }

  *data_out = buffer;
  *size_out = read_size;

  return CPRES_OK;
}

extern cpres_error_t cpres_encode_webp_file(const char *input_path, const char *output_path, const cpres_config_t *config) {
  FILE *fp = NULL;
  uint32_t width, height;
  uint8_t *rgba_data = NULL, *webp_data = NULL;
  size_t written, input_size = 0, webp_size = 0;
  bool have_input_size;
  cpres_error_t error;

  if (!input_path || !output_path || !config) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  have_input_size = cpres_get_file_size_bytes(input_path, &input_size);

  error = png_decode_from_file(input_path, &rgba_data, &width, &height);
  if (error != CPRES_OK) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "PNG read failed: %s", cpres_error_string(error));
    return error;
  }

  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNG loaded - %dx%d pixels", width, height);

  error = webp_encode_rgba_to_memory(rgba_data, width, height, &webp_data, &webp_size, config);
  free(rgba_data);

  if (error != CPRES_OK) {
    return error;
  }

  if (have_input_size && webp_size >= input_size) {
    colopresso_log(CPRES_LOG_LEVEL_WARNING, "WebP: Encoded output larger than input (%zu > %zu)", webp_size, input_size);
    cpres_free(webp_data);
    return CPRES_ERROR_OUTPUT_NOT_SMALLER;
  }

  fp = fopen(output_path, "wb");
  if (!fp) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "Failed to open output file '%s'", output_path);
    cpres_free(webp_data);
    return CPRES_ERROR_IO;
  }

  written = fwrite(webp_data, 1, webp_size, fp);
  fclose(fp);
  cpres_free(webp_data);

  if (written != webp_size) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "Failed to write output file '%s'", output_path);
    return CPRES_ERROR_IO;
  }

  return CPRES_OK;
}

extern cpres_error_t cpres_encode_avif_file(const char *input_path, const char *output_path, const cpres_config_t *config) {
  FILE *fp = NULL;
  uint32_t width, height;
  uint8_t *rgba_data = NULL, *avif_data = NULL;
  size_t written, avif_size = 0, input_size = 0;
  bool have_input_size;
  cpres_error_t error;

  if (!input_path || !output_path || !config) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  have_input_size = cpres_get_file_size_bytes(input_path, &input_size);

  error = png_decode_from_file(input_path, &rgba_data, &width, &height);
  if (error != CPRES_OK) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "PNG read (AVIF) failed: %s", cpres_error_string(error));
    return error;
  }

  colopresso_log(CPRES_LOG_LEVEL_DEBUG, "PNG loaded (AVIF) - %dx%d pixels", width, height);

  error = avif_encode_rgba_to_memory(rgba_data, width, height, &avif_data, &avif_size, config);
  free(rgba_data);

  if (error != CPRES_OK) {
    return error;
  }

  if (have_input_size && avif_size >= input_size) {
    colopresso_log(CPRES_LOG_LEVEL_WARNING, "AVIF: Encoded output larger than input (%zu > %zu)", avif_size, input_size);
    cpres_free(avif_data);
    return CPRES_ERROR_OUTPUT_NOT_SMALLER;
  }

  fp = fopen(output_path, "wb");
  if (!fp) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "Failed to open output file '%s'", output_path);
    cpres_free(avif_data);
    return CPRES_ERROR_IO;
  }

  written = fwrite(avif_data, 1, avif_size, fp);
  fclose(fp);
  cpres_free(avif_data);

  if (written != avif_size) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "Failed to write output file '%s'", output_path);
    return CPRES_ERROR_IO;
  }

  return CPRES_OK;
}

extern cpres_error_t cpres_encode_pngx_file(const char *input_path, const char *output_path, const cpres_config_t *config) {
  FILE *fp = NULL;
  uint8_t *input_data = NULL, *optimized_data = NULL;
  size_t input_size = 0, png_size = 0, optimized_size = 0, written = 0;
  bool have_input_size = false, allow_lossy_rgba_larger_output = false;
  cpres_error_t err;

  if (!input_path || !output_path || !config) {
    return CPRES_ERROR_INVALID_PARAMETER;
  }

  have_input_size = cpres_get_file_size_bytes(input_path, &input_size);
  allow_lossy_rgba_larger_output = (config && config->pngx_lossy_enable && (config->pngx_lossy_type == CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444));

  err = cpres_read_file_to_memory(input_path, &input_data, &png_size);
  if (err != CPRES_OK) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "PNG read (PNGX) failed: %s", cpres_error_string(err));
    return err;
  }

  err = cpres_encode_pngx_memory(input_data, png_size, &optimized_data, &optimized_size, config);
  free(input_data);
  if (err != CPRES_OK) {
    return err;
  }

  if (have_input_size && optimized_size >= input_size) {
    if (allow_lossy_rgba_larger_output) {
      colopresso_log(CPRES_LOG_LEVEL_WARNING, "PNGX: RGBA lossy output larger than input (%zu > %zu) but forcing write per RGBA mode", optimized_size, input_size);
    } else {
      colopresso_log(CPRES_LOG_LEVEL_WARNING, "PNGX: Optimized output larger than input (%zu > %zu)", optimized_size, input_size);
      cpres_free(optimized_data);
      return CPRES_ERROR_OUTPUT_NOT_SMALLER;
    }
  }

  fp = fopen(output_path, "wb");
  if (!fp) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "Failed to open output file '%s'", output_path);
    cpres_free(optimized_data);
    return CPRES_ERROR_IO;
  }

  written = fwrite(optimized_data, 1, optimized_size, fp);
  fclose(fp);
  cpres_free(optimized_data);

  if (written != optimized_size) {
    colopresso_log(CPRES_LOG_LEVEL_ERROR, "Failed to write output file '%s'", output_path);
    return CPRES_ERROR_IO;
  }

  return CPRES_OK;
}

#endif /* COLOPRESSO_WITH_FILE_OPS */
