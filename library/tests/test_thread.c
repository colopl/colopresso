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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <colopresso.h>
#include <colopresso/portable.h>

#include <unity.h>

#include "../src/internal/threads.h"

void setUp(void) {}

void tearDown(void) {}

#if COLOPRESSO_ENABLE_THREADS

typedef struct {
  uint32_t call_count;
  uint32_t total_items;
  colopresso_mutex_t mutex;
} parallel_test_ctx_t;

static void parallel_test_worker(void *context, uint32_t start_index, uint32_t end_index) {
  parallel_test_ctx_t *ctx = (parallel_test_ctx_t *)context;

  if (ctx) {
    colopresso_mutex_lock(&ctx->mutex);
    ctx->call_count += 1;
    ctx->total_items += (end_index - start_index);
    colopresso_mutex_unlock(&ctx->mutex);
  }
}

void test_thread_is_threads_enabled(void) {
  bool enabled = cpres_is_threads_enabled();

  TEST_ASSERT_TRUE(enabled);
}

void test_thread_get_default_thread_count(void) {
  uint32_t count = cpres_get_default_thread_count();

  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1, count);
}

void test_thread_get_max_thread_count(void) {
  uint32_t count = cpres_get_max_thread_count();

  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1, count);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(cpres_get_default_thread_count(), count);
}

void test_thread_parallel_for_with_null_func(void) {
  bool result = colopresso_parallel_for(1, 10, NULL, NULL);

  TEST_ASSERT_FALSE(result);
}

void test_thread_parallel_for_with_zero_items(void) {
  parallel_test_ctx_t ctx = {0, 0};
  bool result;

  colopresso_mutex_init(&ctx.mutex, NULL);
  result = colopresso_parallel_for(1, 0, parallel_test_worker, &ctx);

  TEST_ASSERT_FALSE(result);
  TEST_ASSERT_EQUAL_UINT32(0, ctx.call_count);
  colopresso_mutex_destroy(&ctx.mutex);
}

void test_thread_parallel_for_single_threaded(void) {
  parallel_test_ctx_t ctx = {0, 0};
  bool result;

  colopresso_mutex_init(&ctx.mutex, NULL);
  result = colopresso_parallel_for(1, 100, parallel_test_worker, &ctx);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL_UINT32(1, ctx.call_count);
  TEST_ASSERT_EQUAL_UINT32(100, ctx.total_items);
  colopresso_mutex_destroy(&ctx.mutex);
}

void test_thread_parallel_for_multi_threaded(void) {
  parallel_test_ctx_t ctx = {0, 0};
  bool result;

  colopresso_mutex_init(&ctx.mutex, NULL);
  result = colopresso_parallel_for(4, 100, parallel_test_worker, &ctx);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL_UINT32(100, ctx.total_items);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(1, ctx.call_count);
  colopresso_mutex_destroy(&ctx.mutex);
}

void test_thread_parallel_for_default_threads(void) {
  parallel_test_ctx_t ctx = {0, 0};
  bool result;

  colopresso_mutex_init(&ctx.mutex, NULL);
  result = colopresso_parallel_for(0, 50, parallel_test_worker, &ctx);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL_UINT32(50, ctx.total_items);
  colopresso_mutex_destroy(&ctx.mutex);
}

void test_thread_parallel_for_more_threads_than_items(void) {
  parallel_test_ctx_t ctx = {0, 0};
  bool result;

  colopresso_mutex_init(&ctx.mutex, NULL);
  result = colopresso_parallel_for(10, 3, parallel_test_worker, &ctx);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL_UINT32(3, ctx.total_items);
  colopresso_mutex_destroy(&ctx.mutex);
}

void test_thread_parallel_for_large_item_count(void) {
  parallel_test_ctx_t ctx = {0, 0};
  bool result;

  colopresso_mutex_init(&ctx.mutex, NULL);
  result = colopresso_parallel_for(4, 10000, parallel_test_worker, &ctx);

  TEST_ASSERT_TRUE(result);
  TEST_ASSERT_EQUAL_UINT32(10000, ctx.total_items);
  colopresso_mutex_destroy(&ctx.mutex);
}

#endif

void test_thread_dummy(void) { TEST_ASSERT_TRUE(true); }

int main(void) {
  UNITY_BEGIN();

#if COLOPRESSO_ENABLE_THREADS
  RUN_TEST(test_thread_is_threads_enabled);
  RUN_TEST(test_thread_get_default_thread_count);
  RUN_TEST(test_thread_get_max_thread_count);
  RUN_TEST(test_thread_parallel_for_with_null_func);
  RUN_TEST(test_thread_parallel_for_with_zero_items);
  RUN_TEST(test_thread_parallel_for_single_threaded);
  RUN_TEST(test_thread_parallel_for_multi_threaded);
  RUN_TEST(test_thread_parallel_for_default_threads);
  RUN_TEST(test_thread_parallel_for_more_threads_than_items);
  RUN_TEST(test_thread_parallel_for_large_item_count);
#endif

  RUN_TEST(test_thread_dummy);

  return UNITY_END();
}
