# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

set(PNGX_BRIDGE_WASM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/pngx_bridge")
set(PNGX_BRIDGE_WASM_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/pngx_bridge_wasm")
set(PNGX_BRIDGE_WASM_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/pngx_bridge_wasm/pkg")
option(COLOPRESSO_PNGX_WASM_DISABLE_THREADING "Disable Rayon threading in pngx_bridge WASM module" OFF)

if(COLOPRESSO_CHROME_EXTENSION)
  if(NOT COLOPRESSO_PNGX_WASM_DISABLE_THREADING)
    message(STATUS "pngx_bridge_wasm: Forcing threading OFF for Chrome Extension build")
    set(COLOPRESSO_PNGX_WASM_DISABLE_THREADING ON)
  endif()
endif()

find_program(WASM_PACK_EXECUTABLE wasm-pack)
if(NOT WASM_PACK_EXECUTABLE)
  message(FATAL_ERROR "wasm-pack not found.")
endif()

set(_pngx_bridge_wasm_cc "")

foreach(_llvm_path "/opt/homebrew/opt/llvm/bin/clang" "/usr/local/opt/llvm/bin/clang")
  if(EXISTS "${_llvm_path}")
    execute_process(
      COMMAND ${_llvm_path} --target=wasm32-unknown-unknown --version
      RESULT_VARIABLE _pngx_bridge_llvm_result
      OUTPUT_QUIET
      ERROR_QUIET
    )
    if(_pngx_bridge_llvm_result EQUAL 0)
      set(_pngx_bridge_wasm_cc "${_llvm_path}")
      message(STATUS "pngx_bridge_wasm: Found WASM-capable LLVM clang at ${_llvm_path}")
      break()
    endif()
  endif()
endforeach()

if(NOT _pngx_bridge_wasm_cc)
  find_program(_pngx_bridge_clang clang)
  if(_pngx_bridge_clang)
    execute_process(
      COMMAND ${_pngx_bridge_clang} --target=wasm32-unknown-unknown --version
      RESULT_VARIABLE _pngx_bridge_clang_wasm_result
      OUTPUT_QUIET
      ERROR_QUIET
    )
    if(_pngx_bridge_clang_wasm_result EQUAL 0)
      set(_pngx_bridge_wasm_cc "${_pngx_bridge_clang}")
      message(STATUS "pngx_bridge_wasm: Found WASM-capable clang at ${_pngx_bridge_clang}")
    else()
      message(STATUS "pngx_bridge_wasm: clang found but does not support wasm32 target")
    endif()
  endif()
endif()

if(NOT _pngx_bridge_wasm_cc)
  message(WARNING "pngx_bridge_wasm: No WASM-capable C compiler found. "
    "libdeflate-sys build may fail. On macOS, install LLVM via: brew install llvm")
endif()

set(_pngx_bridge_wasm_ar "")
if(_pngx_bridge_wasm_cc)
  get_filename_component(_pngx_bridge_llvm_dir "${_pngx_bridge_wasm_cc}" DIRECTORY)
  set(_pngx_bridge_llvm_ar_path "${_pngx_bridge_llvm_dir}/llvm-ar")
  if(EXISTS "${_pngx_bridge_llvm_ar_path}")
    set(_pngx_bridge_wasm_ar "${_pngx_bridge_llvm_ar_path}")
    message(STATUS "pngx_bridge_wasm: Found llvm-ar at ${_pngx_bridge_wasm_ar}")
  endif()
endif()

file(MAKE_DIRECTORY "${PNGX_BRIDGE_WASM_BUILD_DIR}")

set(_pngx_bridge_wasm_copy_inputs
  "${PNGX_BRIDGE_WASM_SOURCE_DIR}/Cargo.toml"
  "${PNGX_BRIDGE_WASM_SOURCE_DIR}/src/lib.rs"
  "${PNGX_BRIDGE_WASM_SOURCE_DIR}/src/wasm.rs"
)

add_custom_command(
  OUTPUT "${PNGX_BRIDGE_WASM_BUILD_DIR}/Cargo.toml"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${PNGX_BRIDGE_WASM_SOURCE_DIR}"
    "${PNGX_BRIDGE_WASM_BUILD_DIR}"
  DEPENDS ${_pngx_bridge_wasm_copy_inputs}
  COMMENT "Copying pngx_bridge source to WASM build directory"
  VERBATIM
)

set(_pngx_bridge_wasm_build_commands)
set(_pngx_bridge_wasm_env)
if(_pngx_bridge_wasm_cc)
  list(APPEND _pngx_bridge_wasm_env
    "CC_wasm32_unknown_unknown=${_pngx_bridge_wasm_cc}"
    "CC_wasm32-unknown-unknown=${_pngx_bridge_wasm_cc}"
  )
endif()
if(_pngx_bridge_wasm_ar)
  list(APPEND _pngx_bridge_wasm_env
    "AR_wasm32_unknown_unknown=${_pngx_bridge_wasm_ar}"
    "AR_wasm32-unknown-unknown=${_pngx_bridge_wasm_ar}"
  )
endif()

if(COLOPRESSO_PNGX_WASM_DISABLE_THREADING)
  message(STATUS "pngx_bridge_wasm: Building with stable Rust (no threading)")

  list(APPEND _pngx_bridge_wasm_env
    "RUSTFLAGS=-C target-feature=+simd128"
  )

  set(_pngx_bridge_wasm_feature "wasm-bindgen")
  set(_pngx_bridge_wasm_build_comment "Building pngx_bridge WASM module (stable Rust, no threading)")

  list(APPEND _pngx_bridge_wasm_build_commands
    COMMAND ${CMAKE_COMMAND} -E echo "Building pngx_bridge WASM module (stable Rust, no threading)"
    COMMAND ${CMAKE_COMMAND} -E env ${_pngx_bridge_wasm_env}
      wasm-pack build
      --target web
      --out-dir "${PNGX_BRIDGE_WASM_OUT_DIR}"
      --out-name pngx_bridge
      --
      --features ${_pngx_bridge_wasm_feature}
  )

else()
  message(STATUS "pngx_bridge_wasm: Building with nightly Rust (integrated mode with threading)")

  list(APPEND _pngx_bridge_wasm_env
    "ZSTD_SYS_USE_PKG_CONFIG=0"
  )

  set(_pngx_bridge_wasm_feature "wasm-bindgen-rayon")
  set(_pngx_bridge_wasm_build_comment "Building pngx_bridge WASM module (nightly Rust with wasm-bindgen-rayon)")

  list(APPEND _pngx_bridge_wasm_build_commands
    COMMAND ${CMAKE_COMMAND} -E echo "Configuring nightly Rust toolchain for WASM threading"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${PNGX_BRIDGE_WASM_SOURCE_DIR}/rust-toolchain-nightly.toml"
      "${PNGX_BRIDGE_WASM_BUILD_DIR}/rust-toolchain.toml"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PNGX_BRIDGE_WASM_BUILD_DIR}/.cargo"
    COMMAND ${CMAKE_COMMAND} -E copy
      "${PNGX_BRIDGE_WASM_SOURCE_DIR}/cargo-config-nightly.toml"
      "${PNGX_BRIDGE_WASM_BUILD_DIR}/.cargo/config.toml"
    COMMAND ${CMAKE_COMMAND} -E echo "Building pngx_bridge WASM module (nightly Rust, wasm-bindgen-rayon)"
    COMMAND ${CMAKE_COMMAND} -E env ${_pngx_bridge_wasm_env}
      wasm-pack build
      --target web
      --out-dir "${PNGX_BRIDGE_WASM_OUT_DIR}"
      --out-name pngx_bridge
      --
      --features ${_pngx_bridge_wasm_feature}
  )

endif()

set(PNGX_BRIDGE_WASM_JS_PATH "${PNGX_BRIDGE_WASM_OUT_DIR}/pngx_bridge.js")
set(PNGX_BRIDGE_WASM_PATH "${PNGX_BRIDGE_WASM_OUT_DIR}/pngx_bridge_bg.wasm")
set(PNGX_BRIDGE_WASM_DTS_PATH "${PNGX_BRIDGE_WASM_OUT_DIR}/pngx_bridge.d.ts")

add_custom_command(
  OUTPUT "${PNGX_BRIDGE_WASM_JS_PATH}" "${PNGX_BRIDGE_WASM_PATH}"
  ${_pngx_bridge_wasm_build_commands}
  WORKING_DIRECTORY "${PNGX_BRIDGE_WASM_BUILD_DIR}"
  DEPENDS "${PNGX_BRIDGE_WASM_BUILD_DIR}/Cargo.toml"
  COMMENT "${_pngx_bridge_wasm_build_comment}"
  VERBATIM
)

add_custom_target(pngx_bridge_wasm_build
  DEPENDS "${PNGX_BRIDGE_WASM_JS_PATH}" "${PNGX_BRIDGE_WASM_PATH}"
)

# Export variables for use by other CMake files
set(PNGX_BRIDGE_WASM_THREADING_ENABLED ON)
if(COLOPRESSO_PNGX_WASM_DISABLE_THREADING)
  set(PNGX_BRIDGE_WASM_THREADING_ENABLED OFF)
endif()

message(STATUS "pngx_bridge_wasm: Output directory: ${PNGX_BRIDGE_WASM_OUT_DIR}")
message(STATUS "pngx_bridge_wasm: JS binding: ${PNGX_BRIDGE_WASM_JS_PATH}")
message(STATUS "pngx_bridge_wasm: WASM module: ${PNGX_BRIDGE_WASM_PATH}")
message(STATUS "pngx_bridge_wasm: Threading enabled: ${PNGX_BRIDGE_WASM_THREADING_ENABLED}")
