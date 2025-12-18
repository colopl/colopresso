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
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <colopresso.h>
#include <colopresso/portable.h>

#ifndef COLOPRESSO_TEST_ASSETS_DIR
#define COLOPRESSO_TEST_ASSETS_DIR "./assets"
#endif

#define NUM_ITERATIONS 5
#define NUM_WARMUP_RUNS 2

#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"
#define COLOR_GREEN "\033[32m"
#define COLOR_BLUE "\033[34m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"
#define COLOR_MAGENTA "\033[35m"

typedef struct {
  const char *name;
  double time_mean_ms;
  double time_stddev_ms;
  double time_min_ms;
  double time_max_ms;
  size_t output_size;
  double compression_ratio;
  bool success;
  cpres_error_t error;
} benchmark_result_t;

static inline bool parse_thread_count_value(const char *value, int *out_threads) {
  char *endptr = NULL;
  long parsed;

  if (!value || !out_threads) {
    return false;
  }

  errno = 0;
  parsed = strtol(value, &endptr, 10);
  if (errno != 0 || endptr == value || *endptr != '\0') {
    return false;
  }

  if (parsed < 0 || parsed > INT_MAX) {
    return false;
  }

  *out_threads = (int)parsed;
  return true;
}

static inline void apply_thread_count(cpres_config_t *config, int thread_count) {
  if (!config) {
    return;
  }

  config->webp_thread_level = (thread_count > 1) ? 1 : 0;
  config->avif_threads = thread_count;
  config->pngx_threads = thread_count;
}

static void print_usage(const char *program_name) { printf("Usage: %s [--threads N|-t N] [input.png]\n", program_name ? program_name : "benchmark"); }

static double get_time_us(void) {
  struct timeval tv;

  gettimeofday(&tv, NULL);

  return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
}

static inline double calculate_stddev(const double *values, uint8_t count, double mean) {
  uint8_t i;
  double sum_sq_diff = 0.0, diff;

  for (i = 0; i < count; i++) {
    diff = values[i] - mean;
    sum_sq_diff += diff * diff;
  }

  return sqrt(sum_sq_diff / count);
}

static inline void print_separator(void) { printf("─────────────────────────────────────────────────────────────────────────────────────\n"); }

static inline void print_result(const benchmark_result_t *result, size_t input_size) {
  if (!result->success) {
    printf("  %-30s " COLOR_YELLOW "[FAILED: %s]" COLOR_RESET "\n", result->name, cpres_error_string(result->error));
    return;
  }

  printf("  %-30s " COLOR_GREEN "✓" COLOR_RESET " ", result->name);
  printf(COLOR_CYAN "%.2f" COLOR_RESET " ms (±%.2f) ", result->time_mean_ms, result->time_stddev_ms);
  printf("[%.2f-%.2f] ", result->time_min_ms, result->time_max_ms);
  printf(COLOR_MAGENTA "%zu" COLOR_RESET " bytes ", result->output_size);
  printf(COLOR_BLUE "%.1f%%" COLOR_RESET " compression\n", result->compression_ratio * 100.0);
}

static benchmark_result_t benchmark_webp(const uint8_t *png_data, size_t png_size, const char *name, const cpres_config_t *config) {
  double times[NUM_ITERATIONS], start, end, sum;
  uint8_t *output_data = NULL, *tmp, i;
  size_t output_size = 0, tmp_size;
  cpres_error_t err;
  benchmark_result_t result = {0};

  result.name = name;
  result.success = false;

  for (i = 0; i < NUM_WARMUP_RUNS; i++) {
    tmp = NULL;
    tmp_size = 0;
    cpres_encode_webp_memory(png_data, png_size, &tmp, &tmp_size, config);
    cpres_free(tmp);
  }

  for (i = 0; i < NUM_ITERATIONS; i++) {
    if (output_data) {
      cpres_free(output_data);
      output_data = NULL;
    }

    start = get_time_us();
    err = cpres_encode_webp_memory(png_data, png_size, &output_data, &output_size, config);
    end = get_time_us();

    if (err != CPRES_OK) {
      result.error = err;

      if (output_data) {
        cpres_free(output_data);
      }

      return result;
    }

    times[i] = (end - start) / 1000.0;
  }

  result.time_min_ms = times[0];
  result.time_max_ms = times[0];
  sum = 0.0;

  for (i = 0; i < NUM_ITERATIONS; i++) {
    sum += times[i];
    if (times[i] < result.time_min_ms) {
      result.time_min_ms = times[i];
    }
    if (times[i] > result.time_max_ms) {
      result.time_max_ms = times[i];
    }
  }

  result.time_mean_ms = sum / NUM_ITERATIONS;
  result.time_stddev_ms = calculate_stddev(times, NUM_ITERATIONS, result.time_mean_ms);
  result.output_size = output_size;
  result.compression_ratio = (double)output_size / (double)png_size;
  result.success = true;

  cpres_free(output_data);

  return result;
}

