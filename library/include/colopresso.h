/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2026 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

#ifndef COLOPRESSO_H
#define COLOPRESSO_H

#include <colopresso_config.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define COLOPRESSO_VERSION /* COLOPL_VERSION_START */ 123456789          /* COLOPL_VERSION_END */
#define COLOPRESSO_PNG_MAX_MEMORY_INPUT_SIZE ((size_t)512 * 1024 * 1024) /* 512 MB */
#define COLOPRESSO_WEBP_DEFAULT_QUALITY 80.0f
#define COLOPRESSO_WEBP_DEFAULT_LOSSLESS false
#define COLOPRESSO_WEBP_DEFAULT_METHOD 6
#define COLOPRESSO_WEBP_DEFAULT_TARGET_SIZE 0
#define COLOPRESSO_WEBP_DEFAULT_TARGET_PSNR 0.0f
#define COLOPRESSO_WEBP_DEFAULT_SEGMENTS 4
#define COLOPRESSO_WEBP_DEFAULT_SNS_STRENGTH 50
#define COLOPRESSO_WEBP_DEFAULT_FILTER_STRENGTH 60
#define COLOPRESSO_WEBP_DEFAULT_FILTER_SHARPNESS 0
#define COLOPRESSO_WEBP_DEFAULT_FILTER_TYPE 1
#define COLOPRESSO_WEBP_DEFAULT_AUTOFILTER true
#define COLOPRESSO_WEBP_DEFAULT_ALPHA_COMPRESSION true
#define COLOPRESSO_WEBP_DEFAULT_ALPHA_FILTERING 1
#define COLOPRESSO_WEBP_DEFAULT_ALPHA_QUALITY 100
#define COLOPRESSO_WEBP_DEFAULT_PASS 1
#define COLOPRESSO_WEBP_DEFAULT_PREPROCESSING 0
#define COLOPRESSO_WEBP_DEFAULT_PARTITIONS 0
#define COLOPRESSO_WEBP_DEFAULT_PARTITION_LIMIT 0
#define COLOPRESSO_WEBP_DEFAULT_EMULATE_JPEG_SIZE false
#define COLOPRESSO_WEBP_DEFAULT_THREAD_LEVEL 0
#define COLOPRESSO_WEBP_DEFAULT_LOW_MEMORY false
#define COLOPRESSO_WEBP_DEFAULT_NEAR_LOSSLESS 100
#define COLOPRESSO_WEBP_DEFAULT_EXACT false
#define COLOPRESSO_WEBP_DEFAULT_USE_DELTA_PALETTE false
#define COLOPRESSO_WEBP_DEFAULT_USE_SHARP_YUV false

#define COLOPRESSO_AVIF_DEFAULT_QUALITY 50.0f
#define COLOPRESSO_AVIF_DEFAULT_ALPHA_QUALITY 100
#define COLOPRESSO_AVIF_DEFAULT_LOSSLESS false
#define COLOPRESSO_AVIF_DEFAULT_SPEED 6
#define COLOPRESSO_AVIF_DEFAULT_THREADS 1

