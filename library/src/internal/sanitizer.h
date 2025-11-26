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

#ifndef COLOPRESSO_INTERNAL_SANITIZER_H
#define COLOPRESSO_INTERNAL_SANITIZER_H

#if defined(__has_feature)
#if __has_feature(memory_sanitizer)
#include <sanitizer/msan_interface.h>
#define MSAN_UNPOISON(ptr, size) __msan_unpoison(ptr, size)
#else
#define MSAN_UNPOISON(ptr, size) ((void)0)
#endif
#else
#define MSAN_UNPOISON(ptr, size) ((void)0)
#endif

#endif /* COLOPRESSO_INTERNAL_SANITIZER_H */
