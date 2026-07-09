# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

if(NOT APPLE AND NOT WIN32)
  message(FATAL_ERROR "Native Electron packaging is supported only on macOS and Windows.")
endif()

if(NOT TARGET colopresso_native)
  message(FATAL_ERROR "COLOPRESSO_ELECTRON_APP requires the colopresso_native target.")
endif()

find_program(PNPM_EXECUTABLE pnpm)
if(NOT PNPM_EXECUTABLE)
  message(FATAL_ERROR "pnpm executable not found. Please install pnpm before building the Electron app.")
endif()

find_program(NODE_EXECUTABLE NAMES node node.exe)
if(NOT NODE_EXECUTABLE)
  message(FATAL_ERROR "node executable not found. Please install Node.js before building the Electron app.")
endif()

function(_colopresso_detect_electron_arch OUT_VAR)
  set(_arch_source "${CMAKE_SYSTEM_PROCESSOR}")

  if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    list(LENGTH CMAKE_OSX_ARCHITECTURES _osx_arch_count)
    if(_osx_arch_count EQUAL 1)
      list(GET CMAKE_OSX_ARCHITECTURES 0 _arch_source)
    endif()
  endif()

  string(TOLOWER "${_arch_source}" _arch_lower)
  if(_arch_lower MATCHES "^(arm64|aarch64)$")
    set(${OUT_VAR} "arm64" PARENT_SCOPE)
  elseif(_arch_lower MATCHES "^(x86_64|amd64|x64)$")
    set(${OUT_VAR} "x64" PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Unsupported Electron architecture: ${_arch_source}. Set COLOPRESSO_ELECTRON_ARCH to x64 or arm64.")
  endif()
endfunction()

if(NOT COLOPRESSO_ELECTRON_ARCH)
  _colopresso_detect_electron_arch(COLOPRESSO_ELECTRON_ARCH)
endif()

if(NOT COLOPRESSO_ELECTRON_ARCH MATCHES "^(x64|arm64)$")
  message(FATAL_ERROR "COLOPRESSO_ELECTRON_ARCH must be x64 or arm64.")
endif()

set(COLOPRESSO_ELECTRON_OUT_DIR "${CMAKE_BINARY_DIR}/electron")
file(RELATIVE_PATH COLOPRESSO_ELECTRON_OUT_DIR_REL "${CMAKE_SOURCE_DIR}" "${COLOPRESSO_ELECTRON_OUT_DIR}")

set(VITE_BASE_ENV
  "NODE_ENV=production"
  "ROLLUP_FORCE_JS=1"
)
set(VITE_CLI "${CMAKE_SOURCE_DIR}/node_modules/vite/bin/vite.js")
set(COLOPRESSO_ELECTRON_NODE_MODULES_STAMP "${CMAKE_BINARY_DIR}/.electron_node_modules.stamp")
set(COLOPRESSO_ELECTRON_RESOURCES_STAMP "${CMAKE_BINARY_DIR}/.electron_resources.stamp")
set(COLOPRESSO_ELECTRON_NATIVE_STAGE_STAMP "${CMAKE_BINARY_DIR}/.electron_native_stage.stamp")
set(COLOPRESSO_ELECTRON_PACKAGE_FILES
  "${CMAKE_SOURCE_DIR}/package.json"
  "${CMAKE_SOURCE_DIR}/pnpm-lock.yaml"
  "${CMAKE_SOURCE_DIR}/pnpm-workspace.yaml"
)
set(COLOPRESSO_ELECTRON_CONFIG_FILES
  "${CMAKE_SOURCE_DIR}/app/shared/vite.config.ts"
  "${CMAKE_SOURCE_DIR}/app/electron/vite.config.ts"
  "${CMAKE_SOURCE_DIR}/app/electron/vite.preload.config.ts"
)

file(GLOB_RECURSE COLOPRESSO_ELECTRON_SOURCE_FILES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/app/shared/*.ts"
  "${CMAKE_SOURCE_DIR}/app/shared/*.tsx"
  "${CMAKE_SOURCE_DIR}/app/shared/*.js"
  "${CMAKE_SOURCE_DIR}/app/shared/*.jsx"
  "${CMAKE_SOURCE_DIR}/app/shared/*.json"
  "${CMAKE_SOURCE_DIR}/app/shared/*.css"
  "${CMAKE_SOURCE_DIR}/app/shared/*.scss"
  "${CMAKE_SOURCE_DIR}/app/shared/*.html"
  "${CMAKE_SOURCE_DIR}/app/electron/*.ts"
  "${CMAKE_SOURCE_DIR}/app/electron/*.tsx"
  "${CMAKE_SOURCE_DIR}/app/electron/*.js"
  "${CMAKE_SOURCE_DIR}/app/electron/*.jsx"
  "${CMAKE_SOURCE_DIR}/app/electron/*.json"
  "${CMAKE_SOURCE_DIR}/app/electron/*.css"
  "${CMAKE_SOURCE_DIR}/app/electron/*.scss"
  "${CMAKE_SOURCE_DIR}/app/electron/*.html"
)

function(_colopresso_add_electron_vite_build OUT_VAR)
  set(options)
  set(oneValueArgs CONFIG)
  set(multiValueArgs ENV)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_CONFIG)
    message(FATAL_ERROR "CONFIG argument required for _colopresso_add_electron_vite_build")
  endif()

  set(_env ${VITE_BASE_ENV})
  if(ARG_ENV)
    list(APPEND _env ${ARG_ENV})
  endif()

  set(_commands "${${OUT_VAR}}")
  list(APPEND _commands
    COMMAND ${CMAKE_COMMAND} -E env ${_env}
      ${NODE_EXECUTABLE} "${VITE_CLI}" build --config "${ARG_CONFIG}"
  )
  set(${OUT_VAR} "${_commands}" PARENT_SCOPE)
endfunction()

set(_electron_env
  "COLOPRESSO_BUILD_TARGET=electron"
  "COLOPRESSO_DEST_DIR=${COLOPRESSO_ELECTRON_OUT_DIR}"
  "COLOPRESSO_RESOURCES_DIR=${COLOPRESSO_ELECTRON_OUT_DIR}"
  "COLOPRESSO_SHARED_DIST_DIR=${COLOPRESSO_ELECTRON_OUT_DIR}"
)
set(_electron_app_env ${_electron_env})
list(APPEND _electron_app_env "COLOPRESSO_ELECTRON_OUT_DIR=${COLOPRESSO_ELECTRON_OUT_DIR}")

set(_electron_vite_commands)
_colopresso_add_electron_vite_build(_electron_vite_commands
  CONFIG "${CMAKE_SOURCE_DIR}/app/shared/vite.config.ts"
  ENV ${_electron_env}
)
_colopresso_add_electron_vite_build(_electron_vite_commands
  CONFIG "${CMAKE_SOURCE_DIR}/app/electron/vite.config.ts"
  ENV ${_electron_app_env}
)
_colopresso_add_electron_vite_build(_electron_vite_commands
  CONFIG "${CMAKE_SOURCE_DIR}/app/electron/vite.preload.config.ts"
  ENV ${_electron_app_env}
)

add_custom_command(
  OUTPUT ${COLOPRESSO_ELECTRON_NODE_MODULES_STAMP}
  DEPENDS ${COLOPRESSO_ELECTRON_PACKAGE_FILES}
  COMMAND ${CMAKE_COMMAND} -E remove "${COLOPRESSO_ELECTRON_NODE_MODULES_STAMP}"
  # Invoke pnpm through "cmake -E env" so that on Windows the MSBuild custom-build
  # batch script keeps running after pnpm: pnpm resolves to pnpm.cmd, and calling
  # a .cmd directly (without CALL) transfers control and aborts the remaining
  # commands (the vite resource builds and the stamp touch), leaving main.js
  # unbuilt. Launching via cmake.exe returns control to the script.
  COMMAND ${CMAKE_COMMAND} -E env ${PNPM_EXECUTABLE} install --frozen-lockfile
  COMMAND ${CMAKE_COMMAND} -E touch "${COLOPRESSO_ELECTRON_NODE_MODULES_STAMP}"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Installing Electron dependencies"
  VERBATIM
  COMMAND_EXPAND_LISTS
)

add_custom_command(
  OUTPUT ${COLOPRESSO_ELECTRON_RESOURCES_STAMP}
  DEPENDS
    ${COLOPRESSO_ELECTRON_NODE_MODULES_STAMP}
    ${COLOPRESSO_ELECTRON_PACKAGE_FILES}
    ${COLOPRESSO_ELECTRON_CONFIG_FILES}
    ${COLOPRESSO_ELECTRON_SOURCE_FILES}
  COMMAND ${CMAKE_COMMAND} -E remove "${COLOPRESSO_ELECTRON_RESOURCES_STAMP}"
  ${_electron_vite_commands}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/app/electron/index.html"
    "${COLOPRESSO_ELECTRON_OUT_DIR}/index.html"
  COMMAND ${CMAKE_COMMAND} -E touch "${COLOPRESSO_ELECTRON_RESOURCES_STAMP}"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Building Electron resources"
  VERBATIM
  COMMAND_EXPAND_LISTS
)

add_custom_target(colopresso_electron_resources
  DEPENDS ${COLOPRESSO_ELECTRON_RESOURCES_STAMP}
)

add_custom_command(
  OUTPUT ${COLOPRESSO_ELECTRON_NATIVE_STAGE_STAMP}
  DEPENDS colopresso_electron_resources colopresso_native "$<TARGET_FILE:colopresso_native>"
  COMMAND ${CMAKE_COMMAND} -E remove "${COLOPRESSO_ELECTRON_NATIVE_STAGE_STAMP}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${COLOPRESSO_ELECTRON_OUT_DIR}/native"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:colopresso_native>"
    "${COLOPRESSO_ELECTRON_OUT_DIR}/native/colopresso_native.node"
  COMMAND ${CMAKE_COMMAND} -E touch "${COLOPRESSO_ELECTRON_NATIVE_STAGE_STAMP}"
  COMMENT "Staging Electron native addon (${COLOPRESSO_ELECTRON_ARCH})"
  VERBATIM
)

add_custom_target(colopresso_electron
  DEPENDS ${COLOPRESSO_ELECTRON_NATIVE_STAGE_STAMP}
)

set(ELECTRON_BUILDER_BASE_ARGS exec electron-builder --publish never)
list(APPEND ELECTRON_BUILDER_BASE_ARGS
  "-c.extraMetadata.main=${COLOPRESSO_ELECTRON_OUT_DIR_REL}/main.js"
  "-c.files=${COLOPRESSO_ELECTRON_OUT_DIR_REL}/**/*"
  "-c.asarUnpack=${COLOPRESSO_ELECTRON_OUT_DIR_REL}/**/*.node"
)

if(COLOPRESSO_ELECTRON_TARGETS)
  set(_electron_targets "${COLOPRESSO_ELECTRON_TARGETS}")
else()
  if(APPLE)
    set(_electron_targets "--mac")
  else()
    set(_electron_targets "--win")
  endif()
endif()

set(ELECTRON_BUILDER_COMMANDS)
string(REPLACE "," ";" _electron_target_list "${_electron_targets}")
foreach(_electron_target IN LISTS _electron_target_list)
  string(STRIP "${_electron_target}" _electron_target)
  if(NOT _electron_target)
    continue()
  endif()

  if(_electron_target STREQUAL "--mac")
    foreach(_electron_builder_mac_target dmg zip)
      list(APPEND ELECTRON_BUILDER_COMMANDS
        COMMAND ${PNPM_EXECUTABLE} ${ELECTRON_BUILDER_BASE_ARGS} --mac "${_electron_builder_mac_target}" "--${COLOPRESSO_ELECTRON_ARCH}"
      )
    endforeach()
  elseif(_electron_target STREQUAL "--win")
    list(APPEND ELECTRON_BUILDER_COMMANDS
      COMMAND ${PNPM_EXECUTABLE} ${ELECTRON_BUILDER_BASE_ARGS} --win "--${COLOPRESSO_ELECTRON_ARCH}"
    )
  else()
    list(APPEND ELECTRON_BUILDER_COMMANDS
      COMMAND ${PNPM_EXECUTABLE} ${ELECTRON_BUILDER_BASE_ARGS} "${_electron_target}" "--${COLOPRESSO_ELECTRON_ARCH}"
    )
  endif()
endforeach()

add_custom_target(colopresso_electron_package ALL
  ${ELECTRON_BUILDER_COMMANDS}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Packaging Electron application (${COLOPRESSO_ELECTRON_ARCH})"
  VERBATIM
)
add_dependencies(colopresso_electron_package colopresso_electron)

message(STATUS "Electron output: ${COLOPRESSO_ELECTRON_OUT_DIR}")
message(STATUS "Electron package architecture: ${COLOPRESSO_ELECTRON_ARCH}")
