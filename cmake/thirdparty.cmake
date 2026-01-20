# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

function(colopresso_force_cache VAR TYPE VALUE)
  set(${VAR} ${VALUE} CACHE ${TYPE} "" FORCE)
endfunction()

colopresso_force_cache(BUILD_SHARED_LIBS BOOL OFF)

if(EMSCRIPTEN AND COLOPRESSO_ENABLE_WASM_SIMD)
  message(STATUS "Enabling WASM SIMD128 optimizations")
  add_compile_options(-msimd128 -msse -msse2 -msse3 -mssse3 -msse4.1)
  colopresso_force_cache(WEBP_ENABLE_SIMD BOOL ON)
elseif(EMSCRIPTEN)
  colopresso_force_cache(WEBP_ENABLE_SIMD BOOL OFF)
endif()

# zlib
set(ZLIB_LIBRARY_TYPE STATIC)
colopresso_force_cache(ZLIB_BUILD_EXAMPLES BOOL OFF)
colopresso_force_cache(ZLIB_BUILD_SHARED BOOL OFF)
colopresso_force_cache(ZLIB_BUILD_STATIC BOOL ON)
colopresso_force_cache(ZLIB_ROOT PATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib")
colopresso_force_cache(ZLIB_INCLUDE_DIR PATH "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib;${CMAKE_CURRENT_SOURCE_DIR}/third_party/zlib")
if(WIN32)
  colopresso_force_cache(ZLIB_LIBRARY FILEPATH "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib/Release/zlibstatic.lib")
else()
  colopresso_force_cache(ZLIB_LIBRARY FILEPATH "${CMAKE_CURRENT_BINARY_DIR}/third_party/zlib/libz.a")
endif()
foreach(_skip_target LIBRARIES HEADERS FILES)
  colopresso_force_cache(SKIP_INSTALL_${_skip_target} BOOL OFF)
endforeach()
colopresso_force_cache(SKIP_INSTALL_ALL BOOL ON)

# libpng
colopresso_force_cache(PNG_SHARED BOOL OFF)
colopresso_force_cache(PNG_STATIC BOOL ON)
colopresso_force_cache(PNG_TESTS BOOL OFF)
colopresso_force_cache(PNG_DEBUG BOOL OFF)
colopresso_force_cache(PNGARG BOOL OFF)
colopresso_force_cache(PNG_TOOLS BOOL OFF)
colopresso_force_cache(PNG_DEBUG_POSTFIX STRING "")

# libavif
foreach(_avif_flag
    BUILD_APPS BUILD_TESTS BUILD_EXAMPLES BUILD_MAN_PAGES
    ENABLE_WERROR ENABLE_COVERAGE ENABLE_EXPERIMENTAL_MINI
    ENABLE_EXPERIMENTAL_SAMPLE_TRANSFORM ENABLE_EXPERIMENTAL_EXTENDED_PIXI)
  colopresso_force_cache(AVIF_${_avif_flag} BOOL OFF)
endforeach()

foreach(_avif_option ZLIBPNG JPEG LIBYUV LIBSHARPYUV LIBXML2)
  colopresso_force_cache(AVIF_${_avif_option} STRING OFF)
endforeach()

colopresso_force_cache(AVIF_CODEC_AOM STRING LOCAL)
colopresso_force_cache(AVIF_CODEC_AOM_ENCODE STRING ON)
foreach(_codec AOM_DECODE DAV1D LIBGAV1 RAV1E SVT AVM)
  colopresso_force_cache(AVIF_CODEC_${_codec} STRING OFF)
endforeach()

set(COLOPRESSO_BACKUP_BUILD_TESTING ${BUILD_TESTING})
set(BUILD_TESTING OFF)

# zlib (suppress warnings)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/zlib EXCLUDE_FROM_ALL)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS OFF CACHE BOOL "" FORCE)
if(TARGET zlib)
  get_target_property(ZLIB_TYPE zlib TYPE)
  if(ZLIB_TYPE STREQUAL "SHARED_LIBRARY" OR ZLIB_TYPE STREQUAL "STATIC_LIBRARY")
    set_target_properties(zlib PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
endif()

# libpng
add_subdirectory(third_party/libpng EXCLUDE_FROM_ALL)

# libwebp (special handling for WASM SIMD)
if(EMSCRIPTEN AND COLOPRESSO_ENABLE_WASM_SIMD)
  set(_saved_c_flags "${CMAKE_C_FLAGS}")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -U__AVX__ -U__AVX2__")
endif()

add_subdirectory(third_party/libwebp EXCLUDE_FROM_ALL)

if(EMSCRIPTEN AND COLOPRESSO_ENABLE_WASM_SIMD)
  set(CMAKE_C_FLAGS "${_saved_c_flags}")
  unset(_saved_c_flags)
endif()

# libavif
add_subdirectory(third_party/libavif EXCLUDE_FROM_ALL)

set(BUILD_TESTING ${COLOPRESSO_BACKUP_BUILD_TESTING})

# libwebp SIMD workaround for Emscripten
if(EMSCRIPTEN AND COLOPRESSO_ENABLE_WASM_SIMD)
  set(_webp_targets
    webp webpdecoder webpdemux webpmux sharpyuv
    webpdsp webpdspdecode webpdecode webpencode webputils webputilsdecode
  )
  foreach(_target ${_webp_targets})
    if(TARGET ${_target})
      target_compile_options(${_target} PRIVATE -U__AVX__ -U__AVX2__)
      target_compile_definitions(${_target} PRIVATE
        HAVE_CONFIG_H=1
        WEBP_HAVE_SSE2=1
        WEBP_HAVE_SSE41=1
      )
    endif()
  endforeach()
endif()
