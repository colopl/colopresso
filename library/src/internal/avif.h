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

#ifndef COLOPRESSO_INTERNAL_AVIF_H
#define COLOPRESSO_INTERNAL_AVIF_H

#include <stdint.h>

#include <png.h>

#include <colopresso.h>

#ifdef __cplusplus
extern "C" {
#endif

cpres_error_t avif_encode_rgba_to_memory(uint8_t *rgba_data, uint32_t width, uint32_t height, uint8_t **avif_data, size_t *avif_size, const cpres_config_t *config);

int avif_get_last_error(void);
void avif_set_last_error(int error_code);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_INTERNAL_AVIF_H */
