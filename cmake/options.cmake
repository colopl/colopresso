# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

option(COLOPRESSO_USE_TESTS "Build tests" OFF)
option(COLOPRESSO_USE_UTILS "Build utility programs" OFF)
option(COLOPRESSO_USE_CLI "Build unified CLI application" OFF)
option(COLOPRESSO_WITH_FILE_OPS "Enable file operation functions (fopen, fwrite, etc.)" ON)
option(COLOPRESSO_ENABLE_THREADS "Enable threads support" ON)
option(COLOPRESSO_ENABLE_SIMD "Enable SIMD optimizations" ON)
option(COLOPRESSO_PYTHON_BINDINGS "Build Python bindings" OFF)

if(EMSCRIPTEN)
  option(COLOPRESSO_CHROME_EXTENSION "Build for Chrome Extension" OFF)
  option(COLOPRESSO_ELECTRON_APP "Build for Electron" OFF)
  option(COLOPRESSO_NODE_BUILD "Build for Node.js" ON)
  option(COLOPRESSO_ENABLE_WASM_SIMD "Enable WASM SIMD128 optimizations" ON)
  set(COLOPRESSO_ELECTRON_TARGETS "" CACHE STRING "Comma-separated electron-builder targets (e.g. --mac,--win)")

  set(AOM_TARGET_CPU "generic" CACHE STRING "Target CPU for libaom" FORCE)
  set(ENABLE_NASM OFF CACHE BOOL "Disable NASM for libaom" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EMSCRIPTEN AND NOT MSVC)
  option(COLOPRESSO_USE_VALGRIND "Use Valgrind if available" OFF)
  option(COLOPRESSO_VALGRIND_RAYON_NUM_THREADS "RAYON_NUM_THREADS for Valgrind tests" "1")
  option(COLOPRESSO_VALGRIND_TRACK_ORIGINS "Enable Valgrind --track-origins (very slow)" OFF)
  set(COLOPRESSO_VALGRIND_LEAK_CHECK "full" CACHE STRING "Valgrind --leak-check (no|summary|full)")
  set(COLOPRESSO_VALGRIND_SHOW_LEAK_KINDS "all" CACHE STRING "Valgrind --show-leak-kinds")
  option(COLOPRESSO_USE_COVERAGE "Use coverage if available" OFF)
  option(COLOPRESSO_USE_ASAN "Use AddressSanitizer" OFF)
  option(COLOPRESSO_USE_MSAN "Use MemorySanitizer" OFF)
  option(COLOPRESSO_USE_UBSAN "Use UndefinedBehaviorSanitizer" OFF)
endif()

option(PNGX_BRIDGE_ENABLE_RUST_ASAN "Enable AddressSanitizer for Rust pngx_bridge build" OFF)
option(PNGX_BRIDGE_ENABLE_RUST_MSAN "Enable MemorySanitizer for Rust pngx_bridge build" ON)
option(PNGX_BRIDGE_ENABLE_RUST_VALGRIND "Enable Valgrind for Rust pngx_bridge build" ON)

# Buildtime
set(_colopresso_timestamp_fields
  YEAR "%Y" 0xFFF 20
  MONTH "%m" 0xF 16
  DAY "%d" 0x1F 11
  HOUR "%H" 0x1F 6
  MINUTE "%M" 0x3F 0
)

set(ENCODED_TIME 0)
while(_colopresso_timestamp_fields)
  list(POP_FRONT _colopresso_timestamp_fields _name _format _mask _shift)
  string(TIMESTAMP _raw "${_format}" UTC)
  math(EXPR _value "${_raw}")
  math(EXPR _encoded "(${_value}) & ${_mask}")
  math(EXPR ENCODED_TIME "${ENCODED_TIME} | (${_encoded} << ${_shift})")
endwhile()

string(TIMESTAMP CMAKE_TIMESTAMP "%s" UTC)
add_definitions(-DCOLOPRESSO_BUILDTIME=${ENCODED_TIME})

message(STATUS "Build timestamp: ${CMAKE_TIMESTAMP}")

# File ops
function(colopresso_sync_file_ops)
  if(COLOPRESSO_DISABLE_FILE_OPS AND COLOPRESSO_WITH_FILE_OPS)
    message(STATUS "COLOPRESSO_DISABLE_FILE_OPS=ON -> forcing COLOPRESSO_WITH_FILE_OPS=OFF")
    set(COLOPRESSO_WITH_FILE_OPS OFF CACHE BOOL "Enable file operation functions" FORCE)
  endif()

  if(COLOPRESSO_WITH_FILE_OPS)
    set(_file_ops_disable OFF)
  else()
    set(_file_ops_disable ON)
  endif()

  set(COLOPRESSO_WITH_FILE_OPS ${COLOPRESSO_WITH_FILE_OPS} PARENT_SCOPE)
  set(COLOPRESSO_DISABLE_FILE_OPS ${_file_ops_disable} PARENT_SCOPE)
endfunction()
