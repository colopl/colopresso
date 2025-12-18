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

#ifndef COLOPRESSO_TESTS_TEST_H
#define COLOPRESSO_TESTS_TEST_H

#include <colopresso.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef COLOPRESSO_TEST_ASSETS_DIR
#define COLOPRESSO_TEST_ASSETS_DIR "./assets"
#endif

static inline bool format_test_asset_path(char *buffer, size_t buffer_size, const char *name) {
  int written = 0;

  if (!buffer || buffer_size == 0 || !name) {
    return false;
  }

  written = snprintf(buffer, buffer_size, "%s/%s", COLOPRESSO_TEST_ASSETS_DIR, name);
  if (written < 0) {
    return false;
  }

  return (size_t)written < buffer_size;
}

typedef struct {
  colopresso_log_level_t last_level;
  char message[2048];
  bool called;
} test_log_capture_t;

static uint8_t *g_test_cached_example_png_data = NULL;
static uint8_t *g_test_cached_tiny_example_png_data = NULL;
static size_t g_test_cached_example_png_size = 0;
static size_t g_test_cached_tiny_example_png_size = 0;
static test_log_capture_t *g_test_log_capture_target = NULL;

static void common_log_capture_callback(colopresso_log_level_t level, const char *message) {
  if (!g_test_log_capture_target) {
    return;
  }
  g_test_log_capture_target->last_level = level;
  if (message) {
    strncpy(g_test_log_capture_target->message, message, sizeof(g_test_log_capture_target->message) - 1);
    g_test_log_capture_target->message[sizeof(g_test_log_capture_target->message) - 1] = '\0';
  } else {
    g_test_log_capture_target->message[0] = '\0';
  }
  g_test_log_capture_target->called = true;
}

static inline bool bits_required_for_channel(const uint8_t *rgba, size_t pixel_count, uint8_t channel, uint8_t *out_bits) {
  bool used[256];
  size_t i, unique;
  uint32_t mask, value;

  if (!rgba || pixel_count == 0 || channel > 3 || !out_bits) {
    return false;
  }

  memset(used, 0, sizeof(used));
  unique = 0;

  for (i = 0; i < pixel_count; ++i) {
    value = rgba[i * 4 + channel];
    if (!used[value]) {
      used[value] = true;
      ++unique;
      if (unique >= 256) {
        break;
      }
    }
  }

  mask = 1;
  *out_bits = 0;
  while (mask < unique && *out_bits < 8) {
    ++(*out_bits);
    mask <<= 1;
  }

  return true;
}

static inline int color_compare(const void *a, const void *b) {
  uint32_t va = *(const uint32_t *)a, vb = *(const uint32_t *)b;

  if (va < vb) {
    return -1;
  }
  if (va > vb) {
    return 1;
  }
  return 0;
}

static inline bool count_unique_rgba_colors(const uint8_t *rgba, size_t pixel_count, size_t *out_unique_colors) {
  uint32_t *packed;
  size_t i, base, unique;

  if (!rgba || pixel_count == 0 || !out_unique_colors) {
    return false;
  }

  packed = (uint32_t *)malloc(sizeof(uint32_t) * pixel_count);
  if (!packed) {
    return false;
  }

  for (i = 0; i < pixel_count; ++i) {
    base = i * 4;
    packed[i] = ((uint32_t)rgba[base + 0] << 24) | ((uint32_t)rgba[base + 1] << 16) | ((uint32_t)rgba[base + 2] << 8) | (uint32_t)rgba[base + 3];
  }

  qsort(packed, pixel_count, sizeof(uint32_t), color_compare);

  unique = 0;
  for (i = 0; i < pixel_count; ++i) {
    if (i == 0 || packed[i] != packed[i - 1]) {
      ++unique;
    }
  }

  free(packed);
  *out_unique_colors = unique;

  return true;
}

static inline uint8_t *load_test_asset_png(const char *name, size_t *out_size) {
  char path[512];
  FILE *fp = NULL;
  uint8_t *buffer = NULL;
  long file_size = 0;

  if (out_size) {
    *out_size = 0;
  }

  if (!name) {
    return NULL;
  }

  if (snprintf(path, sizeof(path), "%s/%s", COLOPRESSO_TEST_ASSETS_DIR, name) < 0) {
    return NULL;
  }

  fp = fopen(path, "rb");
  if (!fp) {
    return NULL;
  }

  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return NULL;
  }

  file_size = ftell(fp);
  if (file_size <= 0) {
    fclose(fp);
    return NULL;
  }

  if (fseek(fp, 0, SEEK_SET) != 0) {
    fclose(fp);
    return NULL;
  }

  buffer = (uint8_t *)malloc((size_t)file_size);
  if (!buffer) {
    fclose(fp);
    return NULL;
  }

  if (fread(buffer, 1, (size_t)file_size, fp) != (size_t)file_size) {
    fclose(fp);
    free(buffer);
    return NULL;
  }

  fclose(fp);

  if (out_size) {
    *out_size = (size_t)file_size;
  }

  return buffer;
}

