# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

message(STATUS "Configuring for Emscripten")

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

set(_colopresso_gui_enabled OFF)
if(COLOPRESSO_CHROME_EXTENSION OR COLOPRESSO_ELECTRON_APP)
  set(_colopresso_gui_enabled ON)
endif()

if(_colopresso_gui_enabled)
  add_compile_definitions(COLOPRESSO_ELECTRON_APP=1)
endif()

if(_colopresso_gui_enabled)
  set(COLOPRESSO_DISABLE_FILE_OPS ON CACHE BOOL "Disable file operations for Chrome Extension/Electron" FORCE)
  set(COLOPRESSO_WITH_FILE_OPS OFF CACHE BOOL "Enable file operation functions (fopen, fwrite, etc.)" FORCE)
  message(STATUS "File operations disabled for Chrome Extension/Electron build")

  find_program(PNPM_EXECUTABLE pnpm)
  if(NOT PNPM_EXECUTABLE)
    message(FATAL_ERROR "pnpm executable not found. Please install pnpm before building the GUI assets.")
  endif()

  set(VITE_BASE_ENV
    "NODE_ENV=production"
    "ROLLUP_FORCE_JS=1"
  )
  set(VITE_CLI "${CMAKE_SOURCE_DIR}/node_modules/vite/bin/vite.js")

  function(colopresso_add_vite_build OUT_VAR)
    set(options)
    set(oneValueArgs CONFIG)
    set(multiValueArgs ENV)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_CONFIG)
      message(FATAL_ERROR "CONFIG argument required for colopresso_add_vite_build")
    endif()

    set(_env ${VITE_BASE_ENV})
    if(ARG_ENV)
      list(APPEND _env ${ARG_ENV})
    endif()

    set(_commands "${${OUT_VAR}}")
    list(APPEND _commands
      COMMAND ${CMAKE_COMMAND} -E env ${_env}
        node "${VITE_CLI}" build --config "${ARG_CONFIG}"
    )
    set(${OUT_VAR} "${_commands}" PARENT_SCOPE)
  endfunction()

  set(_vite_commands)
  set(_gui_config_files "${CMAKE_SOURCE_DIR}/package.json" "${CMAKE_SOURCE_DIR}/package-lock.json")
  set(_gui_source_globs)
  set(_cleanup_dist_commands)

  if(COLOPRESSO_ELECTRON_APP)
    set(COLOPRESSO_ELECTRON_OUT_DIR "${CMAKE_BINARY_DIR}/electron")
    set(_electron_env
      "COLOPRESSO_BUILD_TARGET=electron"
      "COLOPRESSO_DEST_DIR=${COLOPRESSO_ELECTRON_OUT_DIR}"
      "COLOPRESSO_RESOURCES_DIR=${COLOPRESSO_ELECTRON_OUT_DIR}"
      "COLOPRESSO_SHARED_DIST_DIR=${COLOPRESSO_ELECTRON_OUT_DIR}"
    )
    colopresso_add_vite_build(_vite_commands
      CONFIG "${CMAKE_SOURCE_DIR}/app/shared/vite.config.ts"
      ENV ${_electron_env}
    )

    set(_electron_app_env ${_electron_env})
    list(APPEND _electron_app_env "COLOPRESSO_ELECTRON_OUT_DIR=${COLOPRESSO_ELECTRON_OUT_DIR}")
    colopresso_add_vite_build(_vite_commands
      CONFIG "${CMAKE_SOURCE_DIR}/app/electron/vite.config.ts"
      ENV ${_electron_app_env}
    )

    list(APPEND _gui_config_files
      "${CMAKE_SOURCE_DIR}/app/shared/vite.config.ts"
      "${CMAKE_SOURCE_DIR}/app/electron/vite.config.ts"
    )
    list(APPEND _gui_source_globs
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
    list(APPEND _cleanup_dist_commands
      COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_SOURCE_DIR}/app/shared/dist"
    )
  endif()

  if(COLOPRESSO_CHROME_EXTENSION)
    set(COLOPRESSO_CHROME_OUT_DIR "${CMAKE_BINARY_DIR}/chrome")
    set(_chrome_env
      "COLOPRESSO_BUILD_TARGET=chrome"
      "COLOPRESSO_RESOURCES_DIR=${COLOPRESSO_CHROME_OUT_DIR}"
      "COLOPRESSO_SHARED_DIST_DIR=${COLOPRESSO_CHROME_OUT_DIR}"
    )
    colopresso_add_vite_build(_vite_commands
      CONFIG "${CMAKE_SOURCE_DIR}/app/chrome/vite.config.ts"
      ENV ${_chrome_env}
    )

    list(APPEND _gui_config_files "${CMAKE_SOURCE_DIR}/app/chrome/vite.config.ts")
    list(APPEND _gui_source_globs
      "${CMAKE_SOURCE_DIR}/app/chrome/*.ts"
      "${CMAKE_SOURCE_DIR}/app/chrome/*.tsx"
      "${CMAKE_SOURCE_DIR}/app/chrome/*.js"
      "${CMAKE_SOURCE_DIR}/app/chrome/*.jsx"
      "${CMAKE_SOURCE_DIR}/app/chrome/*.json"
      "${CMAKE_SOURCE_DIR}/app/chrome/*.css"
      "${CMAKE_SOURCE_DIR}/app/chrome/*.scss"
      "${CMAKE_SOURCE_DIR}/app/chrome/*.html"
    )
  endif()

  file(GLOB_RECURSE COLOPRESSO_GUI_SOURCES CONFIGURE_DEPENDS
    ${_gui_config_files}
    ${_gui_source_globs}
  )

  set(COLOPRESSO_GUI_RESOURCES_STAMP "${CMAKE_BINARY_DIR}/.gui_resources.stamp")

  add_custom_command(
    OUTPUT ${COLOPRESSO_GUI_RESOURCES_STAMP}
    DEPENDS ${COLOPRESSO_GUI_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E remove "${COLOPRESSO_GUI_RESOURCES_STAMP}"
    ${_cleanup_dist_commands}
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_SOURCE_DIR}/node_modules"
    COMMAND ${PNPM_EXECUTABLE} i --frozen-lockfile
    ${_vite_commands}
    COMMAND ${CMAKE_COMMAND} -E touch ${COLOPRESSO_GUI_RESOURCES_STAMP}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building GUI assets"
    VERBATIM
    COMMAND_EXPAND_LISTS
  )

  add_custom_target(colopresso_gui_resources DEPENDS ${COLOPRESSO_GUI_RESOURCES_STAMP})
