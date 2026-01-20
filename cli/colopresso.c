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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>

#include <colopresso.h>
#include <colopresso/portable.h>

#include "../library/src/internal/pngx.h"

typedef enum {
  FORMAT_WEBP,
  FORMAT_AVIF,
  FORMAT_PNGX,
  FORMAT_UNKNOWN
} output_format_t;

typedef struct {
  cpres_config_t config;
  output_format_t format;
  bool verbose;
  const char *input_file;
  char *output_file;
  cpres_rgba_color_t *protected_colors;
  int32_t protected_colors_count;
} cli_context_t;

static struct option kLongOptions[] = {
    {"format", required_argument, 0, 0},
    {"type", required_argument, 0, 0},
    {"verbose", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {"version", no_argument, 0, 'V'},
    {"quality", required_argument, 0, 'q'},
    {"lossless", no_argument, 0, 'l'},
    {"method", required_argument, 0, 'm'},
    {"threads", required_argument, 0, 't'},
    {"size", required_argument, 0, 's'},
    {"psnr", required_argument, 0, 'p'},
    {"sns", required_argument, 0, 0},
    {"filter", required_argument, 0, 0},
    {"sharpness", required_argument, 0, 0},
    {"strong", no_argument, 0, 0},
    {"nostrong", no_argument, 0, 0},
    {"autofilter", no_argument, 0, 0},
    {"alpha-q", required_argument, 0, 0},
    {"alpha-filter", required_argument, 0, 0},
    {"pass", required_argument, 0, 0},
    {"preprocessing", required_argument, 0, 0},
    {"segments", required_argument, 0, 0},
    {"partition-limit", required_argument, 0, 0},
    {"sharp-yuv", no_argument, 0, 0},
    {"near-lossless", required_argument, 0, 0},
    {"low-memory", no_argument, 0, 0},
    {"exact", no_argument, 0, 0},
    {"delta-palette", no_argument, 0, 0},
    {"speed", required_argument, 0, 0},
    {"strip-safe", no_argument, 0, 0},
    {"no-strip-safe", no_argument, 0, 0},
    {"optimize-alpha", no_argument, 0, 0},
    {"no-optimize-alpha", no_argument, 0, 0},
    {"lossy", no_argument, 0, 0},
    {"max-colors", required_argument, 0, 0},
    {"reduced-colors", required_argument, 0, 0},
    {"reduce-bits-rgb", required_argument, 0, 0},
    {"reduce-alpha", required_argument, 0, 0},
    {"dither", required_argument, 0, 0},
    {"smooth-cutoff", required_argument, 0, 0},
    {"gradient-profile", no_argument, 0, 0},
    {"no-gradient-profile", no_argument, 0, 0},
    {"gradient-dither-floor", required_argument, 0, 0},
    {"gradient-opaque-threshold", required_argument, 0, 0},
    {"gradient-mean-max", required_argument, 0, 0},
    {"gradient-sat-mean-max", required_argument, 0, 0},
    {"tune-opaque-threshold", required_argument, 0, 0},
    {"tune-gradient-mean-max", required_argument, 0, 0},
    {"tune-sat-mean-max", required_argument, 0, 0},
    {"tune-speed-max", required_argument, 0, 0},
    {"tune-quality-min-floor", required_argument, 0, 0},
    {"tune-quality-max-target", required_argument, 0, 0},
    {"alpha-bleed", no_argument, 0, 0},
    {"no-alpha-bleed", no_argument, 0, 0},
    {"alpha-bleed-max-distance", required_argument, 0, 0},
    {"alpha-bleed-opaque-threshold", required_argument, 0, 0},
    {"alpha-bleed-soft-limit", required_argument, 0, 0},
    {"protect-color", required_argument, 0, 0},
    {0, 0, 0, 0}};

static const char *describe_pngx_type(int type) {
  switch (type) {
  case CPRES_PNGX_LOSSY_TYPE_PALETTE256:
    return "Palette (256 colors)";
  case CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444:
    return "Limited RGBA4444 (4 bits/channel)";
  case CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32:
    return "Reduced RGBA32 (targeted colors)";
  default:
    break;
  }

  return "Unknown";
}

static inline bool parse_pngx_type_option(const char *value, int *type_out) {
  char lowered[32], normalized[32];
  size_t len, i, norm_len = 0;

  if (!value || !type_out) {
    return false;
  }

  len = strlen(value);
  if (len == 0 || len >= sizeof(lowered)) {
    return false;
  }

  for (i = 0; i < len; ++i) {
    lowered[i] = (char)tolower((unsigned char)value[i]);
  }
  lowered[len] = '\0';

  for (i = 0; i < len && norm_len + 1 < sizeof(normalized); ++i) {
    if (lowered[i] == '_' || lowered[i] == '-') {
      continue;
    }
    normalized[norm_len++] = lowered[i];
  }
  normalized[norm_len] = '\0';

  if (strcmp(normalized, "palette256") == 0) {
    *type_out = CPRES_PNGX_LOSSY_TYPE_PALETTE256;
    return true;
  }
  if (strcmp(normalized, "limitedrgba16bit") == 0 || strcmp(normalized, "limited") == 0) {
    *type_out = CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444;
    return true;
  }
  if (strcmp(normalized, "reducedrgba32") == 0 || strcmp(normalized, "reduced") == 0) {
    *type_out = CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32;
    return true;
  }

  return false;
}

static inline void format_version(uint32_t version, char *buf, size_t buf_size) {
  uint32_t major, minor, patch;

  major = version / 1000000;
  minor = (version % 1000000) / 1000;
  patch = version % 1000;

  snprintf(buf, buf_size, "%u.%u.%u", major, minor, patch);
}

static inline void format_libavif_version(uint32_t version, char *buf, size_t buf_size) {
  uint32_t major, minor, patch; 

  if (version == 0) {
    snprintf(buf, buf_size, "unknown");
    return;
  }

  major = version / 1000000;
  minor = (version % 1000000) / 10000;
  patch = (version % 10000) / 100;

  snprintf(buf, buf_size, "%u.%u.%u", major, minor, patch);
}

static inline void format_webp_version(uint32_t version, char *buf, size_t buf_size) {
  uint32_t major, minor, patch;

  major = (version >> 16) & 0xff;
  minor = (version >> 8) & 0xff;
  patch = version & 0xff;

  snprintf(buf, buf_size, "%u.%u.%u", major, minor, patch);
}

static inline void format_libpng_version(uint32_t version, char *buf, size_t buf_size) {
  uint32_t major, minor, patch;

  major = version / 10000;
  minor = (version % 10000) / 100;
  patch = version % 100;

  snprintf(buf, buf_size, "%u.%u.%u", major, minor, patch);
}

static inline void format_bytes(int64_t bytes, char *buf, size_t buf_size) {
  const double k = 1024.0;
  const char *sizes[] = {"B", "KiB", "MiB", "GiB"};
  double size = (double)bytes;
  int32_t i = 0;

  if (bytes == 0) {
    snprintf(buf, buf_size, "0 B");
    return;
  }

  while (size >= k && i < 3) {
    size /= k;
    i++;
  }

  snprintf(buf, buf_size, "%.1f %s", size, sizes[i]);
}

static inline int64_t get_file_size(const char *path) {
  struct stat st;

  if (stat(path, &st) == 0) {
    return (int64_t)st.st_size;
  }

  return -1;
}

static inline bool write_file_from_memory(const char *path, const uint8_t *data, size_t size) {
  FILE *fp;
  size_t written;

  if (!path || !data || size == 0) {
    return false;
  }

  fp = fopen(path, "wb");
  if (!fp) {
    return false;
  }

  written = fwrite(data, 1, size, fp);
  fclose(fp);

  return written == size;
}

static inline void print_output_larger_warning(const char *format_name, int64_t input_size, size_t output_size) {
  int64_t safe_input = input_size > 0 ? input_size : (int64_t)output_size, output_bytes = (int64_t)output_size;
  char input_size_buf[32], output_size_buf[32];
  double ratio, increase;

  if (safe_input <= 0) {
    safe_input = (int64_t)output_size;
  }

  format_bytes(safe_input, input_size_buf, sizeof(input_size_buf));
  format_bytes(output_bytes, output_size_buf, sizeof(output_size_buf));

  if (safe_input > 0) {
    ratio = ((double)output_size / (double)safe_input) * 100.0;
    increase = ratio - 100.0;
    fprintf(stderr,
            "Warning: %s output would be larger than equal input: %s -> %s (%.1f%%, increased by %.1f%%)\n",
            format_name ? format_name : "Output", input_size_buf, output_size_buf, ratio, increase);
  } else {
    fprintf(stderr,
            "Warning: %s output would be larger than equal input: %s -> %s\n",
            format_name ? format_name : "Output", input_size_buf, output_size_buf);
  }
}

static inline bool parse_long_value(const char *str, long *value) {
  char *endptr = NULL;
  long parsed;

  if (!str || !value) {
    return false;
  }

  errno = 0;
  parsed = strtol(str, &endptr, 10);
  if (errno != 0 || endptr == str || *endptr != '\0') {
    return false;
  }

  *value = parsed;

  return true;
}

static inline bool parse_long_range(const char *str, long min_value, long max_value, long *value) {
  long parsed;

  if (!parse_long_value(str, &parsed)) {
    return false;
  }

  if (parsed < min_value || parsed > max_value) {
    return false;
  }

  if (value) {
    *value = parsed;
  }

  return true;
}

static inline bool parse_double_value(const char *str, double *value) {
  char *endptr = NULL;
  double parsed;

  if (!str || !value) {
    return false;
  }

  errno = 0;
  parsed = strtod(str, &endptr);
  if (errno != 0 || endptr == str || *endptr != '\0') {
    return false;
  }

  *value = parsed;

  return true;
}

static inline bool parse_double_range(const char *str, double min_value, double max_value, double *value) {
  double parsed;

  if (!parse_double_value(str, &parsed)) {
    return false;
  }

  if (parsed < min_value || parsed > max_value) {
    return false;
  }

  if (value) {
    *value = parsed;
  }

  return true;
}

static inline bool parse_hex_color(const char *hex_str, cpres_rgba_color_t *color) {
  const char *str = hex_str;
  size_t len;
  uint32_t r, g, b, a = 255;

  if (str[0] == '#') {
    str++;
  }

  len = strlen(str);

  if (len != 6 && len != 8) {
    return false;
  }

  if (sscanf(str, "%2x%2x%2x", &r, &g, &b) != 3) {
    return false;
  }

  if (len == 8) {
    if (sscanf(str + 6, "%2x", &a) != 1) {
      return false;
    }
  }

  color->r = (uint8_t)r;
  color->g = (uint8_t)g;
  color->b = (uint8_t)b;
  color->a = (uint8_t)a;

  return true;
}

static inline int32_t parse_protected_colors(const char *color_list, cpres_rgba_color_t **colors_out) {
  int32_t count = 0, i = 0;
  char *token, *saveptr, *list_copy_tmp = NULL, *list_copy = NULL;
  cpres_rgba_color_t *colors;

  if (!color_list || color_list[0] == '\0' || !colors_out) {
    return 0;
  }

  list_copy_tmp = strdup(color_list);
  if (!list_copy_tmp) {
    return -1;
  }

  list_copy = strdup(color_list);
  if (!list_copy) {
    free(list_copy_tmp);
    return -1;
  }

  token = strtok_r(list_copy_tmp, ",", &saveptr);
  while (token != NULL) {
    count++;
    if (count > 256) {
      fprintf(stderr, "Error: Too many protected colors (max 256)\n");
      free(list_copy_tmp);
      free(list_copy);
      return -1;
    }
    token = strtok_r(NULL, ",", &saveptr);
  }
  free(list_copy_tmp);

  if (count == 0) {
    free(list_copy);
    return 0;
  }

  colors = (cpres_rgba_color_t *)malloc(sizeof(cpres_rgba_color_t) * count);
  if (!colors) {
    free(list_copy);
    return -1;
  }

  token = strtok_r(list_copy, ",", &saveptr);
  while (token != NULL && i < count) {
    while (*token == ' ' || *token == '\t') {
      token++;
    }

    if (!parse_hex_color(token, &colors[i])) {
      fprintf(stderr, "Error: Invalid color format '%s' (use RRGGBB or RRGGBBAA hex format)\n", token);
      free(colors);
      free(list_copy);
      return -1;
    }

    i++;
    token = strtok_r(NULL, ",", &saveptr);
  }

  free(list_copy);
  *colors_out = colors;

  return count;
}

static inline bool parse_quality_range(const char *input, int32_t *min_quality, int32_t *max_quality) {
  int32_t qmin = -1, qmax = -1, tmp;

  if (!input || !min_quality || !max_quality) {
    return false;
  }

  if (sscanf(input, "%" SCNd32 "-%" SCNd32, &qmin, &qmax) != 2) {
    return false;
  }

  if (qmin < 0 || qmin > 100 || qmax < 0 || qmax > 100) {
    return false;
  }

  if (qmax < qmin) {
    tmp = qmin;
    qmin = qmax;
    qmax = tmp;
  }

  *min_quality = qmin;
  *max_quality = qmax;

  return true;
}

static inline output_format_t parse_format(const char *format_str) {
  if (strcmp(format_str, "webp") == 0) {
    return FORMAT_WEBP;
  } else if (strcmp(format_str, "avif") == 0) {
    return FORMAT_AVIF;
  } else if (strcmp(format_str, "pngx") == 0 || strcmp(format_str, "png") == 0) {
    return FORMAT_PNGX;
  }

  return FORMAT_UNKNOWN;
}

static inline output_format_t infer_format_from_extension(const char *path) {
  const char *ext = colopresso_extract_extension(path);
  char lower_ext[16];
  size_t i, len;

  if (!ext) {
    return FORMAT_UNKNOWN;
  }

  len = strlen(ext);
  if (len == 0 || len >= sizeof(lower_ext)) {
    return FORMAT_UNKNOWN;
  }

  for (i = 0; i < len; i++) {
    lower_ext[i] = (char)tolower((unsigned char)ext[i]);
  }
  lower_ext[len] = '\0';

  if (strcmp(lower_ext, ".webp") == 0) {
    return FORMAT_WEBP;
  }
  if (strcmp(lower_ext, ".avif") == 0) {
    return FORMAT_AVIF;
  }
  if (strcmp(lower_ext, ".png") == 0) {
    return FORMAT_PNGX;
  }

  return FORMAT_UNKNOWN;
}

static inline bool path_has_extension_ci(const char *path, const char *extension) {
  size_t path_len = strlen(path), ext_len = strlen(extension), i;
  const char *path_ext = path + path_len - ext_len;

  if (ext_len == 0 || path_len < ext_len) {
    return false;
  }
  
  for (i = 0; i < ext_len; i++) {
    if (tolower((unsigned char)path_ext[i]) != tolower((unsigned char)extension[i])) {
      return false;
    }
  }

  return true;
}

static inline const char *get_extension(output_format_t format) {
  switch (format) {
  case FORMAT_WEBP:
    return ".webp";
  case FORMAT_AVIF:
    return ".avif";
  case FORMAT_PNGX:
    return ".png";
  default:
    return "";
  }
}

static inline const char *get_format_name(output_format_t format) {
  switch (format) {
  case FORMAT_WEBP:
    return "WebP";
  case FORMAT_AVIF:
    return "AVIF";
  case FORMAT_PNGX:
    return "PNGX";
  default:
    return "Unknown";
  }
}

static inline char *build_output_path(const char *output_base, output_format_t format) {
  const char *ext = get_extension(format);
  size_t len = strlen(output_base) + strlen(ext) + 1;
  char *output_path = malloc(len);

  if (!output_path) {
    return NULL;
  }

  snprintf(output_path, len, "%s%s", output_base, ext);

  return output_path;
}

static inline bool should_append_extension(const char *output_base, output_format_t format, bool format_specified) {
  const char *existing_extension, *expected_extension;
  output_format_t inferred;

  expected_extension = get_extension(format);
  existing_extension = colopresso_extract_extension(output_base);
  inferred = infer_format_from_extension(output_base);

  if (!existing_extension) {
    return true;
  }
  if (path_has_extension_ci(output_base, expected_extension)) {
    return false;
  }
  if (!format_specified && inferred == format) {
    return false;
  }
  if (format_specified && inferred != FORMAT_UNKNOWN && inferred != format) {
    return false;
  }

  return true;
}

static inline void print_verbose_summary(const cpres_config_t *config, output_format_t format, const char *input_file,
                                  const char *output_file, int64_t input_size,
                                  const cpres_rgba_color_t *protected_colors, int32_t protected_colors_count) {
  char version_buf[32];
  char size_buf[32];
  int32_t i;
  bool limited_mode, reduced_mode, palette_mode;

  printf("Converting: %s -> %s\n", input_file, output_file);
  printf("Format: %s\n", get_format_name(format));

  if (input_size >= 0) {
    format_bytes(input_size, size_buf, sizeof(size_buf));
    printf("Input size: %s\n", size_buf);
  }

  if (format == FORMAT_WEBP) {
    format_webp_version(cpres_get_libwebp_version(), version_buf, sizeof(version_buf));
    printf("Using libwebp: v%s\n", version_buf);
  } else if (format == FORMAT_AVIF) {
    format_libavif_version(cpres_get_libavif_version(), version_buf, sizeof(version_buf));
    printf("Using libavif: v%s\n", version_buf);
  }

  if (format == FORMAT_WEBP) {
    printf("Settings:\n");
    printf("  Quality: %.1f\n", config->webp_quality);
    printf("  Lossless: %s\n", config->webp_lossless ? "yes" : "no");
    printf("  Method: %d\n", config->webp_method);
    if (config->webp_target_size > 0) {
      printf("  Target size: %d bytes\n", config->webp_target_size);
    }
  } else if (format == FORMAT_AVIF) {
    printf("Settings:\n");
    if (config->avif_lossless) {
      printf("  Lossless: yes\n");
    } else {
      printf("  Quality: %.1f\n", config->avif_quality);
    }
    printf("  Speed: %d\n", config->avif_speed);
    printf("  Threads: %d\n", config->avif_threads);
  }
  else if (format == FORMAT_PNGX) {
    printf("Settings:\n");
    printf("  Optimization level: %d\n", config->pngx_level);
    printf("  Strip safe chunks: %s\n", config->pngx_strip_safe ? "yes" : "no");
    printf("  Optimize alpha: %s\n", config->pngx_optimize_alpha ? "yes" : "no");
    printf("  Lossy quantization: %s\n", config->pngx_lossy_enable ? "enabled" : "disabled");
    if (config->pngx_lossy_enable) {
      limited_mode = (config->pngx_lossy_type == CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444);
      reduced_mode = (config->pngx_lossy_type == CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32);
      palette_mode = (config->pngx_lossy_type == CPRES_PNGX_LOSSY_TYPE_PALETTE256);
      printf("    Mode: %s\n", describe_pngx_type(config->pngx_lossy_type));
      if (palette_mode) {
        printf("    Max colors: %d\n", config->pngx_lossy_max_colors);
      } else if (reduced_mode) {
        if (config->pngx_lossy_reduced_colors < 0) {
          printf("    Reduced colors: auto\n");
        } else {
          printf("    Reduced colors: %d\n", config->pngx_lossy_reduced_colors);
        }
        printf("    Grid bits: RGB %d / Alpha %d\n", config->pngx_lossy_reduced_bits_rgb, config->pngx_lossy_reduced_alpha_bits);
      }
      printf("    Quality range: %d-%d\n", config->pngx_lossy_quality_min, config->pngx_lossy_quality_max);
      printf("    Speed: %d\n", config->pngx_lossy_speed);
      if (!reduced_mode) {
        if (config->pngx_lossy_dither_level < 0.0f) {
          printf("    Dither level: auto (-1)%s\n", limited_mode ? " (Limited heuristic)" : "");
        } else if (limited_mode) {
          printf("    Dither level: %.2f (manual override)\n", config->pngx_lossy_dither_level);
        } else {
          printf("    Dither level: %.2f\n", config->pngx_lossy_dither_level);
        }
      } else {
        printf("    Dither level: n/a (not used)\n");
      }
      printf("    Saliency map: %s\n", config->pngx_saliency_map_enable ? "enabled" : "disabled");
      printf("    Chroma anchors: %s\n", config->pngx_chroma_anchor_enable ? "enabled" : "disabled");
      printf("    Adaptive dithering: %s\n", config->pngx_adaptive_dither_enable ? "enabled" : "disabled");
      printf("    Gradient boost: %s\n", config->pngx_gradient_boost_enable ? "enabled" : "disabled");
      printf("    Chroma weighting: %s\n", config->pngx_chroma_weight_enable ? "enabled" : "disabled");
      printf("    Palette smoothing: %s\n", config->pngx_postprocess_smooth_enable ? "enabled" : "disabled");
      if (config->pngx_postprocess_smooth_importance_cutoff < 0.0f) {
        printf("    Palette smoothing importance cutoff: disabled\n");
      } else {
        printf("    Palette smoothing importance cutoff: %.2f\n", config->pngx_postprocess_smooth_importance_cutoff);
      }

      if (palette_mode) {
        printf("    Gradient profile: %s\n", config->pngx_palette256_gradient_profile_enable ? "enabled" : "disabled");
        if (config->pngx_palette256_gradient_dither_floor < 0.0f) {
          printf("    Gradient dither floor: internal default\n");
        } else {
          printf("    Gradient dither floor: %.2f\n", config->pngx_palette256_gradient_dither_floor);
        }
        printf("    Alpha bleed: %s\n", config->pngx_palette256_alpha_bleed_enable ? "enabled" : "disabled");
        printf("    Alpha bleed max distance: %d\n", config->pngx_palette256_alpha_bleed_max_distance);
        printf("    Alpha bleed opaque threshold: %d\n", config->pngx_palette256_alpha_bleed_opaque_threshold);
        printf("    Alpha bleed soft limit: %d\n", config->pngx_palette256_alpha_bleed_soft_limit);

        if (config->pngx_palette256_profile_opaque_ratio_threshold < 0.0f) {
          printf("    Gradient profile opaque ratio threshold: internal default\n");
        } else {
          printf("    Gradient profile opaque ratio threshold: %.3f\n", config->pngx_palette256_profile_opaque_ratio_threshold);
        }
        if (config->pngx_palette256_profile_gradient_mean_max < 0.0f) {
          printf("    Gradient profile gradient mean max: internal default\n");
        } else {
          printf("    Gradient profile gradient mean max: %.3f\n", config->pngx_palette256_profile_gradient_mean_max);
        }
        if (config->pngx_palette256_profile_saturation_mean_max < 0.0f) {
          printf("    Gradient profile saturation mean max: internal default\n");
        } else {
          printf("    Gradient profile saturation mean max: %.3f\n", config->pngx_palette256_profile_saturation_mean_max);
        }

        if (config->pngx_palette256_tune_opaque_ratio_threshold < 0.0f) {
          printf("    Tune opaque ratio threshold: internal default\n");
        } else {
          printf("    Tune opaque ratio threshold: %.3f\n", config->pngx_palette256_tune_opaque_ratio_threshold);
        }
        if (config->pngx_palette256_tune_gradient_mean_max < 0.0f) {
          printf("    Tune gradient mean max: internal default\n");
        } else {
          printf("    Tune gradient mean max: %.3f\n", config->pngx_palette256_tune_gradient_mean_max);
        }
        if (config->pngx_palette256_tune_saturation_mean_max < 0.0f) {
          printf("    Tune saturation mean max: internal default\n");
        } else {
          printf("    Tune saturation mean max: %.3f\n", config->pngx_palette256_tune_saturation_mean_max);
        }

        if (config->pngx_palette256_tune_speed_max < 0) {
          printf("    Tune speed max: internal default\n");
        } else {
          printf("    Tune speed max: %d\n", config->pngx_palette256_tune_speed_max);
        }
        if (config->pngx_palette256_tune_quality_min_floor < 0) {
          printf("    Tune quality min floor: internal default\n");
        } else {
          printf("    Tune quality min floor: %d\n", config->pngx_palette256_tune_quality_min_floor);
        }
        if (config->pngx_palette256_tune_quality_max_target < 0) {
          printf("    Tune quality max target: internal default\n");
        } else {
          printf("    Tune quality max target: %d\n", config->pngx_palette256_tune_quality_max_target);
        }
      }
    }
    if (protected_colors_count > 0) {
      printf("    Protected colors: %d\n", (int)protected_colors_count);
      for (i = 0; i < protected_colors_count && i < 5; i++) {
        printf("      #%02X%02X%02X%02X\n", protected_colors[i].r, protected_colors[i].g, protected_colors[i].b,
               protected_colors[i].a);
      }
      if (protected_colors_count > 5) {
        printf("      ... and %d more\n", (int)(protected_colors_count - 5));
      }
    }
  }
}

static inline void print_conversion_success(int64_t input_size, int64_t output_size) {
  char input_size_buf[32], output_size_buf[32];
  double ratio, reduction;

  printf("Conversion successful!\n");

  if (input_size >= 0 && output_size >= 0) {
    format_bytes(input_size, input_size_buf, sizeof(input_size_buf));
    format_bytes(output_size, output_size_buf, sizeof(output_size_buf));

    ratio = ((double)output_size / (double)input_size) * 100.0;
    reduction = 100.0 - ratio;

    printf("\nSize comparison:\n");
    printf("  Input:  %s\n", input_size_buf);
    printf("  Output: %s\n", output_size_buf);
    printf("  Ratio:  %.1f%% ", ratio);
    if (reduction > 0) {
      printf("(reduced by %.1f%%)\n", reduction);
    } else if (reduction < 0) {
      printf("(increased by %.1f%%)\n", -reduction);
    } else {
      printf("(no change)\n");
    }
  }
}

static inline int32_t handle_size_increase_error(int64_t input_size, int64_t output_size) {
  double ratio, reduction;
  char input_size_buf[32], output_size_buf[32];

  if (input_size >= 0 && output_size >= 0) {
    ratio = ((double)output_size / (double)input_size) * 100.0;
    reduction = 100.0 - ratio;

    if (reduction < 0) {
      format_bytes(input_size, input_size_buf, sizeof(input_size_buf));
      format_bytes(output_size, output_size_buf, sizeof(output_size_buf));

      fprintf(stderr,
              "Error: Output size increased: %s -> %s (%.1f%%, increased by %.1f%%)\n",
              input_size_buf, output_size_buf, ratio, -reduction);
      return 2;
    }
  }

  return 0;
}

static inline bool handle_long_option(const char *name, const char *optarg, cpres_config_t *config,
                               cpres_rgba_color_t **protected_colors, int32_t *protected_colors_count,
                               bool *dither_specified) {
  long long_val;
  double double_val;
  int32_t count;
  cpres_rgba_color_t *parsed_colors;

  if (strcmp(name, "sns") == 0) {
    if (!parse_long_range(optarg, 0, 100, &long_val)) {
      fprintf(stderr, "Error: Invalid sns value (must be 0-100)\n");
      return false;
    }
    config->webp_sns_strength = (int)long_val;
    return true;
  }

  if (strcmp(name, "filter") == 0) {
    if (!parse_long_range(optarg, 0, 100, &long_val)) {
      fprintf(stderr, "Error: Invalid filter strength (must be 0-100)\n");
      return false;
    }
    config->webp_filter_strength = (int)long_val;
    return true;
  }

  if (strcmp(name, "sharpness") == 0) {
    if (!parse_long_range(optarg, 0, 7, &long_val)) {
      fprintf(stderr, "Error: Invalid sharpness (must be 0-7)\n");
      return false;
    }
    config->webp_filter_sharpness = (int)long_val;
    return true;
  }

  if (strcmp(name, "strong") == 0) {
    config->webp_filter_type = 1;
    return true;
  }

  if (strcmp(name, "nostrong") == 0) {
    config->webp_filter_type = 0;
    return true;
  }

  if (strcmp(name, "autofilter") == 0) {
    config->webp_autofilter = true;
    return true;
  }

  if (strcmp(name, "alpha-q") == 0) {
    if (!parse_long_range(optarg, 0, 100, &long_val)) {
      fprintf(stderr, "Error: Invalid alpha quality (must be 0-100)\n");
      return false;
    }
    config->webp_alpha_quality = (int)long_val;
    config->avif_alpha_quality = (int)long_val;
    return true;
  }

  if (strcmp(name, "alpha-filter") == 0) {
    if (!parse_long_range(optarg, 0, 2, &long_val)) {
      fprintf(stderr, "Error: Invalid alpha filtering (must be 0-2)\n");
      return false;
    }
    config->webp_alpha_filtering = (int)long_val;
    return true;
  }

  if (strcmp(name, "pass") == 0) {
    if (!parse_long_range(optarg, 1, 10, &long_val)) {
      fprintf(stderr, "Error: Invalid pass count (must be 1-10)\n");
      return false;
    }
    config->webp_pass = (int)long_val;
    return true;
  }

  if (strcmp(name, "preprocessing") == 0) {
    if (!parse_long_range(optarg, 0, 2, &long_val)) {
      fprintf(stderr, "Error: Invalid preprocessing (must be 0-2)\n");
      return false;
    }
    config->webp_preprocessing = (int)long_val;
    return true;
  }

  if (strcmp(name, "segments") == 0) {
    if (!parse_long_range(optarg, 1, 4, &long_val)) {
      fprintf(stderr, "Error: Invalid segments (must be 1-4)\n");
      return false;
    }
    config->webp_segments = (int)long_val;
    return true;
  }

  if (strcmp(name, "partition-limit") == 0) {
    if (!parse_long_range(optarg, 0, 100, &long_val)) {
      fprintf(stderr, "Error: Invalid partition limit (must be 0-100)\n");
      return false;
    }
    config->webp_partition_limit = (int)long_val;
    return true;
  }

  if (strcmp(name, "sharp-yuv") == 0) {
    config->webp_use_sharp_yuv = true;
    return true;
  }

  if (strcmp(name, "near-lossless") == 0) {
    if (!parse_long_range(optarg, 0, 100, &long_val)) {
      fprintf(stderr, "Error: Invalid near-lossless level (must be 0-100)\n");
      return false;
    }
    config->webp_near_lossless = (int)long_val;
    return true;
  }

  if (strcmp(name, "low-memory") == 0) {
    config->webp_low_memory = true;
    return true;
  }

  if (strcmp(name, "exact") == 0) {
    config->webp_exact = true;
    return true;
  }

  if (strcmp(name, "delta-palette") == 0) {
    config->webp_use_delta_palette = true;
    return true;
  }

  if (strcmp(name, "speed") == 0) {
    if (!parse_long_range(optarg, 0, 10, &long_val)) {
      fprintf(stderr, "Error: Invalid speed (must be 0-10)\n");
      return false;
    }
    config->avif_speed = (int)long_val;
    config->pngx_lossy_speed = (int)long_val;
    return true;
  }

  if (strcmp(name, "strip-safe") == 0) {
    config->pngx_strip_safe = true;
    return true;
  }

  if (strcmp(name, "no-strip-safe") == 0) {
    config->pngx_strip_safe = false;
    return true;
  }

  if (strcmp(name, "optimize-alpha") == 0) {
    config->pngx_optimize_alpha = true;
    return true;
  }

  if (strcmp(name, "no-optimize-alpha") == 0) {
    config->pngx_optimize_alpha = false;
    return true;
  }

  if (strcmp(name, "lossy") == 0) {
    config->pngx_lossy_enable = true;
    return true;
  }

  if (strcmp(name, "max-colors") == 0) {
    if (!parse_long_range(optarg, 2, 256, &long_val)) {
      fprintf(stderr, "Error: Invalid max-colors (must be 2-256)\n");
      return false;
    }
    config->pngx_lossy_max_colors = (int)long_val;
    return true;
  }

  if (strcmp(name, "reduced-colors") == 0) {
    if (!parse_long_value(optarg, &long_val)) {
      fprintf(stderr, "Error: Invalid reduced-colors (must be -1 or %d-%d)\n", COLOPRESSO_PNGX_REDUCED_COLORS_MIN,
              COLOPRESSO_PNGX_REDUCED_COLORS_MAX);
      return false;
    }
    if (long_val == COLOPRESSO_PNGX_DEFAULT_REDUCED_COLORS) {
      config->pngx_lossy_reduced_colors = COLOPRESSO_PNGX_DEFAULT_REDUCED_COLORS;
      return true;
    }
    if (long_val < COLOPRESSO_PNGX_REDUCED_COLORS_MIN || long_val > COLOPRESSO_PNGX_REDUCED_COLORS_MAX) {
      fprintf(stderr, "Error: Invalid reduced-colors (must be -1 or %d-%d)\n", COLOPRESSO_PNGX_REDUCED_COLORS_MIN,
              COLOPRESSO_PNGX_REDUCED_COLORS_MAX);
      return false;
    }
    config->pngx_lossy_reduced_colors = (int)long_val;
    return true;
  }

  if (strcmp(name, "reduce-bits-rgb") == 0) {
    if (!parse_long_range(optarg, COLOPRESSO_PNGX_REDUCED_BITS_MIN, COLOPRESSO_PNGX_REDUCED_BITS_MAX, &long_val)) {
      fprintf(stderr, "Error: Invalid reduce-bits-rgb (must be %d-%d)\n", COLOPRESSO_PNGX_REDUCED_BITS_MIN, COLOPRESSO_PNGX_REDUCED_BITS_MAX);
      return false;
    }
    config->pngx_lossy_reduced_bits_rgb = (int)long_val;
    return true;
  }

  if (strcmp(name, "reduce-alpha") == 0) {
    if (!parse_long_range(optarg, COLOPRESSO_PNGX_REDUCED_BITS_MIN, COLOPRESSO_PNGX_REDUCED_BITS_MAX, &long_val)) {
      fprintf(stderr, "Error: Invalid reduce-alpha (must be %d-%d)\n", COLOPRESSO_PNGX_REDUCED_BITS_MIN, COLOPRESSO_PNGX_REDUCED_BITS_MAX);
      return false;
    }
    config->pngx_lossy_reduced_alpha_bits = (int)long_val;
    return true;
  }

  if (strcmp(name, "dither") == 0) {
    if (!parse_double_value(optarg, &double_val)) {
      fprintf(stderr, "Error: Invalid dither level (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    if (!(double_val == -1.0 || (double_val >= 0.0 && double_val <= 1.0))) {
      fprintf(stderr, "Error: Invalid dither level (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    config->pngx_lossy_dither_level = (float)double_val;
    if (dither_specified) {
      *dither_specified = true;
    }
    return true;
  }

  if (strcmp(name, "smooth-cutoff") == 0) {
    if (!parse_double_value(optarg, &double_val)) {
      fprintf(stderr, "Error: Invalid smooth-cutoff (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    if (double_val != -1.0 && (double_val < 0.0 || double_val > 1.0)) {
      fprintf(stderr, "Error: Invalid smooth-cutoff (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    config->pngx_postprocess_smooth_importance_cutoff = (float)double_val;
    return true;
  }

  if (strcmp(name, "gradient-profile") == 0) {
    config->pngx_palette256_gradient_profile_enable = true;
    return true;
  }

  if (strcmp(name, "no-gradient-profile") == 0) {
    config->pngx_palette256_gradient_profile_enable = false;
    return true;
  }

  if (strcmp(name, "gradient-dither-floor") == 0) {
    if (!parse_double_value(optarg, &double_val)) {
      fprintf(stderr, "Error: Invalid gradient-dither-floor (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    if (double_val != -1.0 && (double_val < 0.0 || double_val > 1.0)) {
      fprintf(stderr, "Error: Invalid gradient-dither-floor (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    config->pngx_palette256_gradient_dither_floor = (float)double_val;
    return true;
  }

  if (strcmp(name, "gradient-opaque-threshold") == 0) {
    if (!parse_double_value(optarg, &double_val)) {
      fprintf(stderr, "Error: Invalid gradient-opaque-threshold (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    if (double_val != -1.0 && (double_val < 0.0 || double_val > 1.0)) {
      fprintf(stderr, "Error: Invalid gradient-opaque-threshold (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    config->pngx_palette256_profile_opaque_ratio_threshold = (float)double_val;
    return true;
  }

  if (strcmp(name, "gradient-mean-max") == 0) {
    if (!parse_double_value(optarg, &double_val)) {
      fprintf(stderr, "Error: Invalid gradient-mean-max (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    if (double_val != -1.0 && (double_val < 0.0 || double_val > 1.0)) {
      fprintf(stderr, "Error: Invalid gradient-mean-max (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    config->pngx_palette256_profile_gradient_mean_max = (float)double_val;
    return true;
  }

  if (strcmp(name, "gradient-sat-mean-max") == 0) {
    if (!parse_double_value(optarg, &double_val)) {
      fprintf(stderr, "Error: Invalid gradient-sat-mean-max (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    if (double_val != -1.0 && (double_val < 0.0 || double_val > 1.0)) {
      fprintf(stderr, "Error: Invalid gradient-sat-mean-max (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    config->pngx_palette256_profile_saturation_mean_max = (float)double_val;
    return true;
  }

  if (strcmp(name, "tune-opaque-threshold") == 0) {
    if (!parse_double_value(optarg, &double_val)) {
      fprintf(stderr, "Error: Invalid tune-opaque-threshold (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    if (double_val != -1.0 && (double_val < 0.0 || double_val > 1.0)) {
      fprintf(stderr, "Error: Invalid tune-opaque-threshold (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    config->pngx_palette256_tune_opaque_ratio_threshold = (float)double_val;
    return true;
  }

  if (strcmp(name, "tune-gradient-mean-max") == 0) {
    if (!parse_double_value(optarg, &double_val)) {
      fprintf(stderr, "Error: Invalid tune-gradient-mean-max (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    if (double_val != -1.0 && (double_val < 0.0 || double_val > 1.0)) {
      fprintf(stderr, "Error: Invalid tune-gradient-mean-max (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    config->pngx_palette256_tune_gradient_mean_max = (float)double_val;
    return true;
  }

  if (strcmp(name, "tune-sat-mean-max") == 0) {
    if (!parse_double_value(optarg, &double_val)) {
      fprintf(stderr, "Error: Invalid tune-sat-mean-max (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    if (double_val != -1.0 && (double_val < 0.0 || double_val > 1.0)) {
      fprintf(stderr, "Error: Invalid tune-sat-mean-max (must be -1.0 or within 0.0-1.0)\n");
      return false;
    }
    config->pngx_palette256_tune_saturation_mean_max = (float)double_val;
    return true;
  }

  if (strcmp(name, "tune-speed-max") == 0) {
    if (!parse_long_value(optarg, &long_val)) {
      fprintf(stderr, "Error: Invalid tune-speed-max (must be -1 or within 1-10)\n");
      return false;
    }
    if (!(long_val == -1 || (long_val >= 1 && long_val <= 10))) {
      fprintf(stderr, "Error: Invalid tune-speed-max (must be -1 or within 1-10)\n");
      return false;
    }
    config->pngx_palette256_tune_speed_max = (int)long_val;
    return true;
  }

  if (strcmp(name, "tune-quality-min-floor") == 0) {
    if (!parse_long_value(optarg, &long_val)) {
      fprintf(stderr, "Error: Invalid tune-quality-min-floor (must be -1 or within 0-100)\n");
      return false;
    }
    if (!(long_val == -1 || (long_val >= 0 && long_val <= 100))) {
      fprintf(stderr, "Error: Invalid tune-quality-min-floor (must be -1 or within 0-100)\n");
      return false;
    }
    config->pngx_palette256_tune_quality_min_floor = (int)long_val;
    return true;
  }

  if (strcmp(name, "tune-quality-max-target") == 0) {
    if (!parse_long_value(optarg, &long_val)) {
      fprintf(stderr, "Error: Invalid tune-quality-max-target (must be -1 or within 0-100)\n");
      return false;
    }
    if (!(long_val == -1 || (long_val >= 0 && long_val <= 100))) {
      fprintf(stderr, "Error: Invalid tune-quality-max-target (must be -1 or within 0-100)\n");
      return false;
    }
    config->pngx_palette256_tune_quality_max_target = (int)long_val;
    return true;
  }

  if (strcmp(name, "alpha-bleed") == 0) {
    config->pngx_palette256_alpha_bleed_enable = true;
    return true;
  }

  if (strcmp(name, "no-alpha-bleed") == 0) {
    config->pngx_palette256_alpha_bleed_enable = false;
    return true;
  }

  if (strcmp(name, "alpha-bleed-max-distance") == 0) {
    if (!parse_long_range(optarg, 0, 65535, &long_val)) {
      fprintf(stderr, "Error: Invalid alpha-bleed-max-distance (must be 0-65535)\n");
      return false;
    }
    config->pngx_palette256_alpha_bleed_max_distance = (int)long_val;
    return true;
  }

  if (strcmp(name, "alpha-bleed-opaque-threshold") == 0) {
    if (!parse_long_range(optarg, 0, 255, &long_val)) {
      fprintf(stderr, "Error: Invalid alpha-bleed-opaque-threshold (must be 0-255)\n");
      return false;
    }
    config->pngx_palette256_alpha_bleed_opaque_threshold = (int)long_val;
    return true;
  }

  if (strcmp(name, "alpha-bleed-soft-limit") == 0) {
    if (!parse_long_range(optarg, 0, 255, &long_val)) {
      fprintf(stderr, "Error: Invalid alpha-bleed-soft-limit (must be 0-255)\n");
      return false;
    }
    config->pngx_palette256_alpha_bleed_soft_limit = (int)long_val;
    return true;
  }

  if (strcmp(name, "protect-color") == 0) {
    parsed_colors = NULL;
    count = parse_protected_colors(optarg, &parsed_colors);

    if (count < 0) {
      fprintf(stderr, "Error: Failed to parse protected colors\n");
      return false;
    }

    if (*protected_colors) {
      free(*protected_colors);
    }

    *protected_colors = parsed_colors;
    *protected_colors_count = count;

    return true;
  }

  fprintf(stderr, "Error: Unknown option '--%s'\n", name);

  return false;
}

static inline void print_usage(const char *program_name) {
  printf("Usage: %s [--format=<format>] [OPTIONS] <input.png> <output>\n", program_name);
  printf("\nPNG converter and optimizer\n");
  printf("\nNote: The output file extension will be automatically determined from the format.\n");
  printf("\nFormat Selection:\n");
  printf("  --format=<format>           Output format: webp, avif, pngx (or png).\n");
  printf("                              Optional if output filename has .webp/.avif/.png extension.\n");
  printf("\nCommon Options:\n");
  printf("  -v, --verbose               Verbose output\n");
  printf("  -h, --help                  Show this help message\n");
  printf("  -V, --version               Show version information\n");
  printf("  -t, --threads <int>         Number of threads (>=0, default: all cores)\n");
  printf("  -l, --lossless              Use lossless compression\n");
  printf("\n=== WebP Options (--format=webp) ===\n");
  printf("  -q, --quality <float>       Set quality (0-100, default: 80)\n");
  printf("  -m, --method <int>          Compression method (0-6, default: 6)\n");
  printf("  -s, --size <int>            Target file size in bytes\n");
  printf("  -p, --psnr <float>          Target PSNR\n");
  printf("      --sns <int>             Spatial noise shaping (0-100, default: 50)\n");
  printf("      --filter <int>          Filter strength (0-100, default: 60)\n");
  printf("      --sharpness <int>       Filter sharpness (0-7, default: 0)\n");
  printf("      --strong                Use strong filter type\n");
  printf("      --nostrong              Use simple filter type\n");
  printf("      --autofilter            Auto-adjust filter parameters\n");
  printf("      --alpha-q <int>         Alpha quality (0-100, default: 100)\n");
  printf("      --alpha-filter <int>    Alpha filtering (0-2, default: 1)\n");
  printf("      --pass <int>            Number of entropy passes (1-10, default: 10)\n");
  printf("      --preprocessing <int>   Preprocessing filter (0-2, default: 2)\n");
  printf("      --segments <int>        Number of segments (1-4, default: 4)\n");
  printf("      --partition-limit <int> Quality degradation limit (0-100, default: 0)\n");
  printf("      --sharp-yuv             Use sharp YUV conversion\n");
  printf("      --near-lossless <int>   Near-lossless level (0-100, default: 100)\n");
  printf("      --low-memory            Use low memory mode\n");
  printf("      --exact                 Preserve exact pixels\n");
  printf("      --delta-palette         Use delta palette\n");
  printf("\n=== AVIF Options (--format=avif) ===\n");
  printf("  -q, --quality <float>       Set color quality (0-100, default: 50)\n");
  printf("      --alpha-q <int>         Alpha quality (0-100, default: 100)\n");
  printf("      --speed <int>           Encoder speed (0-10, default: 0; higher=faster)\n");
  printf("\n=== PNGX Options (--format=pngx) ===\n");
  printf("  -m, --method <int>                       Optimization level (0-6, default: 6)\n");
  printf("      --strip-safe                         Strip safe-to-remove chunks (default: on)\n");
  printf("      --no-strip-safe                      Keep all chunks\n");
  printf("      --optimize-alpha                     Optimize alpha channel (default: on)\n");
  printf("      --no-optimize-alpha                  Don't optimize alpha channel\n");
  printf("      --lossy                              Enable lossy palette quantization (default: on)\n");
  printf("      --type <value>                       Reduction type: palette256 (default), limitedrgba16bit/limited, reducedrgba32/reduced\n");
  printf("      --max-colors <int>                   Max palette colors (2-256, default: 256)\n");
  printf("      --reduced-colors <int>               Reduced RGBA32 colors (-1 auto, %d-%d)\n", COLOPRESSO_PNGX_REDUCED_COLORS_MIN,
      COLOPRESSO_PNGX_REDUCED_COLORS_MAX);
  printf("      --reduce-bits-rgb <int>              Reduced RGBA32 RGB bits (%d-%d, default: %d)\n", COLOPRESSO_PNGX_REDUCED_BITS_MIN,
      COLOPRESSO_PNGX_REDUCED_BITS_MAX, COLOPRESSO_PNGX_DEFAULT_REDUCED_BITS_RGB);
  printf("      --reduce-alpha <int>                 Reduced RGBA32 alpha bits (%d-%d, default: %d)\n", COLOPRESSO_PNGX_REDUCED_BITS_MIN,
      COLOPRESSO_PNGX_REDUCED_BITS_MAX, COLOPRESSO_PNGX_DEFAULT_REDUCED_ALPHA_BITS);
  printf("      --quality <min-max>                  Quality range (e.g. 80-95, default: 80-95)\n");
  printf("      --speed <int>                        Quantization speed (1-10, default: 1)\n");
  printf("      --dither <float>                     Dither level (0.0-1.0 or -1 for Limited auto, default: auto)\n");
  printf("      --smooth-cutoff <float>              Palette smoothing importance cutoff (-1 or 0.0-1.0, default: 0.6)\n");
  printf("      --gradient-profile                   Enable palette256 gradient-profile auto tuning (default: on)\n");
  printf("      --no-gradient-profile                Disable palette256 gradient-profile auto tuning\n");
  printf("      --gradient-dither-floor <float>      Override gradient-profile dither floor (-1 or 0.0-1.0, default: %.2f)\n", (double)PNGX_PALETTE256_GRADIENT_PROFILE_DITHER_FLOOR);
  printf("      --gradient-opaque-threshold <float>  Override gradient opaque ratio threshold (-1 or 0.0-1.0, default: %.2f)\n", (double)PNGX_PALETTE256_GRADIENT_PROFILE_OPAQUE_RATIO_THRESHOLD);
  printf("      --gradient-mean-max <float>          Override gradient mean max (-1 or 0.0-1.0, default: %.2f)\n", (double)PNGX_PALETTE256_GRADIENT_PROFILE_GRADIENT_MEAN_MAX);
  printf("      --gradient-sat-mean-max <float>      Override saturation mean max (-1 or 0.0-1.0, default: %.2f)\n", (double)PNGX_PALETTE256_GRADIENT_PROFILE_SATURATION_MEAN_MAX);
  printf("      --tune-opaque-threshold <float>      Override tune opaque ratio threshold (-1 or 0.0-1.0, default: %.2f)\n", (double)PNGX_PALETTE256_TUNE_OPAQUE_RATIO_THRESHOLD);
  printf("      --tune-gradient-mean-max <float>     Override tune gradient mean max (-1 or 0.0-1.0, default: %.2f)\n", (double)PNGX_PALETTE256_TUNE_GRADIENT_MEAN_MAX);
  printf("      --tune-sat-mean-max <float>          Override tune saturation mean max (-1 or 0.0-1.0, default: %.2f)\n", (double)PNGX_PALETTE256_TUNE_SATURATION_MEAN_MAX);
  printf("      --tune-speed-max <int>               Override tune speed max (-1 or 1-10, default: %d)\n", (int)PNGX_PALETTE256_TUNE_SPEED_MAX);
  printf("      --tune-quality-min-floor <int>       Override tune quality min floor (-1 or 0-100, default: %d)\n", (int)PNGX_PALETTE256_TUNE_QUALITY_MIN_FLOOR);
  printf("      --tune-quality-max-target <int>      Override tune quality max target (-1 or 0-100, default: %d)\n", (int)PNGX_PALETTE256_TUNE_QUALITY_MAX_TARGET);
  printf("      --alpha-bleed                        Enable palette256 alpha bleed (default: on)\n");
  printf("      --no-alpha-bleed                     Disable palette256 alpha bleed\n");
  printf("      --alpha-bleed-max-distance <int>     Bleed propagation distance (0-65535, default: 64)\n");
  printf("      --alpha-bleed-opaque-threshold <int> Opaque seed alpha threshold (0-255, default: 248)\n");
  printf("      --alpha-bleed-soft-limit <int>       Apply bleed when alpha <= soft limit (0-255, default: 160)\n");
  printf("      --protect-color <list>               Protect colors from quantization\n");
  printf("                                             Format: RRGGBB or RRGGBBAA (hex), comma-separated\n");
  printf("                                             Example: --protect-color=FF0000,00FF00,0000FFFF\n");
}

static inline void format_buildtime(uint32_t buildtime, char *buf, size_t buf_size) {
  uint32_t year, month, day, hour, minute;
  int32_t utc_hour, jst_hour, jst_day, jst_month, jst_year;
  int32_t days_in_month;

  if (buildtime == 0) {
    snprintf(buf, buf_size, "unknown");
    return;
  }

  year = (buildtime >> 20) & 0xfff;
  month = (buildtime >> 16) & 0xf;
  day = (buildtime >> 11) & 0x1f;
  hour = (buildtime >> 6) & 0x1f;
  minute = buildtime & 0x3f;

  /* Convert UTC to JST (UTC+9) */
  utc_hour = (int32_t)hour;
  jst_hour = utc_hour + 9;
  jst_day = (int32_t)day;
  jst_month = (int32_t)month;
  jst_year = (int32_t)year;

  if (jst_hour >= 24) {
    jst_hour -= 24;
    jst_day++;

    switch (jst_month) {
    case 1: case 3: case 5: case 7: case 8: case 10: case 12:
      days_in_month = 31;
      break;
    case 4: case 6: case 9: case 11:
      days_in_month = 30;
      break;
    case 2:
      days_in_month = ((jst_year % 4 == 0 && jst_year % 100 != 0) || jst_year % 400 == 0) ? 29 : 28;
      break;
    default:
      days_in_month = 31;
      break;
    }

    if (jst_day > days_in_month) {
      jst_day = 1;
      jst_month++;
      if (jst_month > 12) {
        jst_month = 1;
        jst_year++;
      }
    }
  }

  snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d JST", jst_year, jst_month, jst_day, jst_hour, (int)minute);
}

static inline void print_version(void) {
  char version_buf[32], buildtime_buf[32];
  const char *compiler_version, *rust_version;
  uint32_t buildtime;

  format_version(cpres_get_version(), version_buf, sizeof(version_buf));
  printf("colopresso CLI v%s\n", version_buf);
  printf("PNG to WebP/AVIF converter and PNG optimizer\n");
  printf("\nLibrary versions:\n");

  format_webp_version(cpres_get_libwebp_version(), version_buf, sizeof(version_buf));
  printf("  libwebp:         v%s\n", version_buf);

  format_libavif_version(cpres_get_libavif_version(), version_buf, sizeof(version_buf));
  printf("  libavif:         v%s\n", version_buf);

  format_libpng_version(cpres_get_libpng_version(), version_buf, sizeof(version_buf));
  printf("  libpng:          v%s\n", version_buf);

  format_libpng_version(cpres_get_pngx_oxipng_version(), version_buf, sizeof(version_buf));
  printf("  oxipng:          v%s\n", version_buf);

  format_libpng_version(cpres_get_pngx_libimagequant_version(), version_buf, sizeof(version_buf));
  printf("  libimagequant:   v%s\n", version_buf);

  printf("\nBuild information:\n");

  compiler_version = cpres_get_compiler_version_string();
  printf("  C / C++:         %s\n", compiler_version ? compiler_version : "unknown");

  rust_version = cpres_get_rust_version_string();
  printf("  Rust:            %s\n", rust_version ? rust_version : "unknown");

  buildtime = cpres_get_buildtime();
  format_buildtime(buildtime, buildtime_buf, sizeof(buildtime_buf));
  printf("  Build time:      %s\n", buildtime_buf);
}

static inline void init_cli_context(cli_context_t *ctx) {
  uint32_t cpu_count;

  memset(ctx, 0, sizeof(*ctx));
  cpres_config_init_defaults(&ctx->config);

  ctx->format = FORMAT_UNKNOWN;
  cpu_count = colopresso_get_cpu_count();
  if (cpu_count == 0) {
    cpu_count = 1;
  }
  ctx->config.webp_thread_level = (int)cpu_count;
  ctx->config.avif_threads = (int)cpu_count;
  ctx->config.pngx_threads = (int)cpu_count;

  ctx->config.webp_method = 6;
  ctx->config.webp_pass = 10;
  ctx->config.webp_preprocessing = 2;

  ctx->config.avif_speed = 0;

  ctx->config.pngx_level = 6;
  ctx->config.pngx_lossy_speed = 1;
}

static inline void free_cli_context(cli_context_t *ctx) {
  if (!ctx) {
    return;
  }

  free(ctx->output_file);
  ctx->output_file = NULL;

  if (ctx->protected_colors) {
    free(ctx->protected_colors);
    ctx->protected_colors = NULL;
  }

  ctx->config.pngx_protected_colors = NULL;
  ctx->config.pngx_protected_colors_count = 0;
}

static inline bool parse_arguments(int argc, char *argv[], cli_context_t *ctx, int *exit_code) {
  const char *input_file, *output_base, *output_extension;
  output_format_t format = FORMAT_UNKNOWN, inferred_format;
  int32_t quality_min = 0, quality_max = 0;
  bool format_specified = false, verbose = false, append_extension, quality_scalar_set = false, quality_range_set = false, pngx_type_specified = false, dither_specified = false;
  char *output_file;
  int opt, option_index = 0;
  long parsed_long = 0;
  float quality_scalar_value = 0.0f;
  double parsed_double = 0.0;  

  if (argc < 2) {
    print_usage(argv[0]);
    *exit_code = 1;
    return false;
  }

  optind = 1;
  while ((opt = getopt_long(argc, argv, "q:lm:s:p:t:vhV", kLongOptions, &option_index)) != -1) {
    switch (opt) {
    case 0: {
      const char *name = kLongOptions[option_index].name;

      if (strcmp(name, "format") == 0) {
        format_specified = true;
        format = parse_format(optarg);
        if (format == FORMAT_UNKNOWN) {
          fprintf(stderr, "Error: Unknown format '%s'. Use webp, avif, or pngx.\n", optarg);
          *exit_code = 1;
          return false;
        }
      } else if (strcmp(name, "type") == 0) {
        pngx_type_specified = true;
        if (!parse_pngx_type_option(optarg, &ctx->config.pngx_lossy_type)) {
          fprintf(stderr, "Error: Unknown PNGX type '%s'. Use palette256, limitedrgba16bit/limited or reducedrgba32/reduced.\n", optarg);
          *exit_code = 1;
          return false;
        }
      } else {
        if (!handle_long_option(name, optarg, &ctx->config, &ctx->protected_colors, &ctx->protected_colors_count, &dither_specified)) {
          *exit_code = 1;
          return false;
        }
      }
      break;
    }
    case 'q':
      if (strchr(optarg, '-') != NULL) {
        if (!parse_quality_range(optarg, &quality_min, &quality_max)) {
          fprintf(stderr, "Error: Invalid quality format (expect min-max, e.g., 70-95)\n");
          *exit_code = 1;
          return false;
        }
        quality_range_set = true;
        quality_scalar_set = false;
      } else {
        if (!parse_double_range(optarg, 0.0, 100.0, &parsed_double)) {
          fprintf(stderr, "Error: Invalid quality (must be 0-100)\n");
          *exit_code = 1;
          return false;
        }
        quality_scalar_set = true;
        quality_range_set = false;
        quality_scalar_value = (float)parsed_double;
      }
      break;
    case 'l':
      ctx->config.webp_lossless = true;
      ctx->config.avif_lossless = true;
      ctx->config.pngx_lossy_enable = false;
      break;
    case 'm':
      if (!parse_long_range(optarg, 0, 6, &parsed_long)) {
        fprintf(stderr, "Error: Invalid method (must be 0-6)\n");
        *exit_code = 1;
        return false;
      }
      ctx->config.webp_method = (int)parsed_long;
      ctx->config.pngx_level = (int)parsed_long;
      break;
    case 's':
      if (!parse_long_value(optarg, &parsed_long) || parsed_long < 0 || parsed_long > INT_MAX) {
        fprintf(stderr, "Error: Invalid target size\n");
        *exit_code = 1;
        return false;
      }
      ctx->config.webp_target_size = (int)parsed_long;
      break;
    case 'p':
      if (!parse_double_value(optarg, &parsed_double) || parsed_double < 0.0) {
        fprintf(stderr, "Error: Invalid target PSNR\n");
        *exit_code = 1;
        return false;
      }
      ctx->config.webp_target_psnr = (float)parsed_double;
      break;
    case 't':
      if (!parse_long_value(optarg, &parsed_long) || parsed_long < 0 || parsed_long > INT_MAX) {
        fprintf(stderr, "Error: Invalid thread count\n");
        *exit_code = 1;
        return false;
      }
      ctx->config.webp_thread_level = (int)parsed_long;
      ctx->config.avif_threads = (int)parsed_long;
      ctx->config.pngx_threads = (int)parsed_long;
      break;
    case 'v':
      verbose = true;
      break;
    case 'h':
      print_usage(argv[0]);
      *exit_code = 0;
      return false;
    case 'V':
      print_version();
      *exit_code = 0;
      return false;
    default:
      print_usage(argv[0]);
      *exit_code = 1;
      return false;
    }
  }

  if (optind + 2 > argc) {
    fprintf(stderr, "Error: Missing input or output file\n");
    print_usage(argv[0]);
    *exit_code = 1;
    return false;
  }

  input_file = argv[optind];
  output_base = argv[optind + 1];

  output_extension = colopresso_extract_extension(output_base);
  inferred_format = infer_format_from_extension(output_base);

  if (!format_specified && format == FORMAT_UNKNOWN && inferred_format != FORMAT_UNKNOWN) {
    format = inferred_format;
  }

  if (format == FORMAT_UNKNOWN) {
    fprintf(stderr, "Error: Output format not specified and could not infer from output extension\n");
    print_usage(argv[0]);
    *exit_code = 1;
    return false;
  }

  if (pngx_type_specified && format != FORMAT_PNGX) {
    fprintf(stderr, "Error: --type option is only valid when --format=pngx\n");
    *exit_code = 1;
    return false;
  }

  if (quality_range_set) {
    if (format != FORMAT_PNGX) {
      fprintf(stderr, "Error: Quality ranges (min-max) are only supported for PNGX outputs\n");
      *exit_code = 1;
      return false;
    }
    ctx->config.pngx_lossy_quality_min = (int)quality_min;
    ctx->config.pngx_lossy_quality_max = (int)quality_max;
  } else if (quality_scalar_set) {
    if (format == FORMAT_WEBP) {
      ctx->config.webp_quality = quality_scalar_value;
    }
    if (format == FORMAT_AVIF) {
      ctx->config.avif_quality = quality_scalar_value;
    }
    if (format == FORMAT_PNGX) {
      ctx->config.pngx_lossy_quality_min = (int)quality_scalar_value;
      ctx->config.pngx_lossy_quality_max = (int)quality_scalar_value;
    }
  }

  if (format == FORMAT_PNGX) {
    bool limited_mode_selected = (ctx->config.pngx_lossy_type == CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444);
    if (limited_mode_selected && !dither_specified) {
      ctx->config.pngx_lossy_dither_level = -1.0f;
    }
  }

  if (format_specified && inferred_format != FORMAT_UNKNOWN && inferred_format != format) {
    fprintf(stderr,
            "Warning: Output file extension '%s' does not match --format=%s; encoding as %s\n",
            output_extension ? output_extension : "", get_format_name(format), get_format_name(format));
  }

  append_extension = should_append_extension(output_base, format, format_specified);
  if (append_extension) {
    output_file = build_output_path(output_base, format);
  } else {
    output_file = strdup(output_base);
  }

  if (!output_file) {
    fprintf(stderr, "Error: Failed to allocate memory for output path\n");
    *exit_code = 1;
    return false;
  }

  ctx->input_file = input_file;
  ctx->output_file = output_file;
  ctx->format = format;
  ctx->verbose = verbose;

  return true;
}

static inline int run_conversion(cli_context_t *ctx) {
  int64_t input_size, output_size, ref_input_size;
  int32_t size_check;
  uint8_t *png_data = NULL, *encoded_data = NULL;
  size_t encoded_size = 0, png_size = 0;
  void (*encoded_deallocator)(uint8_t *) = NULL;
  int ret = 1;
  bool force_rgba_output = false;
  cpres_error_t result = CPRES_ERROR_INVALID_FORMAT, read_error = CPRES_OK;

  input_size = get_file_size(ctx->input_file);
  force_rgba_output = (ctx->format == FORMAT_PNGX && ctx->config.pngx_lossy_enable &&
                       (ctx->config.pngx_lossy_type == CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444 ||
                        ctx->config.pngx_lossy_type == CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32));

  if (ctx->verbose) {
    print_verbose_summary(&ctx->config, ctx->format, ctx->input_file, ctx->output_file, input_size,
                          ctx->protected_colors, ctx->protected_colors_count);
  }

  if (ctx->format == FORMAT_PNGX && ctx->protected_colors_count > 0) {
    ctx->config.pngx_protected_colors = ctx->protected_colors;
    ctx->config.pngx_protected_colors_count = (int)ctx->protected_colors_count;
  }

  read_error = cpres_read_file_to_memory(ctx->input_file, &png_data, &png_size);
  if (read_error != CPRES_OK) {
    fprintf(stderr, "Error: Failed to read input file '%s': %s\n", ctx->input_file, cpres_error_string(read_error));
    goto bailout;
  }

  switch (ctx->format) {
  case FORMAT_WEBP:
    encoded_deallocator = cpres_free;
    result = cpres_encode_webp_memory(png_data, png_size, &encoded_data, &encoded_size, &ctx->config);
    break;
  case FORMAT_AVIF:
    encoded_deallocator = cpres_free;
    result = cpres_encode_avif_memory(png_data, png_size, &encoded_data, &encoded_size, &ctx->config);
    break;
  case FORMAT_PNGX:
    encoded_deallocator = cpres_free;
    result = cpres_encode_pngx_memory(png_data, png_size, &encoded_data, &encoded_size, &ctx->config);
    break;
  default:
    result = CPRES_ERROR_INVALID_FORMAT;
    break;
  }

  free(png_data);
  png_data = NULL;

  if (result == CPRES_OK) {
    if (!encoded_data || encoded_size == 0) {
      fprintf(stderr, "Error: Encoding produced no output data\n");
      goto bailout;
    }

    if (!write_file_from_memory(ctx->output_file, encoded_data, encoded_size)) {
      fprintf(stderr, "Error: Failed to write output file '%s'\n", ctx->output_file);
      goto bailout;
    }

    if (encoded_deallocator) {
      encoded_deallocator(encoded_data);
    }
    encoded_data = NULL;

    ret = 0;
    output_size = get_file_size(ctx->output_file);
    if (force_rgba_output) {
      if (input_size >= 0 && output_size >= 0 && output_size > input_size) {
        print_output_larger_warning(get_format_name(ctx->format), input_size, (size_t)output_size);
      }
      size_check = 0;
    } else {
      size_check = handle_size_increase_error(input_size, output_size);
    }

    if (size_check != 0) {
      ret = size_check;
    } else if (ctx->verbose) {
      print_conversion_success(input_size, output_size);
    }

    goto bailout;
  }

  if (result == CPRES_ERROR_OUTPUT_NOT_SMALLER) {
    ref_input_size = input_size >= 0 ? input_size : (int64_t)png_size;
    print_output_larger_warning(get_format_name(ctx->format), ref_input_size, encoded_size);
    ret = 2;

    goto bailout;
  }

  fprintf(stderr, "Error: %s\n", cpres_error_string(result));

bailout:
  if (encoded_deallocator && encoded_data) {
    encoded_deallocator(encoded_data);
  }

  if (png_data) {
    free(png_data);
  }

  return ret;
}

int main(int argc, char *argv[]) {
  cli_context_t ctx;
  int exit_code = 0;

  init_cli_context(&ctx);

  if (!parse_arguments(argc, argv, &ctx, &exit_code)) {
    free_cli_context(&ctx);
    return exit_code;
  }

  exit_code = run_conversion(&ctx);
  free_cli_context(&ctx);

  return exit_code;
}
