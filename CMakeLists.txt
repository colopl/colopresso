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

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(GNUInstallDirs)
include(FetchContent)

option(COLOPRESSO_USE_TESTS "Build tests" OFF)
option(COLOPRESSO_USE_UTILS "Build utility programs" OFF)
option(COLOPRESSO_USE_CLI "Build unified CLI application" OFF)
option(COLOPRESSO_WITH_FILE_OPS "Enable file operation functions (fopen, fwrite, etc.)" ON)

file(GLOB_RECURSE COLOPRESSO_SOURCES "library/src/*.c")

set(COLOPRESSO_FILE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/library/src/colopresso_file.c")
if(NOT COLOPRESSO_WITH_FILE_OPS)
  list(REMOVE_ITEM COLOPRESSO_SOURCES "${COLOPRESSO_FILE_SOURCE}")
endif()

include(cmake/buildtime.cmake)

if(EMSCRIPTEN)
  set(AOM_TARGET_CPU generic CACHE STRING "Target CPU for libaom when building with Emscripten" FORCE)
  set(ENABLE_NASM 0 CACHE BOOL "Disable NASM for libaom when building with Emscripten" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EMSCRIPTEN AND NOT MSVC)
  include(cmake/debug.cmake)
endif()

include(cmake/threads.cmake)
colopresso_apply_global_threads_compile_options()

include(cmake/dependencies.cmake)

add_library(colopresso STATIC ${COLOPRESSO_SOURCES})

if(COLOPRESSO_ENABLE_COVERAGE)
  target_compile_options(colopresso PRIVATE "--coverage")
  target_link_options(colopresso PRIVATE "--coverage")
endif()

include(cmake/pngx_bridge.cmake)

colopresso_apply_threads_link_options()

set(_colopresso_public_libs
  png_static
  zlibstatic
  webp
  avif_static
  pngx_bridge
)
target_link_libraries(colopresso PUBLIC ${_colopresso_public_libs})

set(_colopresso_platform_libs)
if(APPLE)
  list(APPEND _colopresso_platform_libs "-framework Accelerate")
elseif(UNIX)
  list(APPEND _colopresso_platform_libs m)
elseif(WIN32)
  list(APPEND _colopresso_platform_libs userenv ntdll ws2_32)
endif()
if(_colopresso_platform_libs)
  target_link_libraries(colopresso PUBLIC ${_colopresso_platform_libs})
endif()

if(EMSCRIPTEN)
  include(cmake/emscripten.cmake)
endif()

function(colopresso_sync_file_ops)
  if(COLOPRESSO_DISABLE_FILE_OPS AND COLOPRESSO_WITH_FILE_OPS)
    message(STATUS "COLOPRESSO_DISABLE_FILE_OPS=ON -> forcing COLOPRESSO_WITH_FILE_OPS=OFF")
    set(COLOPRESSO_WITH_FILE_OPS OFF CACHE BOOL "Enable file operation functions (fopen, fwrite, etc.)" FORCE)
  endif()

  if(COLOPRESSO_WITH_FILE_OPS)
    set(_file_ops_disable OFF)
  else()
    set(_file_ops_disable ON)
  endif()

  set(COLOPRESSO_WITH_FILE_OPS ${COLOPRESSO_WITH_FILE_OPS} PARENT_SCOPE)
  set(COLOPRESSO_DISABLE_FILE_OPS ${_file_ops_disable} PARENT_SCOPE)
endfunction()

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

if(COLOPRESSO_USE_UTILS)
  include(cmake/utils.cmake)
endif()

if(COLOPRESSO_USE_CLI)
  add_subdirectory(cli)
endif()

if(COLOPRESSO_USE_TESTS)
  include(cmake/test.cmake)
endif()

install(TARGETS colopresso
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY library/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
  PATTERN "*.in" EXCLUDE
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/colopresso_config.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(COLOPRESSO_WITH_FILE_OPS)
  install(FILES "${COLOPRESSO_GENERATED_FILE_HEADER}"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/colopresso
  )
endif()