#define COLOPRESSO_PNGX_DEFAULT_LEVEL 5
#define COLOPRESSO_PNGX_DEFAULT_STRIP_SAFE true
#define COLOPRESSO_PNGX_DEFAULT_OPTIMIZE_ALPHA true
#define COLOPRESSO_PNGX_DEFAULT_LOSSY_ENABLE true
#define COLOPRESSO_PNGX_DEFAULT_LOSSY_MAX_COLORS 256
#define COLOPRESSO_PNGX_DEFAULT_LOSSY_QUALITY_MIN 80
#define COLOPRESSO_PNGX_DEFAULT_LOSSY_QUALITY_MAX 95
#define COLOPRESSO_PNGX_DEFAULT_LOSSY_SPEED 3
#define COLOPRESSO_PNGX_DEFAULT_LOSSY_DITHER_LEVEL 0.6f
#define COLOPRESSO_PNGX_DEFAULT_SALIENCY_MAP_ENABLE true
#define COLOPRESSO_PNGX_DEFAULT_CHROMA_ANCHOR_ENABLE true
#define COLOPRESSO_PNGX_DEFAULT_ADAPTIVE_DITHER_ENABLE true
#define COLOPRESSO_PNGX_DEFAULT_GRADIENT_BOOST_ENABLE true
#define COLOPRESSO_PNGX_DEFAULT_CHROMA_WEIGHT_ENABLE true
#define COLOPRESSO_PNGX_DEFAULT_POSTPROCESS_SMOOTH_ENABLE true
#define COLOPRESSO_PNGX_DEFAULT_POSTPROCESS_SMOOTH_IMPORTANCE_CUTOFF 0.6f
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_GRADIENT_PROFILE_ENABLE true
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_GRADIENT_DITHER_FLOOR 0.78f
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_ENABLE true
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_MAX_DISTANCE 64
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_OPAQUE_THRESHOLD 248
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_ALPHA_BLEED_SOFT_LIMIT 160
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_PROFILE_OPAQUE_RATIO_THRESHOLD 0.90f
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_PROFILE_GRADIENT_MEAN_MAX 0.16f
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_PROFILE_SATURATION_MEAN_MAX 0.42f
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_OPAQUE_RATIO_THRESHOLD 0.90f
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_GRADIENT_MEAN_MAX 0.14f
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_SATURATION_MEAN_MAX 0.35f
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_SPEED_MAX 1
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_QUALITY_MIN_FLOOR 90
#define COLOPRESSO_PNGX_DEFAULT_PALETTE256_TUNE_QUALITY_MAX_TARGET 100
#define COLOPRESSO_PNGX_DEFAULT_THREADS 1
#define COLOPRESSO_PNGX_LOSSY_TYPE_PALETTE256 0
#define COLOPRESSO_PNGX_LOSSY_TYPE_LIMITED_RGBA4444 1
#define COLOPRESSO_PNGX_LOSSY_TYPE_REDUCED_RGBA32 2
#define COLOPRESSO_PNGX_DEFAULT_LOSSY_TYPE COLOPRESSO_PNGX_LOSSY_TYPE_PALETTE256
#define COLOPRESSO_PNGX_DEFAULT_REDUCED_COLORS (-1)
#define COLOPRESSO_PNGX_REDUCED_COLORS_MIN 2
#define COLOPRESSO_PNGX_REDUCED_COLORS_MAX 32768
#define COLOPRESSO_PNGX_REDUCED_BITS_MIN 1
#define COLOPRESSO_PNGX_REDUCED_BITS_MAX 8
#define COLOPRESSO_PNGX_DEFAULT_REDUCED_BITS_RGB 4
#define COLOPRESSO_PNGX_DEFAULT_REDUCED_ALPHA_BITS 4

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} cpres_rgba_color_t;

typedef enum {
  CPRES_PNGX_LOSSY_TYPE_PALETTE256 = COLOPRESSO_PNGX_LOSSY_TYPE_PALETTE256,
  CPRES_PNGX_LOSSY_TYPE_LIMITED_RGBA4444 = COLOPRESSO_PNGX_LOSSY_TYPE_LIMITED_RGBA4444,
  CPRES_PNGX_LOSSY_TYPE_REDUCED_RGBA32 = COLOPRESSO_PNGX_LOSSY_TYPE_REDUCED_RGBA32,
} cpres_pngx_lossy_type_t;

