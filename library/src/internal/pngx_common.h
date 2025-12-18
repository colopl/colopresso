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

#ifndef COLOPRESSO_INTERNAL_PNGX_COMMON_H
#define COLOPRESSO_INTERNAL_PNGX_COMMON_H

#include "pngx.h"

#ifdef __cplusplus
extern "C" {
#endif

void image_stats_reset(pngx_image_stats_t *stats);
void quant_support_reset(pngx_quant_support_t *support);
const char *lossy_type_label(uint8_t lossy_type);
void rgba_image_reset(pngx_rgba_image_t *image);
bool load_rgba_image(const uint8_t *png_data, size_t png_size, pngx_rgba_image_t *image);
uint8_t clamp_reduced_bits(uint8_t bits);
uint8_t quantize_channel_value(float value, uint8_t bits_per_channel);
uint8_t quantize_bits(uint8_t value, uint8_t bits);
void snap_rgba_to_bits(uint8_t *r, uint8_t *g, uint8_t *b, uint8_t *a, uint8_t bits_rgb, uint8_t bits_alpha);
void snap_rgba_image_to_bits(uint8_t *rgba, size_t pixel_count, uint8_t bits_rgb, uint8_t bits_alpha);
uint32_t color_distance_sq(const cpres_rgba_color_t *lhs, const cpres_rgba_color_t *rhs);
float estimate_bitdepth_dither_level(const uint8_t *rgba, png_uint_32 width, png_uint_32 height, uint8_t bits_per_channel);
void build_fixed_palette(const pngx_options_t *source_opts, pngx_quant_support_t *support, pngx_options_t *patched_opts);
float resolve_quant_dither(const pngx_options_t *opts, const pngx_image_stats_t *stats);
bool prepare_quant_support(const pngx_rgba_image_t *image, const pngx_options_t *opts, pngx_quant_support_t *support, pngx_image_stats_t *stats);
bool create_rgba_png(const uint8_t *rgba, size_t pixel_count, uint32_t width, uint32_t height, uint8_t **out_data, size_t *out_size);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_INTERNAL_PNGX_COMMON_H */
