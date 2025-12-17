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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#include <png.h>

#define CHANNELS 3u
#define EPSILON 1e-12

typedef struct {
  uint32_t width;
  uint32_t height;
  uint8_t *pixels;
} image_rgb_t;

typedef struct {
  double psnr;
  double ssim;
  size_t size;
} metrics_t;

static inline uint32_t min_u32(uint32_t a, uint32_t b) { return (a < b) ? a : b; }
static inline double clamp_nonnegative(double value) { return (value < 0.0) ? 0.0 : value; }

static inline void free_image(image_rgb_t *img) {
  if (!img) {
    return;
  }

  free(img->pixels);
  img->pixels = NULL;
  img->width = 0;
  img->height = 0;
}

static inline bool get_file_size(const char *path, size_t *out_size) {
  struct stat st;

  if (!path || !out_size) {
    return false;
  }

  if (stat(path, &st) != 0) {
    fprintf(stderr, "Error: failed to stat '%s': %s\n", path, strerror(errno));
    return false;
  }

  *out_size = (size_t)st.st_size;

  return true;
}

static inline bool load_png_rgb(const char *path, image_rgb_t *out_img) {
  FILE *fp;
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep row_buffer, *row_ptrs, src;
  png_uint_32 width = 0, height = 0;
  uint32_t x, y;
  uint8_t *rgb_data = NULL, *dst;
  size_t rowbytes, total_bytes, idx, didx;
  int bit_depth = 0, color_type = 0;
  bool success = false;

  if (!path || !out_img) {
    return false;
  }

  memset(out_img, 0, sizeof(*out_img));

  fp = fopen(path, "rb");
  if (!fp) {
    fprintf(stderr, "Error: failed to open '%s': %s\n", path, strerror(errno));
    goto bailout;
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fprintf(stderr, "Error: png_create_read_struct failed for '%s'\n", path);
    goto bailout;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    fprintf(stderr, "Error: png_create_info_struct failed for '%s'\n", path);
    goto bailout;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    fprintf(stderr, "Error: libpng failed while reading '%s'\n", path);
    goto bailout;
  }

  png_init_io(png_ptr, fp);
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

  if (bit_depth == 16) {
    png_set_strip_16(png_ptr);
  }
  if (color_type == PNG_COLOR_TYPE_PALETTE || (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) || png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    png_set_expand(png_ptr);
  }
  if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png_ptr);
  }
  png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
  png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);

  rowbytes = png_get_rowbytes(png_ptr, info_ptr);
  total_bytes = (size_t)rowbytes * (size_t)height;
  row_buffer = (png_bytep)malloc(total_bytes);
  row_ptrs = (png_bytep *)malloc(sizeof(png_bytep) * (size_t)height);
  if (!row_buffer || !row_ptrs) {
    fprintf(stderr, "Error: out of memory while reading '%s'\n", path);
    goto bailout;
  }

  for (y = 0; y < height; ++y) {
    row_ptrs[y] = row_buffer + (size_t)y * rowbytes;
  }

  png_read_image(png_ptr, row_ptrs);

  rgb_data = (uint8_t *)malloc((size_t)width * (size_t)height * CHANNELS);
  if (!rgb_data) {
    fprintf(stderr, "Error: out of memory while storing '%s'\n", path);
    goto bailout;
  }

  for (y = 0; y < height; ++y) {
    src = row_ptrs[y];
    dst = rgb_data + ((size_t)y * (size_t)width * CHANNELS);
    for (x = 0; x < width; ++x) {
      idx = (size_t)x * 4u;
      didx = (size_t)x * CHANNELS;
      dst[didx + 0] = src[idx + 0];
      dst[didx + 1] = src[idx + 1];
      dst[didx + 2] = src[idx + 2];
    }
  }

  out_img->width = width;
  out_img->height = height;
  out_img->pixels = rgb_data;
  rgb_data = NULL;
  success = true;

bailout:
  free(rgb_data);
  free(row_ptrs);
  free(row_buffer);
  if (png_ptr && info_ptr) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  } else if (png_ptr) {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
  }
  if (fp) {
    fclose(fp);
  }
  if (!success) {
    free_image(out_img);
  }
  return success;
}

