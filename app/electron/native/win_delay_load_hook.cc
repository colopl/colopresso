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

/*
 * Windows delay-load hook for the Electron native addon.
 *
 * The addon links against an import library generated from the Node-API .def
 * files (node-api-headers), whose symbols reference "node.exe". At load time the
 * host process is not node.exe but the Electron executable (e.g. electron.exe),
 * so those imports must be redirected to the running process image instead.
 *
 * This mirrors the win_delay_load_hook that node-gyp injects into native
 * addons: when the delay loader is about to load "node.exe", we hand it a
 * handle to the current process so the Node-API symbols resolve against the
 * host binary.
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <delayimp.h>
#include <string.h>

static FARPROC WINAPI colopresso_load_exe_hook(unsigned int event, DelayLoadInfo* info) {
  if (event != dliNotePreLoadLibrary) {
    return NULL;
  }

  if (_stricmp(info->szDll, "node.exe") != 0) {
    return NULL;
  }

  return reinterpret_cast<FARPROC>(GetModuleHandle(NULL));
}

decltype(__pfnDliNotifyHook2) __pfnDliNotifyHook2 = colopresso_load_exe_hook;
