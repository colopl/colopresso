# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

function(colopresso_configure_simd)
  if(NOT COLOPRESSO_ENABLE_SIMD)
    return()
  endif()

  if(EMSCRIPTEN)
    if(COLOPRESSO_ENABLE_WASM_SIMD)
      set(COLOPRESSO_SIMD_FLAGS "-msimd128" PARENT_SCOPE)
      set(COLOPRESSO_SIMD_TYPE "WASM_SIMD128" PARENT_SCOPE)
    endif()
  elseif(MSVC)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
      set(COLOPRESSO_HAS_NEON ON PARENT_SCOPE)
      set(COLOPRESSO_SIMD_TYPE "NEON" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64|x86")
      set(COLOPRESSO_SIMD_TYPE "SSE4.1" PARENT_SCOPE)
    endif()
  else()
    include(CheckCCompilerFlag)
    check_c_compiler_flag("-mfpu=neon" _has_neon_flag)

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64|ARM64")
      set(COLOPRESSO_HAS_NEON ON PARENT_SCOPE)
      set(COLOPRESSO_SIMD_TYPE "NEON" PARENT_SCOPE)
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm" AND _has_neon_flag)
      set(COLOPRESSO_HAS_NEON ON PARENT_SCOPE)
      set(COLOPRESSO_SIMD_FLAGS "-mfpu=neon" PARENT_SCOPE)
      set(COLOPRESSO_SIMD_TYPE "NEON" PARENT_SCOPE)
    else()
      check_c_compiler_flag("-msse4.1" _has_sse41)
      if(_has_sse41)
        set(COLOPRESSO_SIMD_FLAGS "-msse4.1" PARENT_SCOPE)
        set(COLOPRESSO_SIMD_TYPE "SSE4.1" PARENT_SCOPE)
      endif()
    endif()
  endif()
endfunction()

# Thread
function(colopresso_configure_threads)
  if(EMSCRIPTEN AND NOT COLOPRESSO_ELECTRON_APP AND COLOPRESSO_ENABLE_THREADS)
    message(STATUS "pthreads disabled for Emscripten build (non-Electron)")
    set(COLOPRESSO_ENABLE_THREADS OFF CACHE BOOL "Pthreads disabled for Emscripten" FORCE)
  elseif(COLOPRESSO_CHROME_EXTENSION AND COLOPRESSO_ENABLE_THREADS)
    message(STATUS "pthreads disabled for Chrome Extension (SharedArrayBuffer unavailable)")
    set(COLOPRESSO_ENABLE_THREADS OFF CACHE BOOL "Pthreads disabled for Chrome Extension" FORCE)
  endif()

  set(COLOPRESSO_ENABLE_THREADS ${COLOPRESSO_ENABLE_THREADS} PARENT_SCOPE)
endfunction()

function(colopresso_apply_global_threads_compile_options)
  if(COLOPRESSO_ENABLE_THREADS AND EMSCRIPTEN)
    message(STATUS "Applying global pthreads compile options for Emscripten")
    message(STATUS "NOTE: WebAssembly pthreads require SharedArrayBuffer and COOP/COEP headers")
    add_compile_options("-pthread")
  endif()
endfunction()

function(colopresso_apply_threads_link_options TARGET)
  if(NOT COLOPRESSO_ENABLE_THREADS)
    return()
  endif()

  message(STATUS "threads support enabled")
  target_compile_definitions(${TARGET} PUBLIC COLOPRESSO_ENABLE_THREADS=1)

  if(EMSCRIPTEN)
    message(STATUS "Configuring threads link options for Emscripten/WebAssembly")
    target_link_options(${TARGET} PUBLIC
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
      message(FATAL_ERROR "threads not found. Disable COLOPRESSO_ENABLE_THREADS or install pthreads.")
    endif()
    target_link_libraries(${TARGET} PUBLIC Threads::Threads)
    message(STATUS "Native threads linked")
  endif()
endfunction()


# Debugging
function(colopresso_configure_debug)
  if(NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EMSCRIPTEN AND NOT MSVC))
    return()
  endif()

  message(STATUS "Debug mode enabled")

  if(COLOPRESSO_USE_COVERAGE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "GNU")
      message(FATAL_ERROR "Coverage is only supported with GNU C compilers")
    endif()

    find_program(LCOV lcov)
    find_program(GENHTML genhtml)

    if(LCOV AND GENHTML)
      set(COLOPRESSO_ENABLE_COVERAGE ON PARENT_SCOPE)
      link_libraries(gcov)
      message(STATUS "Coverage enabled with lcov/gcov")

      if(COLOPRESSO_USE_VALGRIND)
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -Og -fno-omit-frame-pointer" PARENT_SCOPE)
        message(STATUS "Valgrind enabled: using -Og for faster Debug builds")
      else()
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -fno-inline -fno-omit-frame-pointer" PARENT_SCOPE)
      endif()
    else()
      message(FATAL_ERROR "lcov or genhtml not found")
    endif()
  else()
    set(COLOPRESSO_ENABLE_COVERAGE OFF PARENT_SCOPE)
  endif()

  set(_sanitizers)
  if(COLOPRESSO_USE_ASAN)
    list(APPEND _sanitizers address)
  endif()
  if(COLOPRESSO_USE_MSAN)
    list(APPEND _sanitizers memory)
  endif()
  if(COLOPRESSO_USE_UBSAN)
    list(APPEND _sanitizers undefined)
  endif()

  if(_sanitizers)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
      message(FATAL_ERROR "Sanitizers are only supported with clang compilers")
    endif()

    if(COLOPRESSO_USE_ASAN AND COLOPRESSO_USE_MSAN)
      message(FATAL_ERROR "ASAN and MSAN cannot be used together")
    endif()

    if(COLOPRESSO_USE_MSAN)
      if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64|ARM64)$")
        set(PNG_ARM_NEON off CACHE STRING "Disable Arm NEON with MemorySanitizer" FORCE)
        message(STATUS "Disabling libpng Arm NEON for MemorySanitizer")
      elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
        set(ENABLE_NASM 0 CACHE BOOL "Disable NASM/assembly for libaom under MemorySanitizer" FORCE)
        message(STATUS "Disabling libaom NASM/assembly for MemorySanitizer")
      endif()

      set(AOM_TARGET_CPU "generic" CACHE STRING "Use generic libaom for MemorySanitizer" FORCE)
      message(STATUS "Setting libaom target CPU to 'generic' for MemorySanitizer")

      if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
        message(STATUS "Keeping libwebp SIMD under MemorySanitizer on amd64")
      else()
        set(WEBP_ENABLE_SIMD OFF CACHE BOOL "Disable libwebp SIMD for MemorySanitizer" FORCE)
        message(STATUS "Disabling libwebp SIMD for MemorySanitizer")
      endif()
    endif()

    set(_sanitizer_flags)
    foreach(_sanitizer ${_sanitizers})
      list(APPEND _sanitizer_flags "-fsanitize=${_sanitizer}")
      message(STATUS "${_sanitizer}Sanitizer enabled")
    endforeach()

    string(JOIN " " _sanitizer_string ${_sanitizer_flags})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_sanitizer_string}" PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_sanitizer_string}" PARENT_SCOPE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${_sanitizer_string}" PARENT_SCOPE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${_sanitizer_string}" PARENT_SCOPE)
  else()
    find_program(VALGRIND "valgrind")
    if(COLOPRESSO_USE_VALGRIND AND VALGRIND)
      message(STATUS "Found Valgrind: ${VALGRIND}")
      set(COLOPRESSO_ENABLE_VALGRIND ON PARENT_SCOPE)
    else()
      set(COLOPRESSO_ENABLE_VALGRIND OFF PARENT_SCOPE)
    endif()
  endif()
endfunction()

# Compiler info
function(colopresso_get_compiler_version_string OUT_VAR)
  if(EMSCRIPTEN)
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} --version
      OUTPUT_VARIABLE _output
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    string(REGEX MATCH "emcc[^)]*[0-9]+\\.[0-9]+\\.[0-9]+" _version "${_output}")
    if(NOT _version)
      string(REGEX REPLACE "\n.*" "" _version "${_output}")
    endif()
    set(${OUT_VAR} "${_version}" PARENT_SCOPE)
  else()
    set(_version "${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
    string(REPLACE "\\" "\\\\" _version "${_version}")
    string(REPLACE "\"" "\\\"" _version "${_version}")
    set(${OUT_VAR} "${_version}" PARENT_SCOPE)
  endif()
endfunction()
