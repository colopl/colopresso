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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <colopresso/datetime.h>
#include <colopresso/portable.h>

struct _cpres_datetime_t {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
};

extern cpres_datetime_timestamp_t cpres_datetime_timestamp(void) {
  time_t tm;

  tm = time(NULL);
  if (tm == -1) {
    return 0;
  }

  return (cpres_datetime_timestamp_t)tm;
}

extern void cpres_datetime_destroy(cpres_datetime_t *pdt) {
  if (!pdt) {
    return;
  }

  free(pdt);
}

extern cpres_datetime_t *cpres_datetime_create_from_timestamp(cpres_datetime_timestamp_t timestamp) {
  cpres_datetime_t *pdt;
  time_t t;
  struct tm tm;

  t = (time_t)timestamp;
  if (!gmtime_r(&t, &tm)) {
    return NULL;
  }

  pdt = (cpres_datetime_t *)malloc(sizeof(cpres_datetime_t));
  if (!pdt) {
    return NULL;
  }

  pdt->year = (uint16_t)(tm.tm_year + 1900);
  pdt->month = (uint8_t)tm.tm_mon + 1;
  pdt->day = (uint8_t)tm.tm_mday;
  pdt->hour = (uint8_t)tm.tm_hour;
  pdt->min = (uint8_t)tm.tm_min;
  pdt->sec = (uint8_t)tm.tm_sec;

  return pdt;
}

extern cpres_datetime_t *cpres_datetime_create(void) { return cpres_datetime_create_from_timestamp(cpres_datetime_timestamp()); }

extern cpres_datetime_timestamp_t cpres_datetime_get_timestamp(const cpres_datetime_t *pdt) {
  struct tm tm = {0};
  time_t local_time, gmt_time;
  struct tm gmt_tm;

  if (!pdt) {
    return 0;
  }

  tm.tm_year = pdt->year - 1900;
  tm.tm_mon = pdt->month - 1;
  tm.tm_mday = pdt->day;
  tm.tm_hour = pdt->hour;
  tm.tm_min = pdt->min;
  tm.tm_sec = pdt->sec;
  tm.tm_isdst = -1;
  colopresso_tm_set_gmt_offset(&tm);

  local_time = mktime(&tm);
  if (local_time == -1) {
    return 0;
  }

  gmtime_r(&local_time, &gmt_tm);
  gmt_time = mktime(&gmt_tm);

  return (cpres_datetime_timestamp_t)local_time + (local_time - gmt_time);
}

extern uint16_t cpres_datetime_get_year(const cpres_datetime_t *pdt) {
  if (!pdt) {
    return 0;
  }

  return pdt->year;
}

extern uint8_t cpres_datetime_get_mon(const cpres_datetime_t *pdt) {
  if (!pdt) {
    return 0;
  }

  return pdt->month;
}

extern uint8_t cpres_datetime_get_day(const cpres_datetime_t *pdt) {
  if (!pdt) {
    return 0;
  }

  return pdt->day;
}

extern uint8_t cpres_datetime_get_hour(const cpres_datetime_t *pdt) {
  if (!pdt) {
    return 0;
  }

  return pdt->hour;
}

extern uint8_t cpres_datetime_get_min(const cpres_datetime_t *pdt) {
  if (!pdt) {
    return 0;
  }

  return pdt->min;
}

extern uint8_t cpres_datetime_get_sec(const cpres_datetime_t *pdt) {
  if (!pdt) {
    return 0;
  }

  return pdt->sec;
}

/**
 * 32bit buildtime structure
 *
 * - UTC only.
 * - Theses are not supported:
 *   - summer time
 *   - time zone
 *
 *  year   (Y) : 12bit => 2^12-1 = 4095 use: ALL
 *  month  (M) : 4bit  => 2^4-1  = 15   use: 1-12
 *  day    (D) : 5bit  => 2^5-1  = 31   use: 1-31
 *  hour   (H) : 5bit  => 2^5-1  = 31   use: 0-23
 *  minute (M) : 6bit  => 2^6-1  = 63   use: 0-59
 *
 * HIGH                             LOW
 *  |--------------------------------|
 *  |YYYYYYYYYYYYMMMMDDDDDHHHHHMMMMMM|
 *  |--------------------------------|
 *  32           20  16   11   6     0
 *
 * e.g.
 * - Min: 0000-01-01 00:00 => 00000000000000010000100000000000 => 0x0010800
 * - Max: 4095-12-31 23:59 => 11111111111111001111110111111011 => 0xFFFCFDFB
 */
extern cpres_datetime_buildtime_t cpres_datetime_encode_buildtime(cpres_datetime_timestamp_t timestamp) {
  cpres_datetime_t *pdt;
  cpres_datetime_buildtime_t bt;

  pdt = cpres_datetime_create_from_timestamp(timestamp);
  if (!pdt) {
    return 0;
  }

  bt = 0;
  bt |= ((cpres_datetime_buildtime_t)(pdt->year) & 0xFFF) << 20;
  bt |= ((cpres_datetime_buildtime_t)(pdt->month) & 0xF) << 16;
  bt |= ((cpres_datetime_buildtime_t)(pdt->day) & 0x1F) << 11;
  bt |= ((cpres_datetime_buildtime_t)(pdt->hour) & 0x1F) << 6;
  bt |= ((cpres_datetime_buildtime_t)(pdt->min) & 0x3F);

  cpres_datetime_destroy(pdt);

  return bt;
}

extern cpres_datetime_timestamp_t cpres_datetime_decode_buildtime(cpres_datetime_buildtime_t buildtime) {
  struct tm t = {0};
  time_t local_time, gmt_time;
  struct tm gmt_tm;

  t.tm_year = ((buildtime >> 20) & 0xFFF) - 1900;
  t.tm_mon = ((buildtime >> 16) & 0xF) - 1;
  t.tm_mday = (buildtime >> 11) & 0x1F;
  t.tm_hour = (buildtime >> 6) & 0x1F;
  t.tm_min = buildtime & 0x3F;
  t.tm_sec = 0;
  t.tm_isdst = -1;
  colopresso_tm_set_gmt_offset(&t);

  local_time = mktime(&t);
  if (local_time == -1) {
    return 0;
  }

  gmtime_r(&local_time, &gmt_tm);
  gmt_time = mktime(&gmt_tm);

  return (cpres_datetime_timestamp_t)local_time + (local_time - gmt_time);
}

extern bool cpres_datetime_buildtime2str(cpres_datetime_buildtime_t buildtime, cpres_datetime_buildtime_str_t str) {
  cpres_datetime_t *pdt;

  pdt = cpres_datetime_create_from_timestamp(cpres_datetime_decode_buildtime(buildtime));
  if (pdt == NULL) {
    return false;
  }

  snprintf(str, CPRES_DT_BUILDTIME_LENGTH, "%04u-%02u-%02u %02u:%02u UTC", pdt->year, pdt->month, pdt->day, pdt->hour, pdt->min);

  cpres_datetime_destroy(pdt);

  return true;
}
