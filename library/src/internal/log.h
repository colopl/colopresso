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

#ifndef COLOPRESSO_INTERNAL_LOG_H
#define COLOPRESSO_INTERNAL_LOG_H

#include <colopresso.h>

#ifdef __cplusplus
extern "C" {
#endif

void cpres_log(cpres_log_level_t level, const char *format, ...);
void cpres_log_init(void);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_INTERNAL_LOG_H */
