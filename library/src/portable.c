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

/* This code ensures portability, so coverage measurement is not required */

/* LCOV_EXCL_START */
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <colopresso.h>
#include <colopresso/portable.h>

#ifdef _WIN32

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

char *optarg = NULL;
int optind = 1;
int opterr = 1;
int optopt = 0;

static int nextchar = 0;

static DWORD WINAPI cpres_thread_wrapper(LPVOID arg) {
  cpres_pthread_wrapper_ctx_t *ctx = (cpres_pthread_wrapper_ctx_t *)arg;
  void *(*start_routine)(void *);
  void *routine_arg;

  if (!ctx) {
    return 1;
  }

  start_routine = ctx->start_routine;
  routine_arg = ctx->arg;
  free(ctx);

  if (start_routine) {
    start_routine(routine_arg);
  }

  return 0;
}

int colopresso_thread_create(colopresso_thread_t *thread, const void *attr, void *(*start_routine)(void *), void *arg) {
  cpres_pthread_wrapper_ctx_t *ctx;
  HANDLE handle;

  (void)attr;

  if (!thread || !start_routine) {
    return EINVAL;
  }

  ctx = (cpres_pthread_wrapper_ctx_t *)malloc(sizeof(cpres_pthread_wrapper_ctx_t));
  if (!ctx) {
    return ENOMEM;
  }

  ctx->start_routine = start_routine;
  ctx->arg = arg;

  handle = CreateThread(NULL, 0, cpres_thread_wrapper, ctx, 0, NULL);
  if (handle == NULL) {
    free(ctx);
    return EAGAIN;
  }

  *thread = handle;
  return 0;
}

int colopresso_thread_join(colopresso_thread_t thread, void **retval) {
  DWORD result;

  (void)retval;

  if (thread == NULL || thread == INVALID_HANDLE_VALUE) {
    return EINVAL;
  }

  result = WaitForSingleObject(thread, INFINITE);
  if (result != WAIT_OBJECT_0) {
    return EINVAL;
  }

  CloseHandle(thread);
  return 0;
}

int colopresso_mutex_init(colopresso_mutex_t *mutex, const void *attr) {
  (void)attr;

  if (!mutex) {
    return EINVAL;
  }

  InitializeCriticalSection(mutex);
  return 0;
}

int colopresso_mutex_lock(colopresso_mutex_t *mutex) {
  if (!mutex) {
    return EINVAL;
  }

  EnterCriticalSection(mutex);
  return 0;
}

int colopresso_mutex_unlock(colopresso_mutex_t *mutex) {
  if (!mutex) {
    return EINVAL;
  }

  LeaveCriticalSection(mutex);
  return 0;
}

int colopresso_mutex_destroy(colopresso_mutex_t *mutex) {
  if (!mutex) {
    return EINVAL;
  }

  DeleteCriticalSection(mutex);
  return 0;
}

static const struct option *match_long_option(const char *name, size_t name_len, const struct option *longopts, int *index_out) {
  int i;

  if (!longopts) {
    return NULL;
  }

  for (i = 0; longopts[i].name != NULL; i++) {
    if (strncmp(longopts[i].name, name, name_len) == 0 && longopts[i].name[name_len] == '\0') {
      if (index_out) {
        *index_out = i;
      }
      return &longopts[i];
    }
  }

  return NULL;
}

