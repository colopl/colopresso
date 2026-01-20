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

#include <inttypes.h>
#include <limits.h>
#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <png.h>

enum {
  CHANNEL_COUNT = 4,
  DEFAULT_BIT_DEPTH = 8,
  MAX_BIT_DEPTH = 16,
  MAX_TRACKED_UNIQUES = 1 << MAX_BIT_DEPTH,
};

typedef enum {
  EXIT_OK = 0,
  EXIT_INVALID_ARGS = 1,
  EXIT_DECODE_FAILED = 2,
  EXIT_INVALID_DIMENSIONS = 3,
  EXIT_IMAGE_TOO_LARGE = 4,
  EXIT_ANALYSIS_TOO_LARGE = 5,
  EXIT_OUT_OF_MEMORY = 6,
} exit_code_t;

typedef enum {
  COLOR_MODE_UNKNOWN = 0,
  COLOR_MODE_PALETTE,
  COLOR_MODE_GRAY,
  COLOR_MODE_GRAY_ALPHA,
  COLOR_MODE_RGBA,
} color_mode_t;

typedef struct {
  uint16_t mask;
  uint16_t min_value;
  uint16_t max_value;
  bool seen;
  uint32_t unique_count;
  uint8_t used_bits;
} channel_stats_t;

typedef struct {
  bool available;
  uint8_t color_type;
  uint8_t bit_depth;
  uint16_t palette_entries;
} png_metadata_t;

typedef struct {
  png_metadata_t metadata;
  png_uint_32 width;
  png_uint_32 height;
  uint8_t sample_bit_depth;
  uint16_t *rgba;
} png_image_t;

typedef struct {
  uint64_t value;
  bool overflow;
  uint8_t overflow_bits;
} capacity_value_t;

static const char *const G_CHANNEL_LABELS[CHANNEL_COUNT] = {"R", "G", "B", "A"};

static inline capacity_value_t capacity_from_u64(uint64_t value) {
  capacity_value_t cap;

  cap.value = value;
  cap.overflow = false;
  cap.overflow_bits = 0;
  return cap;
}

static inline capacity_value_t capacity_overflow(uint8_t bits) {
  capacity_value_t cap;

  cap.value = UINT64_MAX;
  cap.overflow = true;
  cap.overflow_bits = bits;
  return cap;
}

static inline void print_usage(const char *program_name) { fprintf(stderr, "Usage: %s <input.png>\n", program_name ? program_name : "chkbits"); }

static inline int compare_u64(const void *lhs, const void *rhs) {
  const uint64_t a = *(const uint64_t *)lhs, b = *(const uint64_t *)rhs;

  return (a < b) ? -1 : (a > b) ? 1 : 0;
}

static inline uint8_t clamp_bit_depth(uint8_t bit_depth) {
  if (bit_depth == 0 || bit_depth > 16) {
    return DEFAULT_BIT_DEPTH;
  }
  return bit_depth;
}

static bool pow2_to_decimal_string(uint8_t bits, char *buffer, size_t buffer_size) {
  uint16_t value;
  uint8_t digits[40] = {0}, carry = 0;
  size_t digit_count = 1, idx, i;

  if (!buffer || buffer_size == 0) {
    return false;
  }

  digits[0] = 1;
  for (i = 0; i < bits; i++) {
    for (idx = 0; idx < digit_count; idx++) {
      value = (uint16_t)(digits[idx] * 2) + carry;
      digits[idx] = (uint8_t)(value % 10);
      carry = (uint8_t)(value / 10);
    }

    if (carry != 0) {
      if (digit_count >= sizeof(digits)) {
        return false;
      }
      digits[digit_count++] = carry;
    }
  }

  if (digit_count + 1 > buffer_size) {
    return false;
  }

  for (i = 0; i < digit_count; i++) {
    buffer[i] = (char)('0' + digits[digit_count - 1 - i]);
  }
  buffer[digit_count] = '\0';

  return true;
}

