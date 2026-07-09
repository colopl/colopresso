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

# Resolve the Node-API headers.
#
# Prefer the `node-api-headers` package: it is portable, version-independent, and
# ABI-stable across Node.js and Electron, and it is the only reliable source on
# CI runners whose Node.js installation ships no C headers or import library.
# Fall back to the headers bundled with the local Node.js installation for
# developer machines where node-api-headers has not been installed yet.
set(COLOPRESSO_NODE_API_INCLUDE_DIR "")
set(COLOPRESSO_NODE_API_DEF "")
set(COLOPRESSO_JS_NATIVE_API_DEF "")

execute_process(
  COMMAND ${NODE_EXECUTABLE} -e "const h=require('node-api-headers');process.stdout.write([h.include_dir,h.def_paths.node_api_def,h.def_paths.js_native_api_def].join('\\n'));"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE COLOPRESSO_NODE_API_HEADERS_OUTPUT
  RESULT_VARIABLE COLOPRESSO_NODE_API_HEADERS_RESULT
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(COLOPRESSO_NODE_API_HEADERS_RESULT EQUAL 0 AND COLOPRESSO_NODE_API_HEADERS_OUTPUT)
  string(REPLACE "\n" ";" COLOPRESSO_NODE_API_HEADERS_LIST "${COLOPRESSO_NODE_API_HEADERS_OUTPUT}")
  list(GET COLOPRESSO_NODE_API_HEADERS_LIST 0 COLOPRESSO_NODE_API_INCLUDE_DIR)
  list(GET COLOPRESSO_NODE_API_HEADERS_LIST 1 COLOPRESSO_NODE_API_DEF)
  list(GET COLOPRESSO_NODE_API_HEADERS_LIST 2 COLOPRESSO_JS_NATIVE_API_DEF)
endif()

if(COLOPRESSO_NODE_API_INCLUDE_DIR AND EXISTS "${COLOPRESSO_NODE_API_INCLUDE_DIR}/node_api.h")
  set(COLOPRESSO_NODE_INCLUDE_DIR "${COLOPRESSO_NODE_API_INCLUDE_DIR}")
  message(STATUS "Electron native addon: using node-api-headers (${COLOPRESSO_NODE_INCLUDE_DIR})")
else()
  execute_process(
    COMMAND ${NODE_EXECUTABLE} -e "const path = require('node:path'); process.stdout.write(path.join(path.dirname(process.execPath), '..', 'include', 'node'));"
    OUTPUT_VARIABLE COLOPRESSO_NODE_INCLUDE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  message(STATUS "Electron native addon: using Node.js installation headers (${COLOPRESSO_NODE_INCLUDE_DIR})")
endif()

if(NOT EXISTS "${COLOPRESSO_NODE_INCLUDE_DIR}/node_api.h")
  message(FATAL_ERROR
    "node_api.h not found at ${COLOPRESSO_NODE_INCLUDE_DIR}. "
    "Run 'pnpm install' so that the 'node-api-headers' package is available before configuring, "
    "or use a Node.js installation that ships C headers.")
endif()

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
  # The Node.js installation on CI does not ship node.lib, so generate Node-API
  # import libraries from the node-api-headers .def files. The generated imports
  # reference "node.exe"; the delay-load hook below redirects them to the running
  # host executable (e.g. electron.exe) at load time.
  if(NOT COLOPRESSO_NODE_API_DEF OR NOT EXISTS "${COLOPRESSO_NODE_API_DEF}"
      OR NOT COLOPRESSO_JS_NATIVE_API_DEF OR NOT EXISTS "${COLOPRESSO_JS_NATIVE_API_DEF}")
    message(FATAL_ERROR
      "node-api-headers .def files were not found. Run 'pnpm install' before configuring so the "
      "Electron native addon can generate the Node-API import library on Windows.")
  endif()

  get_filename_component(COLOPRESSO_MSVC_BIN_DIR "${CMAKE_C_COMPILER}" DIRECTORY)
  find_program(COLOPRESSO_MSVC_LIB_TOOL NAMES lib HINTS "${COLOPRESSO_MSVC_BIN_DIR}")
  if(NOT COLOPRESSO_MSVC_LIB_TOOL)
    message(FATAL_ERROR "MSVC 'lib' tool was not found next to the compiler; cannot generate the Node-API import library.")
  endif()

  if(CMAKE_GENERATOR_PLATFORM MATCHES "[Aa][Rr][Mm]64")
    set(COLOPRESSO_NODE_API_MACHINE "ARM64")
  else()
    set(COLOPRESSO_NODE_API_MACHINE "X64")
  endif()

  set(COLOPRESSO_NODE_API_LIB "${CMAKE_BINARY_DIR}/colopresso_node_api.lib")
  set(COLOPRESSO_JS_NATIVE_API_LIB "${CMAKE_BINARY_DIR}/colopresso_js_native_api.lib")

  # Generate the import libraries at build time (rather than during configure) so
  # 'lib' runs inside the MSVC build environment where its tool directory is on
  # PATH. The imports reference "node.exe"; the delay-load hook resolves them to
  # the host executable at runtime.
  add_custom_command(
    OUTPUT "${COLOPRESSO_NODE_API_LIB}"
    COMMAND "${COLOPRESSO_MSVC_LIB_TOOL}" "/nologo" "/def:${COLOPRESSO_NODE_API_DEF}" "/out:${COLOPRESSO_NODE_API_LIB}" "/machine:${COLOPRESSO_NODE_API_MACHINE}"
    DEPENDS "${COLOPRESSO_NODE_API_DEF}"
    COMMENT "Generating Node-API import library (node_api, ${COLOPRESSO_NODE_API_MACHINE})"
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${COLOPRESSO_JS_NATIVE_API_LIB}"
    COMMAND "${COLOPRESSO_MSVC_LIB_TOOL}" "/nologo" "/def:${COLOPRESSO_JS_NATIVE_API_DEF}" "/out:${COLOPRESSO_JS_NATIVE_API_LIB}" "/machine:${COLOPRESSO_NODE_API_MACHINE}"
    DEPENDS "${COLOPRESSO_JS_NATIVE_API_DEF}"
    COMMENT "Generating Node-API import library (js_native_api, ${COLOPRESSO_NODE_API_MACHINE})"
    VERBATIM
  )
  add_custom_target(colopresso_native_import_libs
    DEPENDS "${COLOPRESSO_NODE_API_LIB}" "${COLOPRESSO_JS_NATIVE_API_LIB}"
  )
  add_dependencies(colopresso_native colopresso_native_import_libs)

  target_sources(colopresso_native PRIVATE
    "${CMAKE_SOURCE_DIR}/app/electron/native/win_delay_load_hook.cc"
  )
  target_link_libraries(colopresso_native PRIVATE
    "${COLOPRESSO_NODE_API_LIB}"
    "${COLOPRESSO_JS_NATIVE_API_LIB}"
    delayimp
  )
  target_link_options(colopresso_native PRIVATE "/DELAYLOAD:node.exe")
  target_compile_definitions(colopresso_native PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

message(STATUS "Electron native addon output: ${COLOPRESSO_ELECTRON_NATIVE_ADDON_OUTPUT_DIR}")