static benchmark_result_t benchmark_avif(const uint8_t *png_data, size_t png_size, const char *name, const cpres_config_t *config) {
  double times[NUM_ITERATIONS], start, end, sum;
  uint8_t *output_data = NULL, *tmp, i;
  size_t output_size = 0, tmp_size;
  cpres_error_t err;
  benchmark_result_t result = {0};

  result.name = name;
  result.success = false;

  for (i = 0; i < NUM_WARMUP_RUNS; i++) {
    tmp = NULL;
    tmp_size = 0;
    cpres_encode_avif_memory(png_data, png_size, &tmp, &tmp_size, config);
    cpres_free(tmp);
  }

  for (i = 0; i < NUM_ITERATIONS; i++) {
    if (output_data) {
      cpres_free(output_data);
      output_data = NULL;
    }

    start = get_time_us();
    err = cpres_encode_avif_memory(png_data, png_size, &output_data, &output_size, config);
    end = get_time_us();

    if (err != CPRES_OK) {
      result.error = err;
      if (output_data)
        cpres_free(output_data);
      return result;
    }

    times[i] = (end - start) / 1000.0;
  }

  result.time_min_ms = times[0];
  result.time_max_ms = times[0];
  sum = 0.0;

  for (int i = 0; i < NUM_ITERATIONS; i++) {
    sum += times[i];
    if (times[i] < result.time_min_ms)
      result.time_min_ms = times[i];
    if (times[i] > result.time_max_ms)
      result.time_max_ms = times[i];
  }

  result.time_mean_ms = sum / NUM_ITERATIONS;
  result.time_stddev_ms = calculate_stddev(times, NUM_ITERATIONS, result.time_mean_ms);
  result.output_size = output_size;
  result.compression_ratio = (double)output_size / (double)png_size;
  result.success = true;

  cpres_free(output_data);

  return result;
}

static benchmark_result_t benchmark_pngx(const uint8_t *png_data, size_t png_size, const char *name, const cpres_config_t *config) {
  double times[NUM_ITERATIONS], start, end, sum = 0.0f;
  uint8_t *output_data = NULL, *tmp = NULL, i;
  size_t output_size = 0, tmp_size = 0;
  cpres_error_t err;
  benchmark_result_t result = {0};

  result.name = name;
  result.success = false;

  for (i = 0; i < NUM_WARMUP_RUNS; i++) {
    cpres_encode_pngx_memory(png_data, png_size, &tmp, &tmp_size, config);
    cpres_free(tmp);
  }

  for (i = 0; i < NUM_ITERATIONS; i++) {
    if (output_data) {
      cpres_free(output_data);
      output_data = NULL;
    }

    start = get_time_us();
    err = cpres_encode_pngx_memory(png_data, png_size, &output_data, &output_size, config);
    end = get_time_us();

    if (err != CPRES_OK) {
      result.error = err;
      if (output_data)
        cpres_free(output_data);
      return result;
    }

    times[i] = (end - start) / 1000.0;
  }

  result.time_min_ms = times[0];
  result.time_max_ms = times[0];

  for (i = 0; i < NUM_ITERATIONS; i++) {
    sum += times[i];

    if (times[i] < result.time_min_ms) {
      result.time_min_ms = times[i];
    }
    if (times[i] > result.time_max_ms) {
      result.time_max_ms = times[i];
    }
  }

  result.time_mean_ms = sum / NUM_ITERATIONS;
  result.time_stddev_ms = calculate_stddev(times, NUM_ITERATIONS, result.time_mean_ms);
  result.output_size = output_size;
  result.compression_ratio = (double)output_size / (double)png_size;
  result.success = true;

  cpres_free(output_data);

  return result;
}

