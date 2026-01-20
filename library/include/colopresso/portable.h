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

#ifndef COLOPRESSO_PORTABLE_H
#define COLOPRESSO_PORTABLE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include <sys/stat.h>
#include <sys/types.h>
#include <windows.h>

struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

#define no_argument 0
#define required_argument 1
#define optional_argument 2

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

extern int getopt_long(int argc, char *const argv[], const char *optstring, const struct option *longopts, int *longindex);
extern struct tm *gmtime_r(const time_t *timer, struct tm *buf);

#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#define stat _stat64
#define strtok_r strtok_s

typedef HANDLE colopresso_thread_t;

typedef struct {
  void *(*start_routine)(void *);
  void *arg;
} cpres_pthread_wrapper_ctx_t;

extern int colopresso_thread_create(colopresso_thread_t *thread, const void *attr, void *(*start_routine)(void *), void *arg);
extern int colopresso_thread_join(colopresso_thread_t thread, void **retval);

typedef CRITICAL_SECTION colopresso_mutex_t;

extern int colopresso_mutex_init(colopresso_mutex_t *mutex, const void *attr);
extern int colopresso_mutex_lock(colopresso_mutex_t *mutex);
extern int colopresso_mutex_unlock(colopresso_mutex_t *mutex);
extern int colopresso_mutex_destroy(colopresso_mutex_t *mutex);

#else

#include <getopt.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

typedef pthread_t colopresso_thread_t;
typedef pthread_mutex_t colopresso_mutex_t;

#define colopresso_thread_create pthread_create
#define colopresso_thread_join pthread_join
#define colopresso_mutex_init pthread_mutex_init
#define colopresso_mutex_lock pthread_mutex_lock
#define colopresso_mutex_unlock pthread_mutex_unlock
#define colopresso_mutex_destroy pthread_mutex_destroy

#endif

extern uint32_t colopresso_get_cpu_count(void);
extern const char *colopresso_extract_extension(const char *path);
extern void colopresso_tm_set_gmt_offset(struct tm *tm);

#if COLOPRESSO_WITH_FILE_OPS
extern bool colopresso_fseeko(FILE *fp, uint64_t offset, int whence);
extern bool colopresso_ftello(FILE *fp, uint64_t *position_out);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COLOPRESSO_PORTABLE_H */