int getopt_long(int argc, char *const argv[], const char *optstring, const struct option *longopts, int *longindex) {
  const struct option *opt_entry;
  const char *arg, *name, *eq, *spec;
  char opt;
  size_t name_len;

  optarg = NULL;

  if (optind >= argc || !argv || !optstring) {
    return -1;
  }

  arg = argv[optind];
  if (!arg) {
    return -1;
  }

  if (arg[0] != '-' || arg[1] == '\0') {
    return -1;
  }

  if (arg[0] == '-' && arg[1] == '-') {
    if (arg[2] == '\0') {
      optind++;
      return -1;
    }

    name = arg + 2;
    eq = strchr(name, '=');
    name_len = eq ? (size_t)(eq - name) : strlen(name);

    opt_entry = match_long_option(name, name_len, longopts, longindex);
    if (!opt_entry) {
      if (opterr && argv[0]) {
        fprintf(stderr, "%s: unrecognized option '%s'\n", argv[0], arg);
      }
      optind++;
      return '?';
    }

    if (opt_entry->has_arg == required_argument) {
      if (eq) {
        optarg = (char *)(eq + 1);
      } else if ((optind + 1) < argc) {
        optind++;
        optarg = argv[optind];
      } else {
        if (opterr && argv[0]) {
          fprintf(stderr, "%s: option '--%s' requires an argument\n", argv[0], opt_entry->name);
        }
        optopt = opt_entry->flag ? 0 : opt_entry->val;
        optind++;
        return '?';
      }
    } else if (opt_entry->has_arg == optional_argument) {
      if (eq) {
        optarg = (char *)(eq + 1);
      }
    } else if (eq) {
      if (opterr && argv[0]) {
        fprintf(stderr, "%s: option '--%s' doesn't allow an argument\n", argv[0], opt_entry->name);
      }
      optind++;
      return '?';
    }

    optind++;

    if (opt_entry->flag) {
      *(opt_entry->flag) = opt_entry->val;
      return 0;
    }

    return opt_entry->val;
  }

  if (nextchar == 0) {
    nextchar = 1;
  }

  while (nextchar != 0) {
    opt = arg[nextchar];
    if (opt == '\0') {
      optind++;
      nextchar = 0;
      return getopt_long(argc, argv, optstring, longopts, longindex);
    }

    spec = strchr(optstring, opt);
    if (!spec) {
      if (opterr && argv[0]) {
        fprintf(stderr, "%s: invalid option -- '%c'\n", argv[0], opt);
      }
      optopt = opt;
      nextchar++;
      if (arg[nextchar] == '\0') {
        optind++;
        nextchar = 0;
      }
      return '?';
    }

    nextchar++;

    if (spec[1] != ':') {
      if (arg[nextchar] == '\0') {
        optind++;
        nextchar = 0;
      }
      return opt;
    }

    if (arg[nextchar] != '\0') {
      optarg = &arg[nextchar];
      optind++;
      nextchar = 0;
      return opt;
    }

    if (spec[2] == ':') {
      optarg = NULL;
      optind++;
      nextchar = 0;
      return opt;
    }

    if ((optind + 1) < argc) {
      optind++;
      optarg = argv[optind];
      optind++;
      nextchar = 0;
      return opt;
    }

    if (opterr && argv[0]) {
      fprintf(stderr, "%s: option requires an argument -- '%c'\n", argv[0], opt);
    }
    optopt = opt;
    optind++;
    nextchar = 0;
    return '?';
  }

  return -1;
}

struct tm *gmtime_r(const time_t *timer, struct tm *buf) {
  struct tm *tmp;

  if (!timer || !buf) {
    return NULL;
  }

  tmp = gmtime(timer);
  if (!tmp) {
    return NULL;
  }

  *buf = *tmp;
  return buf;
}

#else

#include <stdlib.h>

#endif

uint32_t colopresso_get_cpu_count(void) {
#ifdef _WIN32
  SYSTEM_INFO sysinfo;

  GetSystemInfo(&sysinfo);
  if (sysinfo.dwNumberOfProcessors == 0) {
    return 1;
  }
  return (int32_t)sysinfo.dwNumberOfProcessors;
#else
  long nprocs;

  nprocs = sysconf(_SC_NPROCESSORS_ONLN);
  if (nprocs <= 0) {
    return 1;
  }
  if (nprocs > INT32_MAX) {
    return INT32_MAX;
  }
  return (int32_t)nprocs;
#endif
}

const char *colopresso_extract_extension(const char *path) {
  const char *last_slash, *filename, *dot;
#ifdef _WIN32
  const char *last_backslash;
#endif

  if (!path) {
    return NULL;
  }

  last_slash = strrchr(path, '/');
#ifdef _WIN32
  last_backslash = strrchr(path, '\\');
  if (last_backslash && (!last_slash || last_backslash > last_slash)) {
    last_slash = last_backslash;
  }
#endif

  filename = last_slash ? last_slash + 1 : path;
  dot = strrchr(filename, '.');
  if (!dot || dot == filename) {
    return NULL;
  }

  return dot;
}

void colopresso_tm_set_gmt_offset(struct tm *tm) {
  if (!tm) {
    return;
  }

#ifndef _WIN32
  tm->tm_gmtoff = 0;
#endif
}

#if COLOPRESSO_WITH_FILE_OPS
bool colopresso_fseeko(FILE *fp, uint64_t offset, int whence) {
  if (!fp) {
    return false;
  }

#ifdef _WIN32
  return (uint64_t)_fseeki64(fp, offset, whence) == 0;
#else
  return (uint64_t)fseeko(fp, (off_t)offset, whence) == 0;
#endif
}

bool colopresso_ftello(FILE *fp, uint64_t *position_out) {
  uint64_t pos;

  if (!fp || !position_out) {
    return false;
  }

#ifdef _WIN32
  pos = (uint64_t)_ftelli64(fp);
#else
  pos = (uint64_t)ftello(fp);
#endif

  *position_out = pos;
  return true;
}

#endif

/* LCOV_EXCL_STOP */