static void run_webp_benchmarks(const uint8_t *png_data, size_t png_size, int thread_count) {
  const float qualities[] = {50.0f, 75.0f, 90.0f};
  const char *quality_names[] = {"WebP Quality 50 (Method 6)", "WebP Quality 75 (Method 6)", "WebP Quality 90 (Method 6)"};
  uint8_t result_count = 0, i;
  cpres_config_t config;
  benchmark_result_t results[10];

  printf("\n" COLOR_BOLD COLOR_BLUE "━━━ WebP Encoding Benchmarks ━━━" COLOR_RESET "\n\n");

  for (i = 0; i < 3; i++) {
    cpres_config_init_defaults(&config);
    config.webp_quality = qualities[i];
    config.webp_method = 6;
    apply_thread_count(&config, thread_count);
    results[result_count++] = benchmark_webp(png_data, png_size, quality_names[i], &config);
  }

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.webp_lossless = true;
  results[result_count++] = benchmark_webp(png_data, png_size, "WebP Lossless", &config);

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.webp_quality = 80.0f;
  config.webp_method = 0;
  results[result_count++] = benchmark_webp(png_data, png_size, "WebP Q80 Method 0 (fastest)", &config);

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.webp_quality = 80.0f;
  config.webp_method = 4;
  results[result_count++] = benchmark_webp(png_data, png_size, "WebP Q80 Method 4 (default)", &config);

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.webp_quality = 80.0f;
  config.webp_method = 6;
  results[result_count++] = benchmark_webp(png_data, png_size, "WebP Q80 Method 6 (best)", &config);

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.webp_quality = 80.0f;
  config.webp_method = 6;
  config.webp_use_sharp_yuv = true;
  results[result_count++] = benchmark_webp(png_data, png_size, "WebP Q80 + Sharp YUV", &config);

  print_separator();

  for (i = 0; i < result_count; i++) {
    print_result(&results[i], png_size);
  }

  print_separator();
}

static void run_avif_benchmarks(const uint8_t *png_data, size_t png_size, int thread_count) {
  const float qualities[] = {40.0f, 50.0f, 60.0f, 80.0f};
  const char *quality_names[] = {"AVIF Quality 40 (Speed 6)", "AVIF Quality 50 (Speed 6)", "AVIF Quality 60 (Speed 6)", "AVIF Quality 80 (Speed 6)"};
  uint8_t result_count = 0, i;
  cpres_config_t config;
  benchmark_result_t results[10];

  printf("\n" COLOR_BOLD COLOR_MAGENTA "━━━ AVIF Encoding Benchmarks ━━━" COLOR_RESET "\n\n");

  for (i = 0; i < 4; i++) {
    cpres_config_init_defaults(&config);
    config.avif_quality = qualities[i];
    config.avif_speed = 6;
    apply_thread_count(&config, thread_count);
    results[result_count++] = benchmark_avif(png_data, png_size, quality_names[i], &config);
  }

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.avif_lossless = true;
  config.avif_speed = 6;
  results[result_count++] = benchmark_avif(png_data, png_size, "AVIF Lossless (Speed 6)", &config);

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.avif_quality = 50.0f;
  config.avif_speed = 8;
  results[result_count++] = benchmark_avif(png_data, png_size, "AVIF Q50 Speed 8 (faster)", &config);

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.avif_quality = 50.0f;
  config.avif_speed = 4;
  results[result_count++] = benchmark_avif(png_data, png_size, "AVIF Q50 Speed 4 (better)", &config);

  print_separator();

  for (i = 0; i < result_count; i++) {
    print_result(&results[i], png_size);
  }

  print_separator();
}

