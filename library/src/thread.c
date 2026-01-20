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

#include <colopresso.h>
#include <colopresso/portable.h>

#include "internal/log.h"
#include "internal/threads.h"

#if COLOPRESSO_ENABLE_THREADS

#include <stdlib.h>

typedef struct {
  parallel_func_t func;
  void *context;
  uint32_t start_index;
  uint32_t end_index;
} thread_work_t;

static void *thread_worker(void *arg) {
  thread_work_t *work = (thread_work_t *)arg;

  if (work && work->func) {
    work->func(work->context, work->start_index, work->end_index);
  }

  return NULL;
}

uint32_t cpres_get_default_thread_count(void) {
  uint32_t cpu_count = colopresso_get_cpu_count();
  uint32_t half = (uint32_t)(cpu_count / 2);

  return half > 0 ? half : 1;
}

uint32_t cpres_get_max_thread_count(void) { return colopresso_get_cpu_count(); }

bool cpres_is_threads_enabled(void) { return true; }

bool colopresso_parallel_for(uint32_t use_threads, uint32_t total_items, parallel_func_t func, void *context) {
  colopresso_thread_t *threads;
  thread_work_t *works;
  uint32_t thread_count, chunk_size, remainder, start, end, i;
  int rc;
  bool success = true;

  if (!func || total_items == 0) {
    return false;
  }

  thread_count = use_threads > 0 ? use_threads : cpres_get_default_thread_count();
  if (thread_count > total_items) {
    thread_count = total_items;
  }

  if (thread_count <= 1) {
    func(context, 0, total_items);
    return true;
  }

  threads = (colopresso_thread_t *)malloc(sizeof(colopresso_thread_t) * thread_count);
  works = (thread_work_t *)malloc(sizeof(thread_work_t) * thread_count);
  if (!threads || !works) {
    free(threads);
    free(works);
    func(context, 0, total_items);

    return true;
  }

  chunk_size = total_items / thread_count;
  remainder = total_items % thread_count;
  start = 0;

  for (i = 0; i < thread_count; ++i) {
    end = start + chunk_size;
    if (i < remainder) {
      ++end;
    }

    works[i].func = func;
    works[i].context = context;
    works[i].start_index = start;
    works[i].end_index = end;

    rc = colopresso_thread_create(&threads[i], NULL, thread_worker, &works[i]);
    if (rc != 0) {
      colopresso_log(CPRES_LOG_LEVEL_WARNING, "Threads: colopresso_thread_create failed (rc=%d) - falling back to inline execution", rc);
      works[i].func(works[i].context, works[i].start_index, works[i].end_index);
      threads[i] = (colopresso_thread_t)NULL;
    }

    start = end;
  }

  for (i = 0; i < thread_count; ++i) {
    if (threads[i] != (colopresso_thread_t)NULL) {
      colopresso_thread_join(threads[i], NULL);
    }
  }

  free(threads);
  free(works);

  return success;
}

#else

uint32_t cpres_get_default_thread_count(void) { return 1; }
uint32_t cpres_get_max_thread_count(void) { return 1; }
bool cpres_is_threads_enabled(void) { return false; }

bool colopresso_parallel_for(uint32_t use_threads, uint32_t total_items, parallel_func_t func, void *context) {
  if (!func || total_items == 0) {
    return false;
  }

  func(context, 0, total_items);

  return true;
}

#endif