static inline void free_png_image(png_image_t *image) {
  if (!image) {
    return;
  }
  if (image->rgba) {
    free(image->rgba);
    image->rgba = NULL;
  }
}

static bool decode_png_image(const char *path, png_image_t *out) {
  FILE *fp = NULL;
  png_structp png_ptr = NULL;
  png_infop info_ptr = NULL;
  png_bytep image_data = NULL, *row_pointers = NULL;
  png_colorp palette = NULL;
  png_image_t result = {0};
  png_size_t rowbytes;
  png_uint_32 y;
  uint32_t bit_depth, color_type;
  size_t pixel_count, samples, src, row_span, image_size, i;
  int num_palette = 0;
  bool ok = false;

  if (!path || !out) {
    return false;
  }

  fp = fopen(path, "rb");
  if (!fp) {
    return false;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    goto bailout;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    goto bailout;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    goto bailout;
  }

  png_init_io(png_ptr, fp);
  png_read_info(png_ptr, info_ptr);

  result.width = png_get_image_width(png_ptr, info_ptr);
  result.height = png_get_image_height(png_ptr, info_ptr);
  bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  color_type = png_get_color_type(png_ptr, info_ptr);

  result.metadata.available = true;
  result.metadata.color_type = (uint8_t)color_type;
  result.metadata.bit_depth = clamp_bit_depth((uint8_t)bit_depth);
  result.metadata.palette_entries = 0;

  if (color_type == PNG_COLOR_TYPE_PALETTE && png_get_valid(png_ptr, info_ptr, PNG_INFO_PLTE)) {
    png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
    if (num_palette > 256) {
      num_palette = 256;
    }
    if (num_palette > 0) {
      result.metadata.palette_entries = (uint16_t)num_palette;
    }
  }

  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
  }
  if ((color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) && bit_depth < 8) {
    png_set_expand_gray_1_2_4_to_8(png_ptr);
  }
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png_ptr);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png_ptr);
  }
  if (!(color_type & PNG_COLOR_MASK_ALPHA) && !png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
  }

  if (png_get_interlace_type(png_ptr, info_ptr) != PNG_INTERLACE_NONE) {
    png_set_interlace_handling(png_ptr);
  }

  png_read_update_info(png_ptr, info_ptr);

  result.width = png_get_image_width(png_ptr, info_ptr);
  result.height = png_get_image_height(png_ptr, info_ptr);
  result.sample_bit_depth = (uint8_t)png_get_bit_depth(png_ptr, info_ptr);
  if (result.sample_bit_depth > MAX_BIT_DEPTH) {
    result.sample_bit_depth = MAX_BIT_DEPTH;
  }
  if (result.sample_bit_depth <= 8) {
    result.sample_bit_depth = 8;
  }

  rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  row_span = (size_t)rowbytes;
  if (result.height != 0 && row_span > SIZE_MAX / result.height) {
    goto bailout;
  }
  image_size = row_span * (size_t)result.height;

  if (result.height != 0 && result.width > SIZE_MAX / result.height) {
    goto bailout;
  }

  pixel_count = (size_t)result.width * (size_t)result.height;
  if (pixel_count == 0) {
    goto bailout;
  }
  if (pixel_count > SIZE_MAX / CHANNEL_COUNT) {
    goto bailout;
  }

  image_data = (png_bytep)malloc(image_size);
  if (!image_data) {
    goto bailout;
  }

  row_pointers = (png_bytep *)malloc((size_t)result.height * sizeof(png_bytep));
  if (!row_pointers) {
    goto bailout;
  }

  for (y = 0; y < result.height; y++) {
    row_pointers[y] = image_data + (size_t)y * row_span;
  }

  png_read_image(png_ptr, row_pointers);

  samples = pixel_count * CHANNEL_COUNT;
  if (samples > SIZE_MAX / sizeof(uint16_t)) {
    goto bailout;
  }
  result.rgba = (uint16_t *)malloc(samples * sizeof(uint16_t));
  if (!result.rgba) {
    goto bailout;
  }

  if (result.sample_bit_depth <= 8) {
    for (i = 0; i < samples; i++) {
      result.rgba[i] = (uint16_t)image_data[i];
    }
  } else {
    src = 0;
    for (i = 0; i < samples; i++) {
      result.rgba[i] = ((uint16_t)image_data[src] << 8) | (uint16_t)image_data[src + 1];
      src += 2;
    }
  }

  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  fclose(fp);
  free(row_pointers);
  free(image_data);

  *out = result;
  return true;