static void run_pngx_benchmarks(const uint8_t *png_data, size_t png_size, int thread_count) {
  const int levels[] = {1, 3, 5, 6};
  const char *level_names[] = {"PNGX Level 1 (fastest)", "PNGX Level 3 (balanced)", "PNGX Level 5 (default)", "PNGX Level 6 (maximum)"};
  uint8_t result_count = 0, i;
  benchmark_result_t results[10];
  cpres_config_t config;

  printf("\n" COLOR_BOLD COLOR_CYAN "━━━ PNGX Optimization Benchmarks ━━━" COLOR_RESET "\n\n");

  for (i = 0; i < 4; i++) {
    cpres_config_init_defaults(&config);
    config.pngx_level = levels[i];
    config.pngx_lossy_enable = false;
    apply_thread_count(&config, thread_count);
    results[result_count++] = benchmark_pngx(png_data, png_size, level_names[i], &config);
  }

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.pngx_level = 5;
  config.pngx_lossy_enable = true;
  config.pngx_lossy_max_colors = 256;
  config.pngx_lossy_quality_min = 80;
  config.pngx_lossy_quality_max = 95;
  results[result_count++] = benchmark_pngx(png_data, png_size, "PNGX Lossy (256 colors, Q80-95)", &config);

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.pngx_level = 5;
  config.pngx_lossy_enable = true;
  config.pngx_lossy_max_colors = 128;
  config.pngx_lossy_quality_min = 70;
  config.pngx_lossy_quality_max = 90;
  results[result_count++] = benchmark_pngx(png_data, png_size, "PNGX Lossy (128 colors, Q70-90)", &config);

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.pngx_level = 5;
  config.pngx_lossy_enable = true;
  config.pngx_lossy_max_colors = 64;
  config.pngx_lossy_quality_min = 60;
  config.pngx_lossy_quality_max = 80;
  results[result_count++] = benchmark_pngx(png_data, png_size, "PNGX Lossy (64 colors, Q60-80)", &config);

  cpres_config_init_defaults(&config);
  apply_thread_count(&config, thread_count);
  config.pngx_level = 5;
  config.pngx_lossy_enable = true;
  config.pngx_lossy_type = CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444;
  config.pngx_lossy_dither_level = 1.0f;
  results[result_count++] = benchmark_pngx(png_data, png_size, "PNGX Limited RGBA4444", &config);

  print_separator();

  for (i = 0; i < result_count; i++) {
    print_result(&results[i], png_size);
  }

  print_separator();
}

static void print_summary(size_t png_size, int thread_count) {
  printf("\n" COLOR_BOLD COLOR_GREEN "━━━ Summary ━━━" COLOR_RESET "\n\n");
  printf("  Input PNG size:     " COLOR_CYAN "%zu" COLOR_RESET " bytes\n", png_size);
  printf("  Iterations:         " COLOR_CYAN "%d" COLOR_RESET "\n", NUM_ITERATIONS);
  printf("  Warmup runs:        " COLOR_CYAN "%d" COLOR_RESET "\n", NUM_WARMUP_RUNS);
  printf("  Threads:            " COLOR_CYAN "%d" COLOR_RESET "\n", thread_count);
  printf("\n");
  printf("  Legend:\n");
  printf("    Time:        Mean execution time ± standard deviation [min-max]\n");
  printf("    Size:        Output file size in bytes\n");
  printf("    Compression: Percentage of original size (lower is better)\n");
  printf("\n");
}