endif()

function(colopresso_configure_gui_target PLATFORM DISPLAY_NAME)
  string(TOLOWER "${PLATFORM}" PLATFORM_LOWER)

  set(TARGET_NAME "colopresso_${PLATFORM_LOWER}")
  set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/${PLATFORM_LOWER}")
  set(APP_DIR "${CMAKE_SOURCE_DIR}/app/${PLATFORM_LOWER}")
  if(PLATFORM_LOWER STREQUAL "electron" OR PLATFORM_LOWER STREQUAL "chrome")
    set(RESOURCE_DEST_DIR "${OUTPUT_DIR}")
  else()
    set(RESOURCE_DEST_DIR "${OUTPUT_DIR}/resources/${PLATFORM_LOWER}")
  endif()
  add_executable(${TARGET_NAME} ${CMAKE_SOURCE_DIR}/library/src/emscripten.c)
  target_link_libraries(${TARGET_NAME} PRIVATE colopresso)

  if(TARGET colopresso_gui_resources)
    add_dependencies(${TARGET_NAME} colopresso_gui_resources)
  endif()

  if(TARGET pngx_bridge_wasm_build)
    add_dependencies(${TARGET_NAME} pngx_bridge_wasm_build)
  endif()

  set_target_properties(${TARGET_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR}
    OUTPUT_NAME "colopresso"
    SUFFIX ".js"
  )

  set(_post_build_commands
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${APP_DIR}
      ${OUTPUT_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RESOURCE_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${OUTPUT_DIR}/colopresso.js
      ${RESOURCE_DEST_DIR}/colopresso.js
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${OUTPUT_DIR}/colopresso.wasm
      ${RESOURCE_DEST_DIR}/colopresso.wasm
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${OUTPUT_DIR}/app
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${OUTPUT_DIR}/entries
  )

  if(TARGET pngx_bridge_wasm_build)
    set(_pngx_bridge_wasm_out_dir "${CMAKE_BINARY_DIR}/pngx_bridge_wasm/pkg")
    list(APPEND _post_build_commands
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_pngx_bridge_wasm_out_dir}/pngx_bridge.js"
        ${RESOURCE_DEST_DIR}/pngx_bridge.js
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${_pngx_bridge_wasm_out_dir}/pngx_bridge_bg.wasm"
        ${RESOURCE_DEST_DIR}/pngx_bridge_bg.wasm
    )
    # Copy snippets directory for wasm-bindgen-rayon workerHelpers.js
    if(EXISTS "${_pngx_bridge_wasm_out_dir}/snippets")
      list(APPEND _post_build_commands
        COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${_pngx_bridge_wasm_out_dir}/snippets"
          ${RESOURCE_DEST_DIR}/snippets
      )
    endif()
  endif()

  set(_cleanup_files
    ${OUTPUT_DIR}/global.d.ts
    ${OUTPUT_DIR}/main.ts
    ${OUTPUT_DIR}/main.js.map
    ${OUTPUT_DIR}/preload.ts
    ${OUTPUT_DIR}/preload.js.map
    ${OUTPUT_DIR}/renderer.tsx
    ${OUTPUT_DIR}/renderer.js.map
    ${OUTPUT_DIR}/background.js.map
    ${OUTPUT_DIR}/sidepanel.js.map
    ${OUTPUT_DIR}/offscreen.js.map
    ${OUTPUT_DIR}/tsconfig.json
    ${OUTPUT_DIR}/vite.config.ts
  )

  list(APPEND _post_build_commands
    COMMAND ${CMAKE_COMMAND} -E rm -f ${_cleanup_files}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${OUTPUT_DIR}/resources
  )

  add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    ${_post_build_commands}
    COMMENT "Copying ${DISPLAY_NAME} files"
  )

  message(STATUS "${DISPLAY_NAME} output: ${OUTPUT_DIR}")