typedef struct {
  /* WebP */
  float webp_quality;          /* WebP quality (0-100) */
  bool webp_lossless;          /* false: lossy, true: lossless */
  int webp_method;             /* Compression method (0-6, higher = slower/better) */
  int webp_target_size;        /* Target size in bytes (0 = no target) */
  float webp_target_psnr;      /* Target PSNR (0 = no target) */
  int webp_segments;           /* Number of segments (1-4) */
  int webp_sns_strength;       /* Spatial noise shaping (0-100) */
  int webp_filter_strength;    /* Filter strength (0-100) */
  int webp_filter_sharpness;   /* Filter sharpness (0-7) */
  int webp_filter_type;        /* Filter type (0=simple, 1=strong) */
  bool webp_autofilter;        /* Auto-adjust filter */
  bool webp_alpha_compression; /* Alpha compression */
  int webp_alpha_filtering;    /* Alpha filtering level (0-2) */
  int webp_alpha_quality;      /* Alpha quality (0-100) */
  int webp_pass;               /* Number of entropy passes (1-10) */
  int webp_preprocessing;      /* Preprocessing filter (0-2) */
  int webp_partitions;         /* Log2 of number of partitions (0-3) */
  int webp_partition_limit;    /* Quality degradation limit (0-100) */
  bool webp_emulate_jpeg_size; /* Emulate JPEG size */
  int webp_thread_level;       /* Threading level */
  bool webp_low_memory;        /* Low memory mode */
  int webp_near_lossless;      /* Near-lossless encoding level (0-100) */
  bool webp_exact;             /* Preserve exact pixels */
  bool webp_use_delta_palette; /* Use delta palette */
  bool webp_use_sharp_yuv;     /* Use sharp YUV conversion */
  /* AVIF */
  float avif_quality;     /* Color quality (0-100 or lossless flag) */
  int avif_alpha_quality; /* Alpha quality (0-100) */
  bool avif_lossless;     /* Lossless encode request */
  int avif_speed;         /* Encoder speed (0-10, higher=faster, lower=better) */
  int avif_threads;       /* Max threads (>=1) */
  /* PNGX */
  int pngx_level;                                       /* Optimization preset level (0-6) */
  bool pngx_strip_safe;                                 /* Strip safe-to-remove ancillary chunks */
  bool pngx_optimize_alpha;                             /* Enable alpha channel specific optimizations */
  bool pngx_lossy_enable;                               /* Enable lossy pre-quantization step */
  int pngx_lossy_type;                                  /* See cpres_pngx_lossy_type_t */
  int pngx_lossy_max_colors;                            /* Max palette colors (2-256 for indexed PNG) */
  int pngx_lossy_reduced_colors;                        /* Reduced RGBA32 target colors (-1 auto, >=2 manual) */
  int pngx_lossy_reduced_bits_rgb;                      /* Reduced RGBA32 grid bits for RGB (1-8) */
  int pngx_lossy_reduced_alpha_bits;                    /* Reduced RGBA32 grid bits for alpha (1-8) */
  int pngx_lossy_quality_min;                           /* Min quality hint (0-100) */
  int pngx_lossy_quality_max;                           /* Max quality hint (0-100) */
  int pngx_lossy_speed;                                 /* Quantization speed (1-10; higher=faster lower=better) */
  float pngx_lossy_dither_level;                        /* Dithering level (0.0 - 1.0, or -1 for auto in Unity modes) */
  bool pngx_saliency_map_enable;                        /* Enable saliency-guided importance map */
  bool pngx_chroma_anchor_enable;                       /* Preserve high-chroma gradients */
  bool pngx_adaptive_dither_enable;                     /* Enable adaptive dithering strength */
  bool pngx_gradient_boost_enable;                      /* Boost gradients for important regions */
  bool pngx_chroma_weight_enable;                       /* Emphasize chroma in importance weighting */
  bool pngx_postprocess_smooth_enable;                  /* Apply palette smoothing postprocess */
  float pngx_postprocess_smooth_importance_cutoff;      /* Importance cutoff for smoothing (-1 disables gating) */
  bool pngx_palette256_gradient_profile_enable;         /* Enable gradient-profile auto tuning in palette256 */
  float pngx_palette256_gradient_dither_floor;          /* Override gradient-profile dither floor (0.0-1.0, -1 = internal default) */
  bool pngx_palette256_alpha_bleed_enable;              /* Enable alpha fringe mitigation (palette256 only) */
  int pngx_palette256_alpha_bleed_max_distance;         /* Max bleed propagation distance (0-65535) */
  int pngx_palette256_alpha_bleed_opaque_threshold;     /* Opaque seed alpha threshold (0-255) */
  int pngx_palette256_alpha_bleed_soft_limit;           /* Apply bleed when alpha <= soft limit (0-255) */
  float pngx_palette256_profile_opaque_ratio_threshold; /* Gradient profile: opaque ratio threshold (0.0-1.0, -1 = internal default) */
  float pngx_palette256_profile_gradient_mean_max;      /* Gradient profile: gradient mean max (0.0-1.0, -1 = internal default) */
  float pngx_palette256_profile_saturation_mean_max;    /* Gradient profile: saturation mean max (0.0-1.0, -1 = internal default) */
  float pngx_palette256_tune_opaque_ratio_threshold;    /* Auto tune: opaque ratio threshold (0.0-1.0, -1 = internal default) */
  float pngx_palette256_tune_gradient_mean_max;         /* Auto tune: gradient mean max (0.0-1.0, -1 = internal default) */
  float pngx_palette256_tune_saturation_mean_max;       /* Auto tune: saturation mean max (0.0-1.0, -1 = internal default) */
  int pngx_palette256_tune_speed_max;                   /* Auto tune: max speed override (1-10, -1 = internal default) */
  int pngx_palette256_tune_quality_min_floor;           /* Auto tune: min quality floor (0-100, -1 = internal default) */
  int pngx_palette256_tune_quality_max_target;          /* Auto tune: max quality target (0-100, -1 = internal default) */
  cpres_rgba_color_t *pngx_protected_colors;            /* Array of colors to protect from quantization (NULL if none) */
  int pngx_protected_colors_count;                      /* Number of protected colors (0 if none, max 256) */
  int pngx_threads;                                     /* Max threads (>=0, 0=auto) */
} cpres_config_t;