bailout:
  if (row_pointers) {
    free(row_pointers);
  }
  if (image_data) {
    free(image_data);
  }
  free_png_image(&result);
  if (png_ptr || info_ptr) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  }
  if (fp) {
    fclose(fp);
  }
  return false;
}

static inline uint64_t count_unique_colors(uint64_t *colors, size_t count) {
  uint64_t unique;
  size_t i;

  if (!colors || count == 0) {
    return 0;
  }

  qsort(colors, count, sizeof(uint64_t), compare_u64);

  unique = 1;
  for (i = 1; i < count; i++) {
    if (colors[i] != colors[i - 1]) {
      unique++;
    }
  }

  return unique;
}

static inline uint8_t bits_required(uint32_t count) {
  uint32_t capacity = 1;
  uint8_t bits = 0;

  if (count <= 1) {
    return 0;
  }

  while (capacity < count && bits < MAX_BIT_DEPTH) {
    bits++;
    capacity <<= 1;
  }

  return bits;
}

static inline void update_channel_stat(channel_stats_t *stats, uint16_t value, uint8_t *value_seen_row) {
  stats->mask |= (uint16_t)value;

  if (!stats->seen) {
    stats->min_value = value;
    stats->max_value = value;
    stats->seen = true;
  } else {
    if (value < stats->min_value) {
      stats->min_value = value;
    }
    if (value > stats->max_value) {
      stats->max_value = value;
    }
  }

  if (!value_seen_row[value] && stats->unique_count < MAX_TRACKED_UNIQUES) {
    value_seen_row[value] = 1;
    stats->unique_count++;
  }
}

static inline void report_channel(const char *label, const channel_stats_t *stats, bool palette_mode) {
  uint32_t theoretical_values;

  if (!stats->seen) {
    printf("  %s: no samples\n", label);
    return;
  }

  if (palette_mode) {
    printf("  %s: palette indexed (per-channel values defined by palette)\n", label);
    return;
  }

  theoretical_values = (stats->used_bits == 0) ? 1 : (1 << stats->used_bits);
  printf("  %s: %u unique values (%u-bit, max %u) range [%u,%u] mask=0x%04X\n", label, stats->unique_count, stats->used_bits, theoretical_values, stats->min_value, stats->max_value, stats->mask);
}

static inline color_mode_t classify_color_mode(const png_metadata_t *metadata) {
  if (!metadata || !metadata->available) {
    return COLOR_MODE_UNKNOWN;
  }

  switch (metadata->color_type) {
  case PNG_COLOR_TYPE_PALETTE:
    return COLOR_MODE_PALETTE;
  case PNG_COLOR_TYPE_GRAY:
    return COLOR_MODE_GRAY;
  case PNG_COLOR_TYPE_GRAY_ALPHA:
    return COLOR_MODE_GRAY_ALPHA;
  default:
    return COLOR_MODE_RGBA;
  }
}

static inline capacity_value_t palette_capacity(const png_metadata_t *metadata) {
  uint64_t entries;
  uint8_t depth;

  if (!metadata) {
    return capacity_from_u64(1ll << DEFAULT_BIT_DEPTH);
  }

  if (metadata->palette_entries > 0) {
    return capacity_from_u64(metadata->palette_entries);
  }

  depth = (metadata->bit_depth == 0) ? DEFAULT_BIT_DEPTH : metadata->bit_depth;
  entries = 1ll << depth;
  return capacity_from_u64((entries == 0) ? (1ll << DEFAULT_BIT_DEPTH) : entries);
}

