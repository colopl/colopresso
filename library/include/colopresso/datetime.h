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

#ifndef COLOPRESSO_DATETIME_H
#define COLOPRESSO_DATETIME_H

#include <stdbool.h>
#include <stdint.h>

#define CPRES_DT_BUILDTIME_LENGTH 32 /* YYYY-MM-DD HH:ii UTC */
#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t cpres_datetime_timestamp_t;
typedef uint32_t cpres_datetime_buildtime_t;

typedef char cpres_datetime_buildtime_str_t[CPRES_DT_BUILDTIME_LENGTH];

typedef struct _cpres_datetime_t cpres_datetime_t;

extern cpres_datetime_timestamp_t cpres_datetime_timestamp(void);
extern void cpres_datetime_destroy(cpres_datetime_t *pdt);
extern cpres_datetime_t *cpres_datetime_create_from_timestamp(cpres_datetime_timestamp_t timestamp);
extern cpres_datetime_t *cpres_datetime_create(void);
extern cpres_datetime_timestamp_t cpres_datetime_get_timestamp(const cpres_datetime_t *pdt);
extern uint16_t cpres_datetime_get_year(const cpres_datetime_t *pdt);
extern uint8_t cpres_datetime_get_mon(const cpres_datetime_t *pdt);
extern uint8_t cpres_datetime_get_day(const cpres_datetime_t *pdt);
extern uint8_t cpres_datetime_get_hour(const cpres_datetime_t *pdt);
extern uint8_t cpres_datetime_get_min(const cpres_datetime_t *pdt);
extern uint8_t cpres_datetime_get_sec(const cpres_datetime_t *pdt);
extern cpres_datetime_buildtime_t cpres_datetime_encode_buildtime(cpres_datetime_timestamp_t timestamp);
extern cpres_datetime_timestamp_t cpres_datetime_decode_buildtime(cpres_datetime_buildtime_t buildtime);
extern bool cpres_datetime_buildtime2str(cpres_datetime_buildtime_t buildtime, cpres_datetime_buildtime_str_t str);

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_DATETIME_H */
