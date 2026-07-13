# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

set(PNGX_BRIDGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/library/pngx_bridge")
set(PNGX_BRIDGE_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/pngx_bridge")

macro(_pngx_bridge_create_wasm_stub)
  set(PNGX_BRIDGE_USE_WASM_SEPARATION ON)
  include(cmake/pngx_bridge_wasm.cmake)

  set(PNGX_BRIDGE_STUB_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/library/src/wasm.c")

  add_library(pngx_bridge STATIC "${PNGX_BRIDGE_STUB_SOURCE}")
  add_dependencies(pngx_bridge pngx_bridge_wasm_build)

  target_compile_definitions(pngx_bridge PRIVATE PNGX_BRIDGE_WASM_SEPARATION=1)

  file(MAKE_DIRECTORY "${PNGX_BRIDGE_BUILD_DIR}/generated")
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/library/include/colopresso/pngx_bridge.h.in"
    "${PNGX_BRIDGE_BUILD_DIR}/generated/pngx_bridge.h"
    @ONLY
  )

  target_include_directories(pngx_bridge PUBLIC "${PNGX_BRIDGE_BUILD_DIR}/generated")
endmacro()

if(EMSCRIPTEN)
  if(COLOPRESSO_ELECTRON_APP)
    set(COLOPRESSO_NODE_WASM_SEPARATION ON CACHE BOOL "Legacy separated pngx_bridge asset toggle (forced ON for Electron)" FORCE)
    message(STATUS "pngx_bridge: Electron build uses separated stable pngx_bridge WASM assets (no threading)")
  else()
    if(COLOPRESSO_NODE_WASM_SEPARATION)
      message(WARNING "pngx_bridge: COLOPRESSO_NODE_WASM_SEPARATION is reserved for Electron builds and is ignored for non-Electron Emscripten builds")
    endif()
    set(COLOPRESSO_NODE_WASM_SEPARATION OFF CACHE BOOL "Legacy separated pngx_bridge asset toggle (non-Electron Emscripten builds always use the integrated bridge)" FORCE)
  endif()

  if(COLOPRESSO_NODE_WASM_SEPARATION)
    _pngx_bridge_create_wasm_stub()
    return()
  else()
    message(STATUS "pngx_bridge: Node.js build uses the integrated stable pngx_bridge static library (wasm32-unknown-emscripten, no Rayon)")
  endif()
endif()

set(PNGX_BRIDGE_USE_WASM_SEPARATION OFF)

set(_pngx_bridge_artifact libpngx_bridge.a)
set(_pngx_bridge_cargo_args)

if(EMSCRIPTEN AND COLOPRESSO_NODE_BUILD)
  set(_pngx_bridge_target "wasm32-unknown-emscripten")
  set(PNGX_BRIDGE_ENABLE_RAYON OFF)
  message(STATUS "pngx_bridge: Building for wasm32-unknown-emscripten (Rayon disabled)")
else()
  set(_pngx_bridge_target native)
  set(PNGX_BRIDGE_ENABLE_RAYON ON)
  message(STATUS "pngx_bridge: Rayon multithreading enabled (native build)")
endif()

if(WIN32)
  set(_pngx_bridge_artifact pngx_bridge.lib)
endif()

set(PNGX_BRIDGE_RUST_TARGET ${_pngx_bridge_target})
set(PNGX_BRIDGE_CARGO_FLAGS ${_pngx_bridge_cargo_args})

set(_pngx_bridge_rust_profile "release")

if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT WIN32 AND NOT EMSCRIPTEN)
  set(_pngx_bridge_rust_profile "dev")
endif()

set(PNGX_BRIDGE_RUST_TARGET ${_pngx_bridge_target})

if(_pngx_bridge_rust_profile STREQUAL "release")
  set(_pngx_bridge_cargo_profile_arg "--release")
elseif(_pngx_bridge_rust_profile STREQUAL "dev")
  set(_pngx_bridge_cargo_profile_arg "")
else()
  set(_pngx_bridge_cargo_profile_arg "--profile=${_pngx_bridge_rust_profile}")
endif()

list(APPEND PNGX_BRIDGE_CARGO_FLAGS ${_pngx_bridge_cargo_profile_arg})

if(NOT PNGX_BRIDGE_RUST_TARGET STREQUAL "native")
  list(APPEND PNGX_BRIDGE_CARGO_FLAGS "--target" "${PNGX_BRIDGE_RUST_TARGET}")
endif()

set(_pngx_bridge_features "")
if(PNGX_BRIDGE_ENABLE_RAYON)
  list(APPEND _pngx_bridge_features "rayon")
endif()
if(_pngx_bridge_features)
  list(JOIN _pngx_bridge_features "," _pngx_bridge_features_str)
  list(APPEND PNGX_BRIDGE_CARGO_FLAGS "--features" "${_pngx_bridge_features_str}")
endif()

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
set(PNGX_BRIDGE_TARGET_RUSTFLAGS "")

if(EMSCRIPTEN)
  if(COLOPRESSO_ENABLE_WASM_SIMD)
    set(PNGX_BRIDGE_TARGET_RUSTFLAGS "-C target-feature=+simd128")
  endif()

  if(NOT COLOPRESSO_NODE_WASM_SEPARATION)
    if(PNGX_BRIDGE_TARGET_RUSTFLAGS)
      string(APPEND PNGX_BRIDGE_TARGET_RUSTFLAGS " ")
    endif()
    string(APPEND PNGX_BRIDGE_TARGET_RUSTFLAGS "-C panic=abort")
    message(STATUS "pngx_bridge: Integrated Emscripten build forces Rust panic=abort")
  endif()
endif()

set(_pngx_bridge_combined_rustflags "${PNGX_BRIDGE_RUSTFLAGS}")
if(DEFINED ENV{RUSTFLAGS} AND NOT "$ENV{RUSTFLAGS}" STREQUAL "")
  string(APPEND _pngx_bridge_combined_rustflags " $ENV{RUSTFLAGS}")
endif()
set(_pngx_bridge_cargo_env "RUSTFLAGS=${_pngx_bridge_combined_rustflags}")

if(PNGX_BRIDGE_TARGET_RUSTFLAGS)
  set(_pngx_bridge_combined_target_rustflags "${PNGX_BRIDGE_TARGET_RUSTFLAGS}")
  if(DEFINED ENV{CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUSTFLAGS} AND NOT "$ENV{CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUSTFLAGS}" STREQUAL "")
    string(APPEND _pngx_bridge_combined_target_rustflags " $ENV{CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUSTFLAGS}")
  endif()
  list(APPEND _pngx_bridge_cargo_env
    "CARGO_TARGET_WASM32_UNKNOWN_EMSCRIPTEN_RUSTFLAGS=${_pngx_bridge_combined_target_rustflags}")
endif()

if(DEFINED ENV{CFLAGS} AND NOT "$ENV{CFLAGS}" STREQUAL "")
  list(APPEND _pngx_bridge_cargo_env "CFLAGS=$ENV{CFLAGS}")
endif()

if(DEFINED ENV{CXXFLAGS} AND NOT "$ENV{CXXFLAGS}" STREQUAL "")
  list(APPEND _pngx_bridge_cargo_env "CXXFLAGS=$ENV{CXXFLAGS}")
endif()

if(COLOPRESSO_USE_VALGRIND)
  list(APPEND _pngx_bridge_cargo_env
    "ZSTD_SYS_USE_PKG_CONFIG=0")
  message(STATUS "pngx_bridge: Forcing zstd-sys to build from source for memory testing instrumentation")
endif()

set(_pngx_bridge_cargo_command "cargo")

list(JOIN PNGX_BRIDGE_CARGO_FLAGS " " _pngx_bridge_cargo_flags_str)

set(_pngx_bridge_build_command
  ${_pngx_bridge_cargo_command} build
  --manifest-path "${PNGX_BRIDGE_BUILD_DIR}/Cargo.toml"
  ${PNGX_BRIDGE_CARGO_FLAGS}
)
set(_pngx_bridge_build_comment "Building pngx_bridge (Rust target: ${PNGX_BRIDGE_RUST_TARGET}, flags: ${_pngx_bridge_cargo_flags_str})")

set(_pngx_bridge_cargo_toml "${PNGX_BRIDGE_BUILD_DIR}/Cargo.toml")
set(_pngx_bridge_cargo_toml_staticlib_only_script "${PNGX_BRIDGE_BUILD_DIR}/set_staticlib_only.cmake")
set(_pngx_bridge_integrated_panic_marker "# colopresso-integrated-emscripten-panic-abort")
set(_pngx_bridge_manifest_panic_profile_snippet "")
if(EMSCRIPTEN AND NOT PNGX_BRIDGE_USE_WASM_SEPARATION)
  set(_pngx_bridge_manifest_panic_profile_snippet
"if(NOT \"\${_content}\" MATCHES \"${_pngx_bridge_integrated_panic_marker}\")
  string(APPEND _content \"\\n\\n${_pngx_bridge_integrated_panic_marker}\\n[profile.dev]\\npanic = \\\"abort\\\"\\n\\n[profile.release]\\npanic = \\\"abort\\\"\\n\")
endif()")
endif()
file(WRITE "${_pngx_bridge_cargo_toml_staticlib_only_script}"
"file(READ \"${_pngx_bridge_cargo_toml}\" _content)
string(REPLACE \"crate-type = [\\\"staticlib\\\", \\\"cdylib\\\"]\" \"crate-type = [\\\"staticlib\\\"]\" _content \"\${_content}\")
${_pngx_bridge_manifest_panic_profile_snippet}
file(WRITE \"${_pngx_bridge_cargo_toml}\" \"\${_content}\")
")

set(_pngx_bridge_cargo_toml_restore_script "${PNGX_BRIDGE_BUILD_DIR}/restore_crate_type.cmake")
file(WRITE "${_pngx_bridge_cargo_toml_restore_script}"
"file(READ \"${_pngx_bridge_cargo_toml}\" _content)
string(REPLACE \"crate-type = [\\\"staticlib\\\"]\" \"crate-type = [\\\"staticlib\\\", \\\"cdylib\\\"]\" _content \"\${_content}\")
file(WRITE \"${_pngx_bridge_cargo_toml}\" \"\${_content}\")
")

if(NOT PNGX_BRIDGE_USE_WASM_SEPARATION)
  set(_pngx_bridge_pre_build_commands
    COMMAND ${CMAKE_COMMAND} -P "${_pngx_bridge_cargo_toml_staticlib_only_script}"
  )
  set(_pngx_bridge_post_build_commands
    COMMAND ${CMAKE_COMMAND} -P "${_pngx_bridge_cargo_toml_restore_script}"
  )
  set(_pngx_bridge_build_comment "Building pngx_bridge (Rust target: ${PNGX_BRIDGE_RUST_TARGET}, crate-type: staticlib only, integrated manifest panic policy applied, flags: ${_pngx_bridge_cargo_flags_str})")
else()
  set(_pngx_bridge_pre_build_commands)
  set(_pngx_bridge_post_build_commands)
endif()

add_custom_command(
  OUTPUT "${PNGX_BRIDGE_LIB_PATH}" "${PNGX_BRIDGE_BUILD_DIR}/generated/pngx_bridge.h"
  COMMAND ${CMAKE_COMMAND} -E echo "Building pngx_bridge (target: ${PNGX_BRIDGE_RUST_TARGET}, profile: ${_pngx_bridge_rust_profile})"
  ${_pngx_bridge_pre_build_commands}
  COMMAND ${CMAKE_COMMAND} -E env ${_pngx_bridge_cargo_env}
    ${_pngx_bridge_build_command}
  ${_pngx_bridge_post_build_commands}
  WORKING_DIRECTORY "${PNGX_BRIDGE_BUILD_DIR}"
  DEPENDS
    "${PNGX_BRIDGE_BUILD_DIR}/Cargo.toml"
  COMMENT "${_pngx_bridge_build_comment}"
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
