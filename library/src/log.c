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

#include <stdarg.h>
#include <stdio.h>

#include <colopresso.h>

#include "internal/log.h"

#if defined(__EMSCRIPTEN__) && defined(COLOPRESSO_ELECTRON_APP)
#include <emscripten/emscripten.h>
#endif

#ifndef CPRES_DEBUG
#ifdef DEBUG
#define CPRES_DEBUG 1
#else
#define CPRES_DEBUG 0
#endif
#endif

static colopresso_log_callback_t g_log_callback = NULL;

void colopresso_log(colopresso_log_level_t level, const char *format, ...) {
  const char truncated[] = "... [truncated]";
  char buffer[2048];
  int result;
  va_list args;
  size_t pos;

  if (g_log_callback) {
    va_start(args, format);
    result = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (result >= (int)sizeof(buffer)) {
      pos = sizeof(buffer) - sizeof(truncated) - 1;
      if (pos > 0) {
        snprintf(buffer + pos, sizeof(truncated), "%s", truncated);
      }
    }

    g_log_callback(level, buffer);
  }

#if defined(__EMSCRIPTEN__) && defined(COLOPRESSO_ELECTRON_APP) && level >= CPRES_LOG_LEVEL_DEBUG
  else {
    int flags = EM_LOG_CONSOLE;

    if (level >= CPRES_LOG_LEVEL_ERROR) {
      flags |= EM_LOG_ERROR;
    } else if (level >= CPRES_LOG_LEVEL_WARNING) {
      flags |= EM_LOG_WARN;
    } else if (!CPRES_DEBUG) {
      return;
    }

    va_start(args, format);
    result = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (result >= (int)sizeof(buffer)) {
      pos = sizeof(buffer) - sizeof(truncated) - 1;
      if (pos > 0) {
        snprintf(buffer + pos, sizeof(truncated), "%s", truncated);
      }
    }

    emscripten_log(flags, "%s", buffer);
  }
#else
  else if (CPRES_DEBUG && level >= CPRES_LOG_LEVEL_DEBUG) {
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
  }
#endif
}

void colopresso_log_reset(void) { g_log_callback = NULL; }

extern void cpres_set_log_callback(colopresso_log_callback_t callback) {
  if (!callback) {
    colopresso_log_reset();
    return;
  }

  g_log_callback = callback;
}