static inline capacity_value_t grayscale_capacity(const png_metadata_t *metadata, bool include_alpha) {
  uint64_t levels, alpha_levels;
  uint8_t depth;

  if (!metadata) {
    return capacity_from_u64(0);
  }

  depth = (metadata->bit_depth == 0) ? DEFAULT_BIT_DEPTH : metadata->bit_depth;
  levels = 1ll << depth;
  if (!include_alpha) {
    return capacity_from_u64(levels);
  }

  alpha_levels = levels;
  if (depth >= 64 || levels > (UINT64_MAX / alpha_levels)) {
    return capacity_overflow((uint8_t)(depth * 2));
  }
  return capacity_from_u64(levels * alpha_levels);
}

static inline capacity_value_t rgba_capacity(const channel_stats_t *channels) {
  uint64_t capacity = 1;
  uint16_t total_bits = 0;
  uint8_t i, bits;

  for (i = 0; i < CHANNEL_COUNT; i++) {
    bits = channels[i].used_bits;
    if (bits == 0) {
      continue;
    }

    total_bits += bits;
    if (total_bits >= 64) {
      return capacity_overflow((uint8_t)total_bits);
    }

    if (bits >= 64 || capacity > (UINT64_MAX >> bits)) {
      return capacity_overflow((uint8_t)total_bits);
    }

    capacity <<= bits;
  }

  return capacity_from_u64(capacity);
}

static inline uint64_t pack_rgba64(const uint16_t *pixel, uint8_t channel_bits) {
  uint64_t packed = 0;
  uint8_t i;

  if (channel_bits > MAX_BIT_DEPTH) {
    channel_bits = MAX_BIT_DEPTH;
  }

  for (i = 0; i < CHANNEL_COUNT; i++) {
    packed = (packed << channel_bits) | (uint64_t)pixel[i];
  }

  return packed;
}

