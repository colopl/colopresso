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

#ifndef COLOPRESSO_INTERNAL_THREADS_H
#define COLOPRESSO_INTERNAL_THREADS_H

#include <colopresso/portable.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*parallel_func_t)(void *context, uint32_t start_index, uint32_t end_index);
bool colopresso_parallel_for(uint32_t use_threads, uint32_t total_items, parallel_func_t func, void *context);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_INTERNAL_THREADS_H */
