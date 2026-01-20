# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

cmake_minimum_required(VERSION 3.22)
project(colopresso_python LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
include(FetchContent)

set(COLOPRESSO_PYTHON_BINDINGS ON CACHE BOOL "Build Python bindings" FORCE)
set(COLOPRESSO_USE_CLI OFF CACHE BOOL "Build CLI" FORCE)
set(COLOPRESSO_USE_TESTS OFF CACHE BOOL "Build tests" FORCE)
set(COLOPRESSO_USE_UTILS OFF CACHE BOOL "Build utils" FORCE)

get_filename_component(COLOPRESSO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

set(CMAKE_MODULE_PATH "${COLOPRESSO_ROOT}/cmake" ${CMAKE_MODULE_PATH})

include(options)
include(compiler)

colopresso_configure_threads()
colopresso_apply_global_threads_compile_options()
colopresso_configure_debug()
colopresso_configure_simd()

# zlib configuration
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# libpng configuration
set(PNG_SHARED OFF CACHE BOOL "" FORCE)
set(PNG_STATIC ON CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE)
set(PNG_TOOLS OFF CACHE BOOL "" FORCE)
set(PNG_DEBUG OFF CACHE BOOL "" FORCE)
set(PNG_HARDWARE_OPTIMIZATIONS ON CACHE BOOL "" FORCE)

# libwebp configuration
set(WEBP_BUILD_ANIM_UTILS OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_CWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_DWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_GIF2WEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_IMG2WEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_VWEBP OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPINFO OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_LIBWEBPMUX ON CACHE BOOL "" FORCE)
set(WEBP_BUILD_WEBPMUX OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(WEBP_BUILD_FUZZTEST OFF CACHE BOOL "" FORCE)

# libavif configuration
set(AVIF_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
set(AVIF_BUILD_APPS OFF CACHE BOOL "" FORCE)
set(AVIF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(AVIF_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(AVIF_LIBYUV OFF CACHE STRING "" FORCE)
set(AVIF_CODEC_AOM "LOCAL" CACHE STRING "" FORCE)
set(AVIF_CODEC_AOM_ENCODE ON CACHE BOOL "" FORCE)
set(AVIF_CODEC_AOM_DECODE OFF CACHE STRING "" FORCE)
set(AVIF_CODEC_DAV1D OFF CACHE STRING "" FORCE)
set(AVIF_CODEC_LIBGAV1 OFF CACHE STRING "" FORCE)
set(AVIF_CODEC_RAV1E OFF CACHE STRING "" FORCE)
set(AVIF_CODEC_SVT OFF CACHE STRING "" FORCE)
set(AVIF_CODEC_AVM OFF CACHE STRING "" FORCE)

# Force static library for libavif (avoid runtime dependency)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(COLOPRESSO_BACKUP_BUILD_TESTING ${BUILD_TESTING})
set(BUILD_TESTING OFF)

# zlib
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)
add_subdirectory("${COLOPRESSO_ROOT}/third_party/zlib" "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib" EXCLUDE_FROM_ALL)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS OFF CACHE BOOL "" FORCE)
if(TARGET zlib)
  get_target_property(ZLIB_TYPE zlib TYPE)
  if(ZLIB_TYPE STREQUAL "SHARED_LIBRARY" OR ZLIB_TYPE STREQUAL "STATIC_LIBRARY")
    set_target_properties(zlib PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
endif()

# Set ZLIB_ROOT for libpng
set(ZLIB_ROOT "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib" CACHE PATH "" FORCE)
set(ZLIB_INCLUDE_DIR "${COLOPRESSO_ROOT}/third_party/zlib;${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib" CACHE PATH "" FORCE)
set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib/libz.a" CACHE FILEPATH "" FORCE)

# libpng
add_subdirectory("${COLOPRESSO_ROOT}/third_party/libpng" "${CMAKE_CURRENT_BINARY_DIR}/third_party/libpng" EXCLUDE_FROM_ALL)

# libwebp
add_subdirectory("${COLOPRESSO_ROOT}/third_party/libwebp" "${CMAKE_CURRENT_BINARY_DIR}/third_party/libwebp" EXCLUDE_FROM_ALL)

# libavif
add_subdirectory("${COLOPRESSO_ROOT}/third_party/libavif" "${CMAKE_CURRENT_BINARY_DIR}/third_party/libavif" EXCLUDE_FROM_ALL)

set(BUILD_TESTING ${COLOPRESSO_BACKUP_BUILD_TESTING})

# pngx_bridge (Rust)
set(PNGX_BRIDGE_SOURCE_DIR "${COLOPRESSO_ROOT}/library/pngx_bridge")
set(PNGX_BRIDGE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/pngx_bridge")
set(PNGX_BRIDGE_USE_WASM_SEPARATION OFF)

set(_pngx_bridge_artifact libpngx_bridge.a)
set(_pngx_bridge_cargo_args)
set(PNGX_BRIDGE_ENABLE_RAYON ON)
message(STATUS "pngx_bridge: Rayon multithreading enabled (native build)")

set(_pngx_bridge_lib_path "${PNGX_BRIDGE_BUILD_DIR}/target/release/${_pngx_bridge_artifact}")
message(STATUS "pngx_bridge: library path: ${_pngx_bridge_lib_path}")

find_program(CARGO cargo REQUIRED)

set(_pngx_bridge_rustflags "-A mismatched_lifetime_syntaxes")
message(STATUS "pngx_bridge: RUSTFLAGS: ${_pngx_bridge_rustflags}")

if(PNGX_BRIDGE_ENABLE_RAYON)
  list(APPEND _pngx_bridge_cargo_args --features rayon)
endif()

add_custom_command(
  OUTPUT "${_pngx_bridge_lib_path}"
  COMMAND ${CMAKE_COMMAND} -E env
    "RUSTFLAGS=${_pngx_bridge_rustflags}"
    ${CARGO} build --release ${_pngx_bridge_cargo_args} --target-dir "${PNGX_BRIDGE_BUILD_DIR}/target"
  WORKING_DIRECTORY "${PNGX_BRIDGE_SOURCE_DIR}"
  COMMENT "Building pngx_bridge Rust library"
  VERBATIM
)

file(MAKE_DIRECTORY "${PNGX_BRIDGE_BUILD_DIR}/generated")
set(PNGX_BRIDGE_OXIPNG_VERSION 0)
set(PNGX_BRIDGE_IMAGEQUANT_VERSION 0)
set(PNGX_BRIDGE_RUST_VERSION "unknown")
configure_file(
  "${COLOPRESSO_ROOT}/library/include/colopresso/pngx_bridge.h.in"
  "${PNGX_BRIDGE_BUILD_DIR}/generated/pngx_bridge.h"
  @ONLY
)

add_custom_target(pngx_bridge_build DEPENDS "${_pngx_bridge_lib_path}")

add_library(pngx_bridge STATIC IMPORTED GLOBAL)
set_target_properties(pngx_bridge PROPERTIES
  IMPORTED_LOCATION "${_pngx_bridge_lib_path}"
)
add_dependencies(pngx_bridge pngx_bridge_build)
target_include_directories(pngx_bridge INTERFACE "${PNGX_BRIDGE_BUILD_DIR}/generated")

# sources
file(GLOB_RECURSE COLOPRESSO_SOURCES "${COLOPRESSO_ROOT}/library/src/*.c")

set(COLOPRESSO_FILE_SOURCE "${COLOPRESSO_ROOT}/library/src/colopresso_file.c")
if(NOT COLOPRESSO_WITH_FILE_OPS)
  list(REMOVE_ITEM COLOPRESSO_SOURCES "${COLOPRESSO_FILE_SOURCE}")
endif()

# library
add_library(colopresso STATIC ${COLOPRESSO_SOURCES})

if(COLOPRESSO_ENABLE_SIMD)
  target_compile_definitions(colopresso PRIVATE CPRES_USE_SIMD=1)
  if(COLOPRESSO_SIMD_FLAGS)
    target_compile_options(colopresso PRIVATE ${COLOPRESSO_SIMD_FLAGS})
  endif()
endif()

colopresso_get_compiler_version_string(COLOPRESSO_COMPILER_VERSION_STRING)
add_definitions(-DCOLOPRESSO_COMPILER_VERSION_STRING="${COLOPRESSO_COMPILER_VERSION_STRING}")
colopresso_apply_threads_link_options(colopresso)
target_link_libraries(colopresso PUBLIC
  png_static
  zlibstatic
  webp
  avif_static
  pngx_bridge
)

if(UNIX AND NOT APPLE)
  target_link_libraries(colopresso PUBLIC m)
endif()

target_include_directories(colopresso PRIVATE
  "${COLOPRESSO_ROOT}/third_party/libavif/include"
  "${COLOPRESSO_ROOT}/third_party/libwebp/src"
  "${CMAKE_CURRENT_BINARY_DIR}/third_party/libpng"
  "${COLOPRESSO_ROOT}/third_party/libpng"
  "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib"
  "${COLOPRESSO_ROOT}/third_party/zlib"
)

colopresso_sync_file_ops()

configure_file(
  "${COLOPRESSO_ROOT}/library/include/colopresso_config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/colopresso_config.h"
  @ONLY
)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include")
file(GLOB_RECURSE COLOPRESSO_PUBLIC_HEADERS
  RELATIVE "${COLOPRESSO_ROOT}/library/include"
  "${COLOPRESSO_ROOT}/library/include/*.h"
)

foreach(_header IN LISTS COLOPRESSO_PUBLIC_HEADERS)
  get_filename_component(_header_dir "${_header}" DIRECTORY)
  if(_header_dir)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/${_header_dir}")
  endif()
  configure_file(
    "${COLOPRESSO_ROOT}/library/include/${_header}"
    "${CMAKE_CURRENT_BINARY_DIR}/include/${_header}"
    COPYONLY
  )
endforeach()

set(COLOPRESSO_GENERATED_FILE_HEADER "${CMAKE_CURRENT_BINARY_DIR}/include/colopresso/file.h")
if(COLOPRESSO_WITH_FILE_OPS)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/include/colopresso")
  configure_file(
    "${COLOPRESSO_ROOT}/library/include/colopresso/file.h.in"
    "${COLOPRESSO_GENERATED_FILE_HEADER}"
    COPYONLY
  )
else()
  file(REMOVE "${COLOPRESSO_GENERATED_FILE_HEADER}")
endif()

target_include_directories(colopresso PUBLIC
  $<BUILD_INTERFACE:${COLOPRESSO_ROOT}/library/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  ${COLOPRESSO_ROOT}/library/src
)

# Python bindings (stable ABI)
message(STATUS "Building Python bindings with stable ABI (Limited API for Python 3.10+)")

find_package(Python3 REQUIRED COMPONENTS Development.Module)

# Python 3.10+ stable ABI (Py_LIMITED_API)
set(COLOPRESSO_PYTHON_ABI_VERSION 0x030a0000)

add_library(colopresso_python MODULE
  "${CMAKE_CURRENT_SOURCE_DIR}/colopresso/_colopresso_ext.c"
)

target_compile_definitions(colopresso_python PRIVATE
  Py_LIMITED_API=${COLOPRESSO_PYTHON_ABI_VERSION}
)

target_link_libraries(colopresso_python PRIVATE
  Python3::Module
)

if(UNIX AND NOT APPLE)
  target_link_libraries(colopresso_python PRIVATE
    -Wl,--whole-archive
    colopresso
    -Wl,--no-whole-archive
    m
  )
elseif(APPLE)
  target_link_libraries(colopresso_python PRIVATE
    -Wl,-force_load,$<TARGET_FILE:colopresso>
  )
else()
  target_link_libraries(colopresso_python PRIVATE colopresso)
endif()

target_include_directories(colopresso_python PRIVATE
  ${COLOPRESSO_ROOT}/library/include
  ${CMAKE_CURRENT_BINARY_DIR}/include
)

set_target_properties(colopresso_python PROPERTIES
  OUTPUT_NAME "_colopresso"
  PREFIX ""
)

if(WIN32)
  set_target_properties(colopresso_python PROPERTIES SUFFIX ".pyd")
elseif(APPLE)
  set_target_properties(colopresso_python PROPERTIES SUFFIX ".abi3.so")
else()
  set_target_properties(colopresso_python PROPERTIES SUFFIX ".abi3.so")
endif()

install(
  TARGETS colopresso_python
  LIBRARY DESTINATION colopresso
  COMPONENT python
)