int main(int argc, char *argv[]) {
  const char *input_file = NULL, *arg, *value;
  char default_path[512];
  uint32_t cpres_ver = cpres_get_version(), webp_ver = cpres_get_libwebp_version(), avif_ver = cpres_get_libavif_version(), oxipng_ver = cpres_get_pngx_oxipng_version(),
           imagequant_ver = cpres_get_pngx_libimagequant_version(), cpu_count;
  uint8_t *png_data = NULL;
  size_t png_size = 0;
  bool used_default_input = false, threads_specified = false;
  int thread_count = -1, i;
  cpres_error_t read_error = CPRES_OK;

  for (i = 1; i < argc; ++i) {
    arg = argv[i];
    if (strcmp(arg, "--threads") == 0 || strcmp(arg, "-t") == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "Error: --threads requires a value\n");
        print_usage(argv[0]);
        return 1;
      }
      value = argv[++i];
      if (!parse_thread_count_value(value, &thread_count)) {
        fprintf(stderr, "Error: Invalid thread count '%s'\n", value);
        print_usage(argv[0]);
        return 1;
      }
      threads_specified = true;
      continue;
    }
    if (strncmp(arg, "--threads=", 10) == 0) {
      value = arg + 10;
      if (!parse_thread_count_value(value, &thread_count)) {
        fprintf(stderr, "Error: Invalid thread count '%s'\n", value);
        print_usage(argv[0]);
        return 1;
      }
      threads_specified = true;
      continue;
    }
    if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    }
    if (arg[0] == '-') {
      fprintf(stderr, "Error: Unknown option '%s'\n", arg);
      print_usage(argv[0]);
      return 1;
    }
    if (!input_file) {
      input_file = arg;
    } else {
      fprintf(stderr, "Error: Multiple input files specified ('%s')\n", arg);
      print_usage(argv[0]);
      return 1;
    }
  }

  if (!threads_specified) {
    cpu_count = colopresso_get_cpu_count();
    if (cpu_count == 0) {
      cpu_count = 1;
    }
    thread_count = (int)cpu_count;
  }

  if (!input_file) {
    snprintf(default_path, sizeof(default_path), "%s/example.png", COLOPRESSO_TEST_ASSETS_DIR);
    input_file = default_path;
    used_default_input = true;
  }

  if (used_default_input) {
    printf(COLOR_YELLOW "No input file specified, using default: %s" COLOR_RESET "\n", input_file);
  }

  read_error = cpres_read_file_to_memory(input_file, &png_data, &png_size);
  if (read_error != CPRES_OK) {
    fprintf(stderr, COLOR_YELLOW "Error: Failed to load PNG file '%s': %s" COLOR_RESET "\n", input_file, cpres_error_string(read_error));
    return 1;
  }

  printf("\n");
  printf(COLOR_BOLD "═══════════════════════════════════════════════════════════════════════════════════\n");
  printf("  libcolopresso - Encoding Performance Benchmark\n");
  printf("═══════════════════════════════════════════════════════════════════════════════════\n" COLOR_RESET);

  printf("\n" COLOR_BOLD "Library Versions:" COLOR_RESET "\n");

  printf("  libcolopresso:  %u.%u.%u\n", cpres_ver / 1000000, (cpres_ver % 1000000) / 1000, cpres_ver % 1000);
  printf("  libwebp:        %u.%u.%u\n", (webp_ver >> 16) & 0xff, (webp_ver >> 8) & 0xff, webp_ver & 0xff);
  printf("  libavif:        %u.%u.%u\n", avif_ver / 1000000, (avif_ver % 1000000) / 10000, (avif_ver % 10000) / 100);
  printf("  oxipng:         %u.%u.%u\n", oxipng_ver / 10000, (oxipng_ver % 10000) / 100, oxipng_ver % 100);
  printf("  libimagequant:  %u.%u.%u\n", imagequant_ver / 10000, (imagequant_ver % 10000) / 100, imagequant_ver % 100);

  print_summary(png_size, thread_count);

  run_webp_benchmarks(png_data, png_size, thread_count);
  run_avif_benchmarks(png_data, png_size, thread_count);
  run_pngx_benchmarks(png_data, png_size, thread_count);

  printf("\n" COLOR_BOLD COLOR_GREEN "Benchmark completed successfully!" COLOR_RESET "\n\n");

  free(png_data);

  return 0;
}