endfunction()

set(WASM_EXPORTED_FUNCTIONS
  _emscripten_convert_png_to_webp
  _emscripten_convert_png_to_avif
  _emscripten_convert_png_to_pngx
  _emscripten_get_error_string
  _emscripten_get_last_error
  _emscripten_get_last_webp_error
  _emscripten_get_last_avif_error
  _emscripten_get_pngx_oxipng_version
  _emscripten_get_pngx_libimagequant_version
  _emscripten_config_create
  _emscripten_config_free
  _emscripten_config_webp_quality
  _emscripten_config_webp_lossless
  _emscripten_config_webp_method
  _emscripten_config_webp_target_size
  _emscripten_config_webp_target_psnr
  _emscripten_config_webp_segments
  _emscripten_config_webp_sns_strength
  _emscripten_config_webp_filter_strength
  _emscripten_config_webp_filter_sharpness
  _emscripten_config_webp_filter_type
  _emscripten_config_webp_autofilter
  _emscripten_config_webp_alpha_compression
  _emscripten_config_webp_alpha_filtering
  _emscripten_config_webp_alpha_quality
  _emscripten_config_webp_pass
  _emscripten_config_webp_preprocessing
  _emscripten_config_webp_partitions
  _emscripten_config_webp_partition_limit
  _emscripten_config_webp_emulate_jpeg_size
  _emscripten_config_webp_low_memory
  _emscripten_config_webp_near_lossless
  _emscripten_config_webp_exact
  _emscripten_config_webp_use_delta_palette
  _emscripten_config_webp_use_sharp_yuv
  _emscripten_config_avif_quality
  _emscripten_config_avif_alpha_quality
  _emscripten_config_avif_lossless
  _emscripten_config_avif_speed
  _emscripten_config_pngx_level
  _emscripten_config_pngx_strip_safe
  _emscripten_config_pngx_optimize_alpha
  _emscripten_config_pngx_lossy_enable
  _emscripten_config_pngx_lossy_max_colors
  _emscripten_config_pngx_lossy_quality_min
  _emscripten_config_pngx_lossy_quality_max
  _emscripten_config_pngx_lossy_speed
  _emscripten_config_pngx_lossy_dither_level
  _emscripten_config_pngx_saliency_map_enable
  _emscripten_config_pngx_chroma_anchor_enable
  _emscripten_config_pngx_adaptive_dither_enable
  _emscripten_config_pngx_gradient_boost_enable
  _emscripten_config_pngx_chroma_weight_enable
  _emscripten_config_pngx_postprocess_smooth_enable
  _emscripten_config_pngx_postprocess_smooth_importance_cutoff
  _emscripten_config_pngx_protected_colors
  _emscripten_config_pngx_threads
  _emscripten_is_threads_enabled
  _emscripten_get_version
  _emscripten_get_libwebp_version
  _emscripten_get_libpng_version
  _emscripten_get_libavif_version
  _emscripten_get_buildtime
  _emscripten_get_compiler_version_string
  _emscripten_get_rust_version_string
  _emscripten_get_default_thread_count
  _emscripten_get_max_thread_count
  _cpres_free
  _malloc
  _free
)

