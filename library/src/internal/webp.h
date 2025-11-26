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

#ifndef COLOPRESSO_INTERNAL_WEBP_H
#define COLOPRESSO_INTERNAL_WEBP_H

#include <stdint.h>

#include <png.h>
#include <webp/encode.h>

#include <colopresso.h>

#ifdef __cplusplus
extern "C" {
#endif

extern cpres_error_t cpres_webp_encode_rgba_to_memory(uint8_t *rgba_data, uint32_t width, uint32_t height, uint8_t **webp_data, size_t *webp_size, const cpres_config_t *config);

extern int cpres_webp_get_last_error(void);
extern void cpres_webp_set_last_error(int error_code);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_INTERNAL_WEBP_H */
