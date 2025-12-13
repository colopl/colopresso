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

#include <stdbool.h>
#include <string.h>

#include <colopresso/datetime.h>

#include <unity.h>

#define TS 783133323ULL /* 1994-10-26 01:02:03 UTC */

void setUp(void) {}

void tearDown(void) {}

void test_datetime_boundary_years(void) {
  cpres_datetime_t *pdt = NULL;
  cpres_datetime_buildtime_t bt = 0, ts_2025 = 1735689600ULL;

  pdt = cpres_datetime_create_from_timestamp(0);
  if (pdt != NULL) {
    bt = cpres_datetime_encode_buildtime(0);

    TEST_ASSERT_NOT_EQUAL(0, bt);

    cpres_datetime_destroy(pdt);
  }

  pdt = cpres_datetime_create_from_timestamp(ts_2025);

  TEST_ASSERT_NOT_NULL(pdt);
  TEST_ASSERT_EQUAL_UINT16(2025, cpres_datetime_get_year(pdt));

  cpres_datetime_destroy(pdt);
}

void test_datetime_buildtime2str(void) {
  cpres_datetime_buildtime_t bt = 0, ts = TS;
  cpres_datetime_buildtime_str_t str;

  if (sizeof(void *) == 4) {
    TEST_IGNORE_MESSAGE("Skipping on 32-bit architecture");
    return;
  }

  bt = cpres_datetime_encode_buildtime(ts);

  TEST_ASSERT_TRUE(cpres_datetime_buildtime2str(bt, str));
  TEST_ASSERT_EQUAL_STRING("1994-10-26 01:02 UTC", str);
}

void test_datetime_create(void) {
  cpres_datetime_t *pdt = NULL;

  pdt = cpres_datetime_create();

  TEST_ASSERT_NOT_NULL(pdt);

  TEST_ASSERT_GREATER_THAN(0, cpres_datetime_get_year(pdt));
  TEST_ASSERT_GREATER_THAN(0, cpres_datetime_get_mon(pdt));
  TEST_ASSERT_GREATER_THAN(0, cpres_datetime_get_day(pdt));

  cpres_datetime_destroy(pdt);
}

void test_datetime_create_from_timestamp(void) {
  cpres_datetime_t *pdt = NULL;
  cpres_datetime_timestamp_t ts = TS;

  pdt = cpres_datetime_create_from_timestamp(ts);

  TEST_ASSERT_NOT_NULL(pdt);

  TEST_ASSERT_EQUAL_UINT16(1994, cpres_datetime_get_year(pdt));
  TEST_ASSERT_EQUAL_UINT8(10, cpres_datetime_get_mon(pdt));
  TEST_ASSERT_EQUAL_UINT8(26, cpres_datetime_get_day(pdt));
  TEST_ASSERT_EQUAL_UINT8(1, cpres_datetime_get_hour(pdt));
  TEST_ASSERT_EQUAL_UINT8(2, cpres_datetime_get_min(pdt));
  TEST_ASSERT_EQUAL_UINT8(3, cpres_datetime_get_sec(pdt));

  cpres_datetime_destroy(pdt);
}

void test_datetime_encode_decode_buildtime(void) {
  cpres_datetime_timestamp_t ts = TS, decoded_ts = 0;
  cpres_datetime_buildtime_t bt = 0;

  if (sizeof(void *) == 4) {
    TEST_IGNORE_MESSAGE("Skipping on 32-bit architecture");
    return;
  }

  bt = cpres_datetime_encode_buildtime(ts);

  TEST_ASSERT_NOT_EQUAL(0, bt);

  decoded_ts = cpres_datetime_decode_buildtime(bt);

  TEST_ASSERT_INT64_WITHIN(60, ts, decoded_ts);
}

void test_datetime_encode_decode_zero(void) {
  cpres_datetime_buildtime_t bt = 0, ts = 0;
  cpres_datetime_buildtime_str_t str;
  bool ok = false;

  bt = cpres_datetime_encode_buildtime(0);
  ts = cpres_datetime_decode_buildtime(bt);

  TEST_ASSERT(ts >= 0);

  ok = cpres_datetime_buildtime2str(bt, str);

  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT(strstr(str, "1970-01-01") != NULL);
}

void test_datetime_get_timestamp(void) {
  cpres_datetime_t *pdt = NULL;
  cpres_datetime_timestamp_t ts = TS, retrieved_ts = 0;

  if (sizeof(void *) == 4) {
    TEST_IGNORE_MESSAGE("Skipping on 32-bit architecture");
    return;
  }

  pdt = cpres_datetime_create_from_timestamp(ts);

  TEST_ASSERT_NOT_NULL(pdt);

  retrieved_ts = cpres_datetime_get_timestamp(pdt);

  TEST_ASSERT_EQUAL_UINT64(ts, retrieved_ts);

  cpres_datetime_destroy(pdt);
}

void test_datetime_null_checks(void) {
  if (sizeof(void *) == 4) {
    TEST_IGNORE_MESSAGE("Skipping on 32-bit architecture");
    return;
  }

  TEST_ASSERT_EQUAL_UINT16(0, cpres_datetime_get_year(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_mon(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_day(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_hour(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_min(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_sec(NULL));
  TEST_ASSERT_EQUAL_UINT64(0, cpres_datetime_get_timestamp(NULL));

  cpres_datetime_destroy(NULL);
}

void test_datetime_null_helpers(void) {
  TEST_ASSERT_EQUAL_UINT16(0, cpres_datetime_get_year(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_mon(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_day(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_hour(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_min(NULL));
  TEST_ASSERT_EQUAL_UINT8(0, cpres_datetime_get_sec(NULL));
  TEST_ASSERT_EQUAL_UINT(0, cpres_datetime_get_timestamp(NULL));
}

void test_datetime_timestamp(void) {
  cpres_datetime_timestamp_t ts = 0;

  ts = cpres_datetime_timestamp();
  TEST_ASSERT_GREATER_THAN(0, ts);
}

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_datetime_boundary_years);
  RUN_TEST(test_datetime_buildtime2str);
  RUN_TEST(test_datetime_create);
  RUN_TEST(test_datetime_create_from_timestamp);
  RUN_TEST(test_datetime_encode_decode_buildtime);
  RUN_TEST(test_datetime_encode_decode_zero);
  RUN_TEST(test_datetime_get_timestamp);
  RUN_TEST(test_datetime_null_checks);
  RUN_TEST(test_datetime_null_helpers);
  RUN_TEST(test_datetime_timestamp);

  return UNITY_END();
}
