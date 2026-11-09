# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

# libavif releases pin an older libaom than libavif main does. COLOPRESSO_AOM_VERSION, which carries
# upstream security fixes, is built while it is newer than libavif's pin; once the libavif submodule
# pins this tag or a later one, libavif's own tag is used and the override has no effect.
#
# `cmake -P cmake/libaom.cmake` prints the tag that will be built, so CI can cache that checkout.

set(COLOPRESSO_AOM_VERSION 3.15.3)

# Sets <tag_var> to the libaom tag to build (for example v3.15.3) and <override_var> to TRUE when it is
# newer than the tag libavif would fetch. <tag_var> is empty when libavif's pin cannot be read.
function(colopresso_libaom_tag tag_var override_var)
  set(_local_aom "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../third_party/libavif/cmake/Modules/LocalAom.cmake")
  set(_tag "")
  set(_override FALSE)
  if(EXISTS "${_local_aom}")
    file(STRINGS "${_local_aom}" _line REGEX "^set\\(AVIF_AOM_GIT_TAG v[0-9.]+\\)$")
    string(REGEX REPLACE "^set\\(AVIF_AOM_GIT_TAG v([0-9.]+)\\)$" "\\1" _avif "${_line}")
    if(_avif AND _avif VERSION_LESS COLOPRESSO_AOM_VERSION)
      set(_tag "v${COLOPRESSO_AOM_VERSION}")
      set(_override TRUE)
    elseif(_avif)
      set(_tag "v${_avif}")
    endif()
  endif()
  set(${tag_var} "${_tag}" PARENT_SCOPE)
  set(${override_var} "${_override}" PARENT_SCOPE)
endfunction()

if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE)
  colopresso_libaom_tag(_tag _override)
  execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${_tag}")
endif()
