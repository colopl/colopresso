# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

set(PNGX_BRIDGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/pngx_bridge")
set(PNGX_BRIDGE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/pngx_bridge")

if(NOT DEFINED PNGX_BRIDGE_ENABLE_MSAN)
  if(DEFINED COLOPRESSO_USE_MSAN AND COLOPRESSO_USE_MSAN)
    set(PNGX_BRIDGE_ENABLE_MSAN ON)
  else()
    set(PNGX_BRIDGE_ENABLE_MSAN OFF)
  endif()
endif()
if(NOT DEFINED PNGX_BRIDGE_ENABLE_ASAN)
  if(DEFINED COLOPRESSO_USE_ASAN AND COLOPRESSO_USE_ASAN)
    set(PNGX_BRIDGE_ENABLE_ASAN ON)
  else()
    set(PNGX_BRIDGE_ENABLE_ASAN OFF)
  endif()
endif()

set(_pngx_bridge_artifact libpngx_bridge.a)
set(_pngx_bridge_target native)
set(_pngx_bridge_cargo_args)

if(EMSCRIPTEN)
  set(_pngx_bridge_target wasm32-unknown-emscripten)
  set(_pngx_bridge_cargo_args --target ${_pngx_bridge_target})
elseif(WIN32)
  set(_pngx_bridge_artifact pngx_bridge.lib)
endif()

set(PNGX_BRIDGE_RUST_TARGET ${_pngx_bridge_target})
set(PNGX_BRIDGE_CARGO_FLAGS ${_pngx_bridge_cargo_args})

set(_pngx_bridge_rust_profile "release")
set(_pngx_bridge_sanitizer_rustflags "")
set(_pngx_bridge_sanitizer_cargo_flags "")
set(_pngx_bridge_needs_nightly OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EMSCRIPTEN AND NOT WIN32)
  if(PNGX_BRIDGE_ENABLE_MSAN)
    message(STATUS "pngx_bridge: MemorySanitizer detected, enabling for Rust build")
    set(_pngx_bridge_sanitizer_rustflags "-Zsanitizer=memory -Zsanitizer-memory-track-origins")
    set(_pngx_bridge_needs_nightly ON)
    set(_pngx_bridge_rust_profile "release")
  elseif(PNGX_BRIDGE_ENABLE_ASAN)
    message(STATUS "pngx_bridge: AddressSanitizer detected, enabling for Rust build")
    set(_pngx_bridge_sanitizer_rustflags "-Zsanitizer=address")
    set(_pngx_bridge_needs_nightly ON)
    set(_pngx_bridge_rust_profile "release")
  else()
    set(_pngx_bridge_rust_profile "dev")
  endif()

  if(PNGX_BRIDGE_ENABLE_MSAN)
    if(NOT DEFINED PNGX_BRIDGE_HOST_TARGET)
      execute_process(
        COMMAND rustc -vV
        OUTPUT_VARIABLE _rustc_verbose
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      if(_rustc_verbose MATCHES "host: ([^\n]+)")
        set(PNGX_BRIDGE_HOST_TARGET "${CMAKE_MATCH_1}")
      else()
        message(FATAL_ERROR "Failed to determine host target from rustc")
      endif()
    endif()

    set(_pngx_bridge_sanitizer_cargo_flags
      "-Zbuild-std"
      "--target" "${PNGX_BRIDGE_HOST_TARGET}"
    )
    set(_pngx_bridge_target "${PNGX_BRIDGE_HOST_TARGET}")
    message(STATUS "pngx_bridge: Using -Zbuild-std with target ${PNGX_BRIDGE_HOST_TARGET} for MSan")
  endif()
endif()

set(PNGX_BRIDGE_RUST_TARGET ${_pngx_bridge_target})

if(_pngx_bridge_rust_profile STREQUAL "release")
  set(_pngx_bridge_cargo_profile_arg "--release")
elseif(_pngx_bridge_rust_profile STREQUAL "dev")
  set(_pngx_bridge_cargo_profile_arg "")
else()
  set(_pngx_bridge_cargo_profile_arg "--profile=${_pngx_bridge_rust_profile}")
endif()

list(APPEND PNGX_BRIDGE_CARGO_FLAGS ${_pngx_bridge_cargo_profile_arg} ${_pngx_bridge_sanitizer_cargo_flags})

if(_pngx_bridge_rust_profile STREQUAL "dev")
  set(_pngx_bridge_profile_dir "debug")
else()
  set(_pngx_bridge_profile_dir "${_pngx_bridge_rust_profile}")
endif()

if(PNGX_BRIDGE_RUST_TARGET STREQUAL "native")
  set(PNGX_BRIDGE_LIB_PATH "${PNGX_BRIDGE_BUILD_DIR}/target/${_pngx_bridge_profile_dir}/${_pngx_bridge_artifact}")
else()
  set(PNGX_BRIDGE_LIB_PATH "${PNGX_BRIDGE_BUILD_DIR}/target/${PNGX_BRIDGE_RUST_TARGET}/${_pngx_bridge_profile_dir}/${_pngx_bridge_artifact}")
endif()

file(MAKE_DIRECTORY "${PNGX_BRIDGE_BUILD_DIR}/generated")

set(_pngx_bridge_copy_inputs
  "${PNGX_BRIDGE_SOURCE_DIR}/Cargo.toml"
  "${PNGX_BRIDGE_SOURCE_DIR}/cbindgen.toml"
  "${PNGX_BRIDGE_SOURCE_DIR}/src/lib.rs"
)

add_custom_command(
  OUTPUT "${PNGX_BRIDGE_BUILD_DIR}/Cargo.toml"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PNGX_BRIDGE_SOURCE_DIR}"
    "${PNGX_BRIDGE_BUILD_DIR}"
  DEPENDS ${_pngx_bridge_copy_inputs}
  COMMENT "Copying pngx_bridge source to build directory"
  VERBATIM
)

set(PNGX_BRIDGE_RUSTFLAGS "-A mismatched_lifetime_syntaxes")
if(_pngx_bridge_sanitizer_rustflags)
  string(APPEND PNGX_BRIDGE_RUSTFLAGS " ${_pngx_bridge_sanitizer_rustflags}")
endif()

if(PNGX_BRIDGE_ENABLE_MSAN)
  string(APPEND PNGX_BRIDGE_RUSTFLAGS " --cfg pngx_bridge_msan")
endif()

set(PNGX_BRIDGE_SANITIZER_CFLAGS "")
if(PNGX_BRIDGE_ENABLE_MSAN)
  set(PNGX_BRIDGE_SANITIZER_CFLAGS "-fsanitize=memory -fno-omit-frame-pointer -g")
elseif(PNGX_BRIDGE_ENABLE_ASAN)
  set(PNGX_BRIDGE_SANITIZER_CFLAGS "-fsanitize=address -fno-omit-frame-pointer -g")
endif()

set(_pngx_bridge_cargo_env "RUSTFLAGS=${PNGX_BRIDGE_RUSTFLAGS}")
if(PNGX_BRIDGE_SANITIZER_CFLAGS)
  list(APPEND _pngx_bridge_cargo_env
    "CFLAGS=${PNGX_BRIDGE_SANITIZER_CFLAGS}"
    "CXXFLAGS=${PNGX_BRIDGE_SANITIZER_CFLAGS}")
endif()

if(PNGX_BRIDGE_ENABLE_MSAN)
  set(_pngx_bridge_cc "${CMAKE_C_COMPILER}")
  if(NOT _pngx_bridge_cc)
    set(_pngx_bridge_cc clang)
  endif()

  set(_pngx_bridge_cxx "${CMAKE_CXX_COMPILER}")
  if(NOT _pngx_bridge_cxx)
    set(_pngx_bridge_cxx clang++)
  endif()

  list(APPEND _pngx_bridge_cargo_env
    "CC=${_pngx_bridge_cc}"
    "CXX=${_pngx_bridge_cxx}"
  )

  string(REPLACE "-" "_" _pngx_bridge_target_env "${PNGX_BRIDGE_RUST_TARGET}")
  list(APPEND _pngx_bridge_cargo_env
    "CC_${_pngx_bridge_target_env}=${_pngx_bridge_cc}"
    "CXX_${_pngx_bridge_target_env}=${_pngx_bridge_cxx}"
    "CC_${PNGX_BRIDGE_RUST_TARGET}=${_pngx_bridge_cc}"
    "CXX_${PNGX_BRIDGE_RUST_TARGET}=${_pngx_bridge_cxx}"
  )
endif()

set(_pngx_bridge_cargo_command "cargo")
if(_pngx_bridge_needs_nightly)
  set(_pngx_bridge_cargo_command "cargo" "+nightly")
  message(STATUS "pngx_bridge: Using nightly toolchain for sanitizer support")
endif()

list(JOIN PNGX_BRIDGE_CARGO_FLAGS " " _pngx_bridge_cargo_flags_str)

add_custom_command(
  OUTPUT "${PNGX_BRIDGE_LIB_PATH}" "${PNGX_BRIDGE_BUILD_DIR}/generated/pngx_bridge.h"
  COMMAND ${CMAKE_COMMAND} -E echo "Building pngx_bridge (target: ${PNGX_BRIDGE_RUST_TARGET}, profile: ${_pngx_bridge_rust_profile})"
  COMMAND ${CMAKE_COMMAND} -E env ${_pngx_bridge_cargo_env}
    ${_pngx_bridge_cargo_command} build --manifest-path "${PNGX_BRIDGE_BUILD_DIR}/Cargo.toml" ${PNGX_BRIDGE_CARGO_FLAGS}
  BYPRODUCTS "${PNGX_BRIDGE_BUILD_DIR}/generated/pngx_bridge.h"
  WORKING_DIRECTORY "${PNGX_BRIDGE_BUILD_DIR}"
  DEPENDS
    "${PNGX_BRIDGE_BUILD_DIR}/Cargo.toml"
  COMMENT "Building pngx_bridge (Rust target: ${PNGX_BRIDGE_RUST_TARGET}, flags: ${_pngx_bridge_cargo_flags_str})"
  VERBATIM
)

add_custom_target(pngx_bridge_build DEPENDS "${PNGX_BRIDGE_LIB_PATH}" "${PNGX_BRIDGE_BUILD_DIR}/generated/pngx_bridge.h")

add_library(pngx_bridge STATIC IMPORTED GLOBAL)
set_target_properties(pngx_bridge PROPERTIES
  IMPORTED_LOCATION "${PNGX_BRIDGE_LIB_PATH}"
  INTERFACE_INCLUDE_DIRECTORIES "${PNGX_BRIDGE_BUILD_DIR}/generated"
)

add_dependencies(pngx_bridge pngx_bridge_build)

message(STATUS "pngx_bridge: library path: ${PNGX_BRIDGE_LIB_PATH}")
message(STATUS "pngx_bridge: RUSTFLAGS: ${PNGX_BRIDGE_RUSTFLAGS}")
if(_pngx_bridge_needs_nightly)
  message(STATUS "pngx_bridge: Sanitizer build enabled")
endif()
