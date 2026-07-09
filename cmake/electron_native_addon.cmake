# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

find_program(NODE_EXECUTABLE NAMES node node.exe)
if(NOT NODE_EXECUTABLE)
  message(FATAL_ERROR "node executable not found. Node.js is required to build the Electron native addon.")
endif()

execute_process(
  COMMAND ${NODE_EXECUTABLE} -e "const path = require('node:path'); process.stdout.write(path.join(path.dirname(process.execPath), '..', 'include', 'node'));"
  OUTPUT_VARIABLE COLOPRESSO_NODE_INCLUDE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT EXISTS "${COLOPRESSO_NODE_INCLUDE_DIR}/node_api.h")
  message(FATAL_ERROR "node_api.h not found at ${COLOPRESSO_NODE_INCLUDE_DIR}. Cannot build Electron native addon.")
endif()

get_filename_component(COLOPRESSO_NODE_ROOT "${COLOPRESSO_NODE_INCLUDE_DIR}/../.." ABSOLUTE)

if(NOT COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR)
  set(COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR "${CMAKE_BINARY_DIR}/electron/native")
endif()

add_library(colopresso_native MODULE
  "${CMAKE_SOURCE_DIR}/app/electron/native/colopresso_native.c"
)

target_include_directories(colopresso_native PRIVATE
  "${COLOPRESSO_NODE_INCLUDE_DIR}"
)

target_link_libraries(colopresso_native PRIVATE colopresso)

target_compile_definitions(colopresso_native PRIVATE
  NAPI_VERSION=9
)

set_target_properties(colopresso_native PROPERTIES
  PREFIX ""
  SUFFIX ".node"
  LIBRARY_OUTPUT_DIRECTORY "${COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR}"
  ARCHIVE_OUTPUT_DIRECTORY "${COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR}"
)

foreach(_config Debug Release RelWithDebInfo MinSizeRel)
  string(TOUPPER "${_config}" _config_upper)
  set_target_properties(colopresso_native PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_${_config_upper} "${COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_${_config_upper} "${COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY_${_config_upper} "${COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR}"
  )
endforeach()

if(APPLE)
  target_link_options(colopresso_native PRIVATE "-undefined" "dynamic_lookup")
elseif(WIN32)
  find_library(COLOPRESSO_NODE_LIBRARY
    NAMES node
    PATHS "${COLOPRESSO_NODE_ROOT}" "${COLOPRESSO_NODE_ROOT}/lib"
    NO_DEFAULT_PATH
  )
  if(NOT COLOPRESSO_NODE_LIBRARY)
    message(FATAL_ERROR "node.lib not found under ${COLOPRESSO_NODE_ROOT}. Cannot build Electron native addon on Windows.")
  endif()
  target_link_libraries(colopresso_native PRIVATE "${COLOPRESSO_NODE_LIBRARY}")
  target_compile_definitions(colopresso_native PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

message(STATUS "Electron native addon output: ${COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR}")