static inline void test_default_log_callback(colopresso_log_level_t level, const char *message) {
  printf("LOG (%u): %s\n", (uint8_t)level, message);
  fflush(stdout);
}

static inline void test_log_capture_begin(test_log_capture_t *capture) {
  if (!capture) {
    return;
  }

  memset(capture, 0, sizeof(*capture));
  g_test_log_capture_target = capture;

  cpres_set_log_callback(common_log_capture_callback);
}

static inline void test_log_capture_end(void) {
  g_test_log_capture_target = NULL;

  cpres_set_log_callback(NULL);
}

static inline const uint8_t *test_get_tiny_png(size_t *out_size) {
  static const uint8_t tiny_png[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                                     0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4, 0x89, 0x00, 0x00, 0x00, 0x0B, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9C, 0x63, 0x60, 0x00,
                                     0x02, 0x00, 0x00, 0x05, 0x00, 0x01, 0x7A, 0x5E, 0xAB, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};

  if (out_size) {
    *out_size = sizeof(tiny_png);
  }

  return tiny_png;
}

static inline const uint8_t *get_cached_tiny_example_png(size_t *out_size) {
  FILE *fp = NULL;
  char path[512];
  long fsz = 0;

  if (out_size) {
    *out_size = 0;
  }

  if (!g_test_cached_tiny_example_png_data) {
    if (snprintf(path, sizeof(path), "%s/128x128.png", COLOPRESSO_TEST_ASSETS_DIR) < 0) {
      return NULL;
    }

    fp = fopen(path, "rb");
    if (!fp) {
      return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
      fclose(fp);
      return NULL;
    }

    fsz = ftell(fp);
    if (fsz < 0) {
      fclose(fp);
      return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
      fclose(fp);
      return NULL;
    }

    g_test_cached_tiny_example_png_data = (uint8_t *)malloc((size_t)fsz);
    if (!g_test_cached_tiny_example_png_data) {
      fclose(fp);
      return NULL;
    }

    if (fread(g_test_cached_tiny_example_png_data, 1, (size_t)fsz, fp) != (size_t)fsz) {
      fclose(fp);
      free(g_test_cached_tiny_example_png_data);
      g_test_cached_tiny_example_png_data = NULL;
      return NULL;
    }

    fclose(fp);
    g_test_cached_tiny_example_png_size = (size_t)fsz;
  }

  if (out_size) {
    *out_size = g_test_cached_tiny_example_png_size;
  }

  return g_test_cached_tiny_example_png_data;
}

static inline const uint8_t *get_cached_example_png(size_t *out_size) {
  FILE *fp = NULL;
  char path[512];
  long fsz = 0;

  if (out_size) {
    *out_size = 0;
  }

  if (!g_test_cached_example_png_data) {
    if (snprintf(path, sizeof(path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR) < 0) {
      return NULL;
    }

    fp = fopen(path, "rb");
    if (!fp) {
      return NULL;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
      fclose(fp);
      return NULL;
    }

    fsz = ftell(fp);
    if (fsz < 0) {
      fclose(fp);
      return NULL;
    }

    if (fseek(fp, 0, SEEK_SET) != 0) {
      fclose(fp);
      return NULL;
    }

    g_test_cached_example_png_data = (uint8_t *)malloc((size_t)fsz);
    if (!g_test_cached_example_png_data) {
      fclose(fp);
      return NULL;
    }

    if (fread(g_test_cached_example_png_data, 1, (size_t)fsz, fp) != (size_t)fsz) {
      fclose(fp);
      free(g_test_cached_example_png_data);
      g_test_cached_example_png_data = NULL;
      return NULL;
    }

    fclose(fp);
    g_test_cached_example_png_size = (size_t)fsz;
  }

  if (out_size) {
    *out_size = g_test_cached_example_png_size;
  }

  return g_test_cached_example_png_data;
}

static inline void release_cached_example_png(void) {
  if (g_test_cached_example_png_data) {
    free(g_test_cached_example_png_data);
    g_test_cached_example_png_data = NULL;
    g_test_cached_example_png_size = 0;
  }

  if (g_test_cached_tiny_example_png_data) {
    free(g_test_cached_tiny_example_png_data);
    g_test_cached_tiny_example_png_data = NULL;
    g_test_cached_tiny_example_png_size = 0;
  }
}

#endif /* COLOPRESSO_TESTS_TEST_H */