typedef enum {
  CPRES_OK = 0,
  CPRES_ERROR_FILE_NOT_FOUND = 1,
  CPRES_ERROR_INVALID_PNG = 2,
  CPRES_ERROR_INVALID_FORMAT = 3,
  CPRES_ERROR_OUT_OF_MEMORY = 4,
  CPRES_ERROR_ENCODE_FAILED = 5,
  CPRES_ERROR_DECODE_FAILED = 6,
  CPRES_ERROR_IO = 7,
  CPRES_ERROR_INVALID_PARAMETER = 8,
  CPRES_ERROR_OUTPUT_NOT_SMALLER = 9,
} cpres_error_t;

typedef enum {
  CPRES_LOG_LEVEL_DEBUG = 0,
  CPRES_LOG_LEVEL_INFO = 1,
  CPRES_LOG_LEVEL_WARNING = 2,
  CPRES_LOG_LEVEL_ERROR = 3,
} colopresso_log_level_t;

typedef void (*colopresso_log_callback_t)(colopresso_log_level_t level, const char *message);

extern void cpres_config_init_defaults(cpres_config_t *config);

extern cpres_error_t cpres_encode_webp_memory(const uint8_t *png_data, size_t png_size, uint8_t **webp_data, size_t *webp_size, const cpres_config_t *config);
extern cpres_error_t cpres_encode_avif_memory(const uint8_t *png_data, size_t png_size, uint8_t **avif_data, size_t *avif_size, const cpres_config_t *config);
extern cpres_error_t cpres_encode_pngx_memory(const uint8_t *png_data, size_t png_size, uint8_t **optimized_data, size_t *optimized_size, const cpres_config_t *config);

#if COLOPRESSO_WITH_FILE_OPS
#include <colopresso/file.h>
#endif

extern void cpres_set_log_callback(colopresso_log_callback_t callback);
extern const char *cpres_error_string(cpres_error_t error);

extern void cpres_free(uint8_t *data);

extern uint32_t cpres_get_version(void);
extern uint32_t cpres_get_libwebp_version(void);
extern uint32_t cpres_get_libpng_version(void);
extern uint32_t cpres_get_libavif_version(void);

extern uint32_t cpres_get_pngx_oxipng_version(void);
extern uint32_t cpres_get_pngx_libimagequant_version(void);

extern uint32_t cpres_get_buildtime(void);
extern const char *cpres_get_compiler_version_string(void);
extern const char *cpres_get_rust_version_string(void);

extern bool cpres_is_threads_enabled(void);
extern uint32_t cpres_get_default_thread_count(void);
extern uint32_t cpres_get_max_thread_count(void);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_H */
