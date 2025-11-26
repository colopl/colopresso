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

#ifndef COLOPRESSO_INTERNAL_PNG_H
#define COLOPRESSO_INTERNAL_PNG_H

#include <stdint.h>

#include <png.h>

#include <colopresso.h>

#ifdef __cplusplus
extern "C" {
#endif

#if COLOPRESSO_WITH_FILE_OPS
extern cpres_error_t cpres_png_decode_from_file(const char *filename, uint8_t **rgba_data, png_uint_32 *width, png_uint_32 *height);
#endif

extern cpres_error_t cpres_png_decode_from_memory(const uint8_t *png_data, size_t png_size, uint8_t **rgba_data, png_uint_32 *width, png_uint_32 *height);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_INTERNAL_PNG_H */
