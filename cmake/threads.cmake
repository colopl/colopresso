# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

option(COLOPRESSO_ENABLE_THREADS "Enable threads for multi-threaded image processing" ON)

if(COLOPRESSO_CHROME_EXTENSION AND COLOPRESSO_ENABLE_THREADS)
  message(STATUS "pthreads disabled for Chrome Extension (SharedArrayBuffer unavailable)")
  set(COLOPRESSO_ENABLE_THREADS OFF CACHE BOOL "Pthreads disabled for Chrome Extension" FORCE)
endif()

function(colopresso_apply_global_threads_compile_options)
  if(COLOPRESSO_ENABLE_THREADS AND EMSCRIPTEN)
    message(STATUS "Applying global pthreads compile options for Emscripten")
    message(STATUS "NOTE: WebAssembly pthreads require SharedArrayBuffer and COOP/COEP headers")
    add_compile_options("-pthread")
  endif()
endfunction()

function(colopresso_apply_threads_link_options)
  if(COLOPRESSO_ENABLE_THREADS)
    message(STATUS "threads support enabled")

    target_compile_definitions(colopresso PUBLIC COLOPRESSO_ENABLE_THREADS=1)

    if(EMSCRIPTEN)
      message(STATUS "Configuring threads link options for Emscripten/WebAssembly")
      target_link_options(colopresso PUBLIC
        "-pthread"
        "-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency"
        "-sPTHREAD_POOL_SIZE_STRICT=0"
        "-sSHARED_MEMORY=1"
      )
    elseif(WIN32)
      message(STATUS "Using Windows Thread API compatibility layer")
    else()
      set(THREADS_PREFER_PTHREAD_FLAG ON)
      find_package(Threads REQUIRED)
      if(NOT CMAKE_USE_PTHREADS_INIT)
        message(FATAL_ERROR "threads not found on this system. Disable COLOPRESSO_ENABLE_THREADS or install pthreads.")
      endif()
      target_link_libraries(colopresso PUBLIC Threads::Threads)
      message(STATUS "Native threads linked")
    endif()
  endif()
endfunction()