static inline double compute_psnr(const image_rgb_t *original, const image_rgb_t *candidate) {
  uint32_t width, height, x, y, c;
  uint8_t *row_a, *row_b;
  size_t count, idx, stride_a, stride_b;
  double mse = 0.0, diff;

  if (!original || !candidate || !original->pixels || !candidate->pixels) {
    return 0.0;
  }

  width = min_u32(original->width, candidate->width);
  height = min_u32(original->height, candidate->height);
  if (width == 0 || height == 0) {
    return 0.0;
  }

  count = (size_t)width * (size_t)height * CHANNELS;
  stride_a = (size_t)original->width * CHANNELS;
  stride_b = (size_t)candidate->width * CHANNELS;

  for (y = 0; y < height; ++y) {
    row_a = original->pixels + (size_t)y * stride_a;
    row_b = candidate->pixels + (size_t)y * stride_b;
    for (x = 0; x < width; ++x) {
      idx = (size_t)x * CHANNELS;
      for (c = 0; c < CHANNELS; ++c) {
        diff = (double)row_a[idx + c] - (double)row_b[idx + c];
        mse += diff * diff;
      }
    }
  }

  mse /= (double)count;
  if (mse <= EPSILON) {
    return 100.0;
  }

  return 20.0 * log10(255.0) - 10.0 * log10(mse);
}

static inline double compute_ssim_block(const image_rgb_t *original, const image_rgb_t *candidate, uint32_t x0, uint32_t y0, uint32_t block_w, uint32_t block_h) {
  const double K1 = 0.01, K2 = 0.03, L = 255.0;
  const double C1 = (K1 * L) * (K1 * L), C2 = (K2 * L) * (K2 * L);
  double sum_a[CHANNELS] = {0.0, 0.0, 0.0}, sum_b[CHANNELS] = {0.0, 0.0, 0.0}, sum_a2[CHANNELS] = {0.0, 0.0, 0.0}, sum_b2[CHANNELS] = {0.0, 0.0, 0.0}, sum_ab[CHANNELS] = {0.0, 0.0, 0.0},
         block_pixels = (double)block_w * (double)block_h, acc = 0.0f, va, vb, inv, mu_x, mu_y, sigma_x, sigma_y, sigma_xy, numerator, denominator;
  uint32_t x, y, c, channel;
  uint8_t *row_a, *row_b;
  size_t idx, stride_a, stride_b;

  if (block_w == 0 || block_h == 0 || block_pixels <= 0.0) {
    return 0.0;
  }

  stride_a = (size_t)original->width * CHANNELS;
  stride_b = (size_t)candidate->width * CHANNELS;

  for (y = 0; y < block_h; ++y) {
    row_a = original->pixels + ((size_t)y0 + (size_t)y) * stride_a + (size_t)x0 * CHANNELS;
    row_b = candidate->pixels + ((size_t)y0 + (size_t)y) * stride_b + (size_t)x0 * CHANNELS;
    for (x = 0; x < block_w; ++x) {
      idx = (size_t)x * CHANNELS;
      for (c = 0; c < CHANNELS; ++c) {
        va = (double)row_a[idx + c];
        vb = (double)row_b[idx + c];
        sum_a[c] += va;
        sum_b[c] += vb;
        sum_a2[c] += va * va;
        sum_b2[c] += vb * vb;
        sum_ab[c] += va * vb;
      }
    }
  }

  for (channel = 0; channel < CHANNELS; ++channel) {
    inv = 1.0 / block_pixels;
    mu_x = sum_a[channel] * inv;
    mu_y = sum_b[channel] * inv;
    sigma_x = clamp_nonnegative(sum_a2[channel] * inv - mu_x * mu_x);
    sigma_y = clamp_nonnegative(sum_b2[channel] * inv - mu_y * mu_y);
    sigma_xy = sum_ab[channel] * inv - mu_x * mu_y;
    numerator = (2.0 * mu_x * mu_y + C1) * (2.0 * sigma_xy + C2);
    denominator = (mu_x * mu_x + mu_y * mu_y + C1) * (sigma_x + sigma_y + C2);
    if (denominator <= EPSILON) {
      acc += 1.0;
    } else {
      acc += numerator / denominator;
    }
  }

  return acc / (double)CHANNELS;
}

static inline double compute_ssim(const image_rgb_t *original, const image_rgb_t *candidate) {
  const uint32_t win = 8;
  uint32_t width, height, blocks_w, blocks_h, by, bx, block_count = 0;
  double sum = 0.0;

  if (!original || !candidate || !original->pixels || !candidate->pixels) {
    return 0.0;
  }

  width = min_u32(original->width, candidate->width);
  height = min_u32(original->height, candidate->height);
  if (width == 0 || height == 0) {
    return 0.0;
  }

  blocks_w = width / win;
  blocks_h = height / win;

  if (blocks_w == 0 || blocks_h == 0) {
    return compute_ssim_block(original, candidate, 0, 0, width, height);
  }

  for (by = 0; by < blocks_h; ++by) {
    for (bx = 0; bx < blocks_w; ++bx) {
      sum += compute_ssim_block(original, candidate, bx * win, by * win, win, win);
      block_count++;
    }
  }

  if (block_count == 0) {
    return 0.0;
  }

  return sum / (double)block_count;
}

