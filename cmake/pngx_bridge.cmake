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

set(_pngx_bridge_artifact libpngx_bridge.a)
set(_pngx_bridge_target native)
set(_pngx_bridge_cargo_args)

if(EMSCRIPTEN)
  set(_pngx_bridge_target wasm32-unknown-emscripten)
  set(_pngx_bridge_cargo_args --target ${_pngx_bridge_target})
elseif(WIN32)
  set(_pngx_bridge_artifact pngx_bridge.lib)
elseif(APPLE AND CMAKE_APPLE_SILICON_PROCESSOR STREQUAL "x86_64")
  # When cross-compiling for x86_64 on Apple Silicon using Rosetta 2
  set(_pngx_bridge_target x86_64-apple-darwin)
  set(_pngx_bridge_cargo_args --target ${_pngx_bridge_target})
endif()

set(PNGX_BRIDGE_RUST_TARGET ${_pngx_bridge_target})
set(PNGX_BRIDGE_CARGO_FLAGS ${_pngx_bridge_cargo_args})
set(PNGX_BRIDGE_LIB_PATH "${PNGX_BRIDGE_BUILD_DIR}/target/${_pngx_bridge_target}/release/${_pngx_bridge_artifact}")
if(PNGX_BRIDGE_RUST_TARGET STREQUAL "native")
  set(PNGX_BRIDGE_LIB_PATH "${PNGX_BRIDGE_BUILD_DIR}/target/release/${_pngx_bridge_artifact}")
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

add_custom_command(
  OUTPUT "${PNGX_BRIDGE_LIB_PATH}" "${PNGX_BRIDGE_BUILD_DIR}/generated/pngx_bridge.h"
  COMMAND ${CMAKE_COMMAND} -E echo "Building pngx_bridge (target: ${PNGX_BRIDGE_RUST_TARGET})"
  COMMAND ${CMAKE_COMMAND} -E env "RUSTFLAGS=${PNGX_BRIDGE_RUSTFLAGS}"
    cargo build --manifest-path "${PNGX_BRIDGE_BUILD_DIR}/Cargo.toml" --release ${PNGX_BRIDGE_CARGO_FLAGS}
  BYPRODUCTS "${PNGX_BRIDGE_BUILD_DIR}/generated/pngx_bridge.h"
  WORKING_DIRECTORY "${PNGX_BRIDGE_BUILD_DIR}"
  DEPENDS
    "${PNGX_BRIDGE_BUILD_DIR}/Cargo.toml"
  COMMENT "Building pngx_bridge (Rust target: ${PNGX_BRIDGE_RUST_TARGET})"
  VERBATIM
)

add_custom_target(pngx_bridge_build DEPENDS "${PNGX_BRIDGE_LIB_PATH}" "${PNGX_BRIDGE_BUILD_DIR}/generated/pngx_bridge.h")

add_library(pngx_bridge STATIC IMPORTED GLOBAL)
set_target_properties(pngx_bridge PROPERTIES
  IMPORTED_LOCATION "${PNGX_BRIDGE_LIB_PATH}"
  INTERFACE_INCLUDE_DIRECTORIES "${PNGX_BRIDGE_BUILD_DIR}/generated"
)

add_dependencies(pngx_bridge pngx_bridge_build)

message(STATUS "PNGX bridge integrated: ${PNGX_BRIDGE_LIB_PATH}")