string(REPLACE ";" "','" WASM_EXPORTED_FUNCTIONS_JOIN "${WASM_EXPORTED_FUNCTIONS}")
set(WASM_EXPORTED_FUNCTIONS_STR "['${WASM_EXPORTED_FUNCTIONS_JOIN}']")
set(WASM_NODE_EXPORTED_FUNCTIONS_STR "['_main','${WASM_EXPORTED_FUNCTIONS_JOIN}']")
set(WASM_RUNTIME_METHODS "['UTF8ToString','getValue','HEAPU8']")

set(COLOPRESSO_EMSCRIPTEN_STACK_SIZE 131072 CACHE STRING "Stack size (128KiB)")

if(COLOPRESSO_ENABLE_THREADS)
  set(COLOPRESSO_EMSCRIPTEN_INITIAL_MEMORY 2147483648 CACHE STRING "Initial memory size for Emscripten with threads (2GiB)")
  set(_colopresso_allow_memory_growth 0)
else()
  set(COLOPRESSO_EMSCRIPTEN_INITIAL_MEMORY 536870912 CACHE STRING "Initial memory size for Emscripten without threads (512MiB)")
  set(_colopresso_allow_memory_growth 1)
endif()
set(_colopresso_initial_memory "${COLOPRESSO_EMSCRIPTEN_INITIAL_MEMORY}")

set(COMMON_LINK_OPTIONS
  "-sALLOW_MEMORY_GROWTH=${_colopresso_allow_memory_growth}"
  "-sINITIAL_MEMORY=${_colopresso_initial_memory}"
  "-sSTACK_SIZE=${COLOPRESSO_EMSCRIPTEN_STACK_SIZE}"
)

if(COLOPRESSO_ENABLE_WASM_SIMD)
  list(APPEND COMMON_LINK_OPTIONS
    "-msimd128"
    "-msse"
    "-msse2"
    "-msse3"
    "-mssse3"
    "-msse4.1"
  )
  message(STATUS "WASM SIMD128 link options enabled")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Enabling Emscripten debug options")
  add_link_options(
    "-sASSERTIONS=2"
    "-sSAFE_HEAP=1"
    "-sSTACK_OVERFLOW_CHECK=2"
  )
