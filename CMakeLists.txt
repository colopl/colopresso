# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

cmake_minimum_required(VERSION 3.22)
project(colopresso
  VERSION 0.0.0
  LANGUAGES C
  DESCRIPTION "PNG compressor and converter"
)

# ==============================================================================
# C Standard Configuration
# ==============================================================================

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
include(FetchContent)

# ==============================================================================
# Options and Build Configuration
# ==============================================================================

include(cmake/options.cmake)
include(cmake/compiler.cmake)

# Configure threads (may disable COLOPRESSO_ENABLE_THREADS based on target)
colopresso_configure_threads()
colopresso_apply_global_threads_compile_options()

# Configure debug/sanitizer settings
colopresso_configure_debug()

# Configure SIMD
colopresso_configure_simd()
if(COLOPRESSO_SIMD_TYPE)
  message(STATUS "SIMD optimization enabled: ${COLOPRESSO_SIMD_TYPE}")
elseif(COLOPRESSO_ENABLE_SIMD)
  message(STATUS "SIMD optimization enabled but no supported SIMD detected")
endif()

# ==============================================================================
# Third-Party Dependencies
# ==============================================================================

include(cmake/thirdparty.cmake)

# ==============================================================================
# Source Files
# ==============================================================================

file(GLOB_RECURSE COLOPRESSO_SOURCES "library/src/*.c")

set(COLOPRESSO_FILE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/library/src/colopresso_file.c")
if(NOT COLOPRESSO_WITH_FILE_OPS)
  list(REMOVE_ITEM COLOPRESSO_SOURCES "${COLOPRESSO_FILE_SOURCE}")
endif()

# ==============================================================================
# Main Library Target
# ==============================================================================

add_library(colopresso STATIC ${COLOPRESSO_SOURCES})

# SIMD configuration
if(COLOPRESSO_ENABLE_SIMD)
  target_compile_definitions(colopresso PRIVATE CPRES_USE_SIMD=1)
  if(COLOPRESSO_SIMD_FLAGS)
    target_compile_options(colopresso PRIVATE ${COLOPRESSO_SIMD_FLAGS})
  endif()
endif()

# Coverage configuration
if(COLOPRESSO_ENABLE_COVERAGE)
  target_compile_options(colopresso PRIVATE "--coverage")
  target_link_options(colopresso PRIVATE "--coverage")
endif()

# ==============================================================================
# pngx_bridge (Rust Library)
# ==============================================================================

include(cmake/pngx_bridge.cmake)

# ==============================================================================
# Compiler Version String
# ==============================================================================

colopresso_get_compiler_version_string(COLOPRESSO_COMPILER_VERSION_STRING)
add_definitions(-DCOLOPRESSO_COMPILER_VERSION_STRING="${COLOPRESSO_COMPILER_VERSION_STRING}")

# ==============================================================================
# Emscripten Configuration
# ==============================================================================

if(EMSCRIPTEN)
  include(cmake/emscripten.cmake)
endif()

# ==============================================================================
# Thread Linking
# ==============================================================================

colopresso_apply_threads_link_options(colopresso)

# ==============================================================================
# Link Libraries
# ==============================================================================

target_link_libraries(colopresso PUBLIC
  png_static
  zlibstatic
  webp
  avif_static
  pngx_bridge
)

# Platform-specific libraries
if(APPLE)
  target_link_libraries(colopresso PUBLIC "-framework Accelerate")
elseif(UNIX)
  target_link_libraries(colopresso PUBLIC m)
elseif(WIN32)
  target_link_libraries(colopresso PUBLIC userenv ntdll ws2_32)
endif()

# ==============================================================================
# Header Configuration
# ==============================================================================

colopresso_sync_file_ops()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/library/include/colopresso_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/colopresso_config.h"
  @ONLY
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
file(GLOB_RECURSE COLOPRESSO_PUBLIC_HEADERS
  RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/library/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/library/include/*.h"
)

foreach(_header IN LISTS COLOPRESSO_PUBLIC_HEADERS)
  get_filename_component(_header_dir "${_header}" DIRECTORY)
  if(_header_dir)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/${_header_dir}")
  endif()
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/library/include/${_header}"
    "${CMAKE_CURRENT_BINARY_DIR}/include/${_header}"
    COPYONLY
  )
endforeach()

set(COLOPRESSO_GENERATED_FILE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/colopresso/file.h")
if(COLOPRESSO_WITH_FILE_OPS)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/colopresso")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/library/include/colopresso/file.h.in"
    "${COLOPRESSO_GENERATED_FILE_HEADER}"
    COPYONLY
  )
else()
  file(REMOVE "${COLOPRESSO_GENERATED_FILE_HEADER}")
endif()

target_include_directories(colopresso PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/library/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  ${CMAKE_CURRENT_SOURCE_DIR}/library/src
)

# ==============================================================================
# Optional Components
# ==============================================================================

# Utility programs
if(COLOPRESSO_USE_UTILS)
  if(COLOPRESSO_DISABLE_FILE_OPS)
    message(WARNING "Skipping utils build (COLOPRESSO_DISABLE_FILE_OPS=ON)")
  else()
    file(GLOB UTIL_SOURCES "library/utils/*.c")
    foreach(UTIL_SOURCE ${UTIL_SOURCES})
      get_filename_component(UTIL_NAME ${UTIL_SOURCE} NAME_WE)
      set(UTIL_TARGET "util_${UTIL_NAME}")
      add_executable(${UTIL_TARGET} ${UTIL_SOURCE})
      target_link_libraries(${UTIL_TARGET} PRIVATE colopresso)
      set(_util_properties
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/utils
        OUTPUT_NAME ${UTIL_NAME}
      )
      if(EMSCRIPTEN)
        list(APPEND _util_properties SUFFIX ".js")
      endif()
      set_target_properties(${UTIL_TARGET} PROPERTIES ${_util_properties})
    endforeach()
  endif()
endif()

# CLI application
if(COLOPRESSO_USE_CLI)
  add_subdirectory(cli)
endif()

# Tests
if(COLOPRESSO_USE_TESTS)
  include(cmake/testing.cmake)
endif()

# Python bindings
if(COLOPRESSO_PYTHON_BINDINGS)
  include(cmake/python.cmake)
endif()

# ==============================================================================
# Installation
# ==============================================================================

include(cmake/install.cmake)
