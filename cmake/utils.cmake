# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

if(COLOPRESSO_DISABLE_FILE_OPS)
  message(WARNING "Skipping utils build (COLOPRESSO_DISABLE_FILE_OPS=ON)")
  return()
endif()

file(GLOB UTIL_SOURCES "library/utils/*.c")
foreach(UTIL_SOURCE ${UTIL_SOURCES})
  get_filename_component(UTIL_NAME ${UTIL_SOURCE} NAME_WE)

  set(UTIL_TARGET "util_${UTIL_NAME}")
  add_executable(${UTIL_TARGET} ${UTIL_SOURCE})
  target_link_libraries(${UTIL_TARGET} PRIVATE colopresso)

  set(_util_properties
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/utils
    OUTPUT_NAME ${UTIL_NAME}
  )
  if(EMSCRIPTEN)
    list(APPEND _util_properties SUFFIX ".js")
  endif()
  set_target_properties(${UTIL_TARGET} PROPERTIES ${_util_properties})
endforeach()