endif()

if(_colopresso_gui_enabled)
  if(COLOPRESSO_CHROME_EXTENSION)
    message(STATUS "Building for Chrome Extension (threads disabled, no SharedArrayBuffer)")
    set(_gui_environment "web")
  elseif(COLOPRESSO_ENABLE_THREADS)
    message(STATUS "Building for Electron with threads enabled")
    set(_gui_environment "web,node,worker")
  else()
    message(STATUS "Building for Electron / Chrome Extension")
    set(_gui_environment "web,node")
  endif()
  add_link_options(
    ${COMMON_LINK_OPTIONS}
    "-sEXPORTED_FUNCTIONS=${WASM_EXPORTED_FUNCTIONS_STR}"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sENVIRONMENT=${_gui_environment}"
    "-sEXPORT_NAME=ColopressoModule"
    "-sFILESYSTEM=0"
    "-sEXPORTED_RUNTIME_METHODS=${WASM_RUNTIME_METHODS}"
  )
elseif(COLOPRESSO_NODE_BUILD)
  message(STATUS "Building for Node.js")
  add_link_options(
    ${COMMON_LINK_OPTIONS}
    "-sEXPORTED_FUNCTIONS=${WASM_NODE_EXPORTED_FUNCTIONS_STR}"
    "-sNODERAWFS=1"
    "-sFILESYSTEM=1"
  )
else()
  message(FATAL_ERROR "No Emscripten build target specified")
endif()

if(COLOPRESSO_CHROME_EXTENSION)
  colopresso_configure_gui_target("chrome" "Chrome extension")
endif()

if(COLOPRESSO_ELECTRON_APP)
  colopresso_configure_gui_target("electron" "Electron app")
  file(RELATIVE_PATH COLOPRESSO_ELECTRON_OUT_DIR_REL "${CMAKE_SOURCE_DIR}" "${COLOPRESSO_ELECTRON_OUT_DIR}")

  set(ELECTRON_BUILDER_ARGS exec electron-builder --publish never)
  list(APPEND ELECTRON_BUILDER_ARGS
    "-c.extraMetadata.main=${COLOPRESSO_ELECTRON_OUT_DIR_REL}/main.js"
    "-c.files=${COLOPRESSO_ELECTRON_OUT_DIR_REL}/**/*"
  )
  if(COLOPRESSO_ELECTRON_TARGETS)
    message(STATUS "Electron packaging targets: ${COLOPRESSO_ELECTRON_TARGETS}")
    string(REPLACE "," ";" COLOPRESSO_ELECTRON_TARGETS_EACH "${COLOPRESSO_ELECTRON_TARGETS}")
    foreach(ELECTRON_TARGET ${COLOPRESSO_ELECTRON_TARGETS_EACH})
      string(STRIP "${ELECTRON_TARGET}" ELECTRON_TARGET_TRIMMED)
      if(ELECTRON_TARGET_TRIMMED)
        list(APPEND ELECTRON_BUILDER_ARGS "${ELECTRON_TARGET_TRIMMED}")
      endif()
    endforeach()
  endif()

  add_custom_target(colopresso_electron_package ALL
    COMMAND ${PNPM_EXECUTABLE} ${ELECTRON_BUILDER_ARGS}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Packaging Electron application"
    VERBATIM
  )
  add_dependencies(colopresso_electron_package colopresso_electron)
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --version
  OUTPUT_VARIABLE COMPILER_VERSION_OUTPUT
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REGEX MATCH "emcc[^)]*[0-9]+\\.[0-9]+\\.[0-9]+" COLOPRESSO_COMPILER_VERSION_STRING "${COMPILER_VERSION_OUTPUT}")
if(NOT COLOPRESSO_COMPILER_VERSION_STRING)
  string(REGEX REPLACE "\n.*" "" COLOPRESSO_COMPILER_VERSION_STRING "${COMPILER_VERSION_OUTPUT}")
endif()