static inline bool compute_metrics(const image_rgb_t *original, const image_rgb_t *candidate, const char *candidate_path, metrics_t *out_metrics) {
  size_t file_size = 0;

  if (!original || !candidate || !out_metrics) {
    return false;
  }

  if (!get_file_size(candidate_path, &file_size)) {
    return false;
  }

  out_metrics->psnr = compute_psnr(original, candidate);
  out_metrics->ssim = compute_ssim(original, candidate);
  out_metrics->size = file_size;

  return true;
}

static inline void human_bytes(size_t value, char *buffer, size_t buffer_size) {
  static const char *units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
  size_t unit_index = 0, unit_count = sizeof(units) / sizeof(units[0]);
  double bytes = (double)value;

  if (!buffer || buffer_size == 0) {
    return;
  }

  while (bytes >= 1024.0 && unit_index + 1 < unit_count) {
    bytes /= 1024.0;
    unit_index++;
  }

  if (unit_index == 0) {
    snprintf(buffer, buffer_size, "%zu %s", value, units[unit_index]);
  } else {
    snprintf(buffer, buffer_size, "%.1f %s", bytes, units[unit_index]);
  }
}

static inline char winner_label(double a, double b, bool higher_is_better, double *margin) {
  double delta;

  if (margin) {
    *margin = 0.0;
  }

  if (higher_is_better) {
    delta = a - b;
    if (fabs(delta) <= EPSILON) {
      return '=';
    }
    if (margin) {
      *margin = fabs(delta);
    }

    return (delta > 0.0) ? 'A' : 'B';
  }

  delta = b - a;
  if (fabs(delta) <= EPSILON) {
    return '=';
  }

  if (margin) {
    *margin = fabs(delta);
  }

  return (delta > 0.0) ? 'A' : 'B';
}

static double percent_improvement(double a, double b, bool higher_is_better) {
  double base;

  if (higher_is_better) {
    base = fabs(b) > EPSILON ? fabs(b) : EPSILON;

    return (a - b) / base * 100.0;
  }

  base = (b > EPSILON) ? b : EPSILON;

  return (b - a) / base * 100.0;
}

static inline const char *basename_ptr(const char *path) {
  const char *slash, *backslash;

  if (!path) {
    return "";
  }

  slash = strrchr(path, '/');
  backslash = strrchr(path, '\\');
  if (backslash && (!slash || backslash > slash)) {
    slash = backslash;
  }

  return slash ? slash + 1 : path;
}