int main(int argc, char **argv) {
  const char *input_path;
  uint64_t *colors = NULL, unique_colors = 0;
  uint8_t idx, channel_bits, *value_seen = NULL, *value_rows[CHANNEL_COUNT] = {NULL};
  size_t pixel_count = 0, base = 0, value_limit, i;
  png_image_t image = {0};
  channel_stats_t channels[CHANNEL_COUNT] = {{0}};
  capacity_value_t max_colors = capacity_from_u64(0);
  color_mode_t color_mode;
  int exit_code = EXIT_OK;

  if (argc != 2) {
    print_usage(argv[0]);
    return EXIT_INVALID_ARGS;
  }

  input_path = argv[1];
  if (!decode_png_image(input_path, &image)) {
    fprintf(stderr, "Failed to decode %s as PNG\n", input_path);
    return EXIT_DECODE_FAILED;
  }

  color_mode = classify_color_mode(&image.metadata);

  if (image.width == 0 || image.height == 0) {
    fprintf(stderr, "Invalid dimensions (%u x %u)\n", image.width, image.height);
    exit_code = EXIT_INVALID_DIMENSIONS;
    goto bailout;
  }

  if (image.height != 0 && image.width > SIZE_MAX / image.height) {
    fprintf(stderr, "Image too large to process\n");
    exit_code = EXIT_IMAGE_TOO_LARGE;
    goto bailout;
  }

  pixel_count = (size_t)image.width * (size_t)image.height;
  if (pixel_count == 0) {
    fprintf(stderr, "Image has no pixels\n");
    exit_code = EXIT_INVALID_DIMENSIONS;
    goto bailout;
  }

  if (pixel_count > SIZE_MAX / sizeof(uint64_t)) {
    fprintf(stderr, "Image too large to analyze\n");
    exit_code = EXIT_ANALYSIS_TOO_LARGE;
    goto bailout;
  }

  colors = (uint64_t *)malloc(pixel_count * sizeof(uint64_t));
  if (!colors) {
    fprintf(stderr, "Out of memory while allocating color buffer\n");
    exit_code = EXIT_OUT_OF_MEMORY;
    goto bailout;
  }

  channel_bits = (image.sample_bit_depth > 8) ? 16 : 8;
  value_limit = ((size_t)1) << channel_bits;

  value_seen = (uint8_t *)calloc(CHANNEL_COUNT, value_limit);
  if (!value_seen) {
    fprintf(stderr, "Out of memory while tracking channel values\n");
    exit_code = EXIT_OUT_OF_MEMORY;
    goto bailout;
  }

  for (idx = 0; idx < CHANNEL_COUNT; idx++) {
    value_rows[idx] = value_seen + (size_t)idx * value_limit;
  }

  for (i = 0; i < pixel_count; i++) {
    base = i * CHANNEL_COUNT;
    for (idx = 0; idx < CHANNEL_COUNT; idx++) {
      update_channel_stat(&channels[idx], image.rgba[base + idx], value_rows[idx]);
    }
    colors[i] = pack_rgba64(&image.rgba[base], channel_bits);
  }

  unique_colors = count_unique_colors(colors, pixel_count);

  for (i = 0; i < CHANNEL_COUNT; i++) {
    channels[i].used_bits = bits_required(channels[i].unique_count);
  }

  switch (color_mode) {
  case COLOR_MODE_PALETTE:
    max_colors = palette_capacity(&image.metadata);
    break;
  case COLOR_MODE_GRAY:
    max_colors = grayscale_capacity(&image.metadata, false);
    break;
  case COLOR_MODE_GRAY_ALPHA:
    max_colors = grayscale_capacity(&image.metadata, true);
    break;
  case COLOR_MODE_RGBA:
  case COLOR_MODE_UNKNOWN:
  default:
    max_colors = rgba_capacity(channels);
    break;
  }

  printf("File: %s\n", input_path);
  printf("Size: %u x %u (%zu pixels)\n", image.width, image.height, pixel_count);
  if (max_colors.overflow) {
    char capacity_buffer[64];
    if (pow2_to_decimal_string(max_colors.overflow_bits, capacity_buffer, sizeof(capacity_buffer))) {
      printf("Unique colors: %" PRIu64 " / %s\n", unique_colors, capacity_buffer);
    } else {
      printf("Unique colors: %" PRIu64 " / 2^%u\n", unique_colors, max_colors.overflow_bits);
    }
  } else {
    printf("Unique colors: %" PRIu64 " / %" PRIu64 "\n", unique_colors, max_colors.value);
  }
  if (color_mode == COLOR_MODE_PALETTE) {
    printf("Palette PNG: %" PRIu64 " entries (index bit depth %u)\n", max_colors.value, image.metadata.bit_depth);
  } else if (color_mode == COLOR_MODE_GRAY || color_mode == COLOR_MODE_GRAY_ALPHA) {
    printf("Color type: %s (bit depth %u)\n", (color_mode == COLOR_MODE_GRAY_ALPHA) ? "Grayscale+Alpha" : "Grayscale", image.metadata.bit_depth);
  }
  printf("Channel usage:\n");
  if (color_mode == COLOR_MODE_GRAY || color_mode == COLOR_MODE_GRAY_ALPHA) {
    report_channel("Intensity", &channels[0], false);
    if (color_mode == COLOR_MODE_GRAY_ALPHA) {
      report_channel("Alpha", &channels[3], false);
    }
  } else {
    for (i = 0; i < CHANNEL_COUNT; i++) {
      report_channel(G_CHANNEL_LABELS[i], &channels[i], color_mode == COLOR_MODE_PALETTE);
    }
  }

bailout:
  free(colors);
  free(value_seen);
  free_png_image(&image);
  return exit_code;
}