static inline void print_table(const char *name_a, const char *name_b, const metrics_t *metrics_a, const metrics_t *metrics_b) {
  const char *header = "+--------------+------------+------------+--------+--------------+---------------+";
  uint32_t wins_a = 0, wins_b = 0;
  char size_a[32], size_b[32], margin_psnr_str[32], margin_ssim_str[32], margin_size_str[32], win_psnr, win_ssim, win_size, overall[64];
  double margin_psnr, margin_ssim, margin_size, imp_psnr, imp_ssim, imp_size;

  human_bytes(metrics_a->size, size_a, sizeof(size_a));
  human_bytes(metrics_b->size, size_b, sizeof(size_b));

  win_psnr = winner_label(metrics_a->psnr, metrics_b->psnr, true, &margin_psnr);
  win_ssim = winner_label(metrics_a->ssim, metrics_b->ssim, true, &margin_ssim);
  win_size = winner_label((double)metrics_a->size, (double)metrics_b->size, false, &margin_size);

  imp_psnr = percent_improvement(metrics_a->psnr, metrics_b->psnr, true);
  imp_ssim = percent_improvement(metrics_a->ssim, metrics_b->ssim, true);
  imp_size = percent_improvement((double)metrics_a->size, (double)metrics_b->size, false);

  if (win_psnr == '=') {
    snprintf(margin_psnr_str, sizeof(margin_psnr_str), "=");
  } else {
    snprintf(margin_psnr_str, sizeof(margin_psnr_str), "%+.3f dB", margin_psnr);
  }

  if (win_ssim == '=') {
    snprintf(margin_ssim_str, sizeof(margin_ssim_str), "=");
  } else {
    snprintf(margin_ssim_str, sizeof(margin_ssim_str), "%+.5f", margin_ssim);
  }

  if (win_size == '=') {
    snprintf(margin_size_str, sizeof(margin_size_str), "=");
  } else {
    snprintf(margin_size_str, sizeof(margin_size_str), "%zu B", (size_t)margin_size);
  }

  printf("%s\n", header);
  printf("| %-12s | %-10s | %-10s | %-6s | %-12s | %-13s |\n", "Metric", "A", "B", "Better", "Margin", "Improvement");
  printf("%s\n", header);
  printf("| %-12s | %10.3f | %10.3f | %-6c | %-12s | %+-11.2f%%  |\n", "PSNR (dB)", metrics_a->psnr, metrics_b->psnr, win_psnr, margin_psnr_str, imp_psnr);
  printf("| %-12s | %10.5f | %10.5f | %-6c | %-12s | %+-11.2f%%  |\n", "SSIM", metrics_a->ssim, metrics_b->ssim, win_ssim, margin_ssim_str, imp_ssim);
  printf("| %-12s | %10s | %10s | %-6c | %-12s | %+-11.2f%%  |\n", "Size", size_a, size_b, win_size, margin_size_str, imp_size);
  printf("%s\n", header);

  snprintf(overall, sizeof(overall), "Overall: Tie");

  if (win_psnr == 'A') {
    wins_a++;
  } else if (win_psnr == 'B') {
    wins_b++;
  }
  if (win_ssim == 'A') {
    wins_a++;
  } else if (win_ssim == 'B') {
    wins_b++;
  }
  if (win_size == 'A') {
    wins_a++;
  } else if (win_size == 'B') {
    wins_b++;
  }

  if (wins_a > wins_b) {
    snprintf(overall, sizeof(overall), "Overall: A wins (%u-%u)", wins_a, wins_b);
  } else if (wins_b > wins_a) {
    snprintf(overall, sizeof(overall), "Overall: B wins (%u-%u)", wins_b, wins_a);
  } else {
    if (fabs(metrics_a->psnr - metrics_b->psnr) > EPSILON) {
      snprintf(overall, sizeof(overall), "%s", (metrics_a->psnr > metrics_b->psnr) ? "Overall: A wins (PSNR)" : "Overall: B wins (PSNR)");
    } else if (fabs(metrics_a->ssim - metrics_b->ssim) > EPSILON) {
      snprintf(overall, sizeof(overall), "%s", (metrics_a->ssim > metrics_b->ssim) ? "Overall: A wins (SSIM)" : "Overall: B wins (SSIM)");
    } else if (metrics_a->size != metrics_b->size) {
      snprintf(overall, sizeof(overall), "%s", (metrics_a->size < metrics_b->size) ? "Overall: A wins (smaller)" : "Overall: B wins (smaller)");
    }
  }

  printf("A: %s\n", name_a);
  printf("B: %s\n", name_b);
  printf("%s\n", overall);
}

int main(int argc, char **argv) {
  const char *orig_path, *cand_a_path, *cand_b_path;
  int exit_code = 0;
  image_rgb_t orig = {0}, cand_a = {0}, cand_b = {0};
  metrics_t metrics_a, metrics_b;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <original.png> <candidate_a.png> <candidate_b.png>\n", argv[0] ? argv[0] : "qcheck");
    return 2;
  }

  orig_path = argv[1];
  cand_a_path = argv[2];
  cand_b_path = argv[3];

  if (!load_png_rgb(orig_path, &orig)) {
    exit_code = 1;
    goto bailout;
  }
  if (!load_png_rgb(cand_a_path, &cand_a)) {
    exit_code = 1;
    goto bailout;
  }
  if (!load_png_rgb(cand_b_path, &cand_b)) {
    exit_code = 1;
    goto bailout;
  }

  if (!compute_metrics(&orig, &cand_a, cand_a_path, &metrics_a)) {
    exit_code = 1;
    goto bailout;
  }
  if (!compute_metrics(&orig, &cand_b, cand_b_path, &metrics_b)) {
    exit_code = 1;
    goto bailout;
  }

  print_table(basename_ptr(cand_a_path), basename_ptr(cand_b_path), &metrics_a, &metrics_b);

bailout:
  free_image(&orig);
  free_image(&cand_a);
  free_image(&cand_b);
  return exit_code;
}
