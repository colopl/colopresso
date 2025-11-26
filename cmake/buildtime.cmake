# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

set(_colopresso_timestamp_fields
  YEAR "%Y" 0xFFF 20
  MONTH "%m" 0xF 16
  DAY "%d" 0x1F 11
  HOUR "%H" 0x1F 6
  MINUTE "%M" 0x3F 0
)

set(ENCODED_TIME 0)
while(_colopresso_timestamp_fields)
  list(POP_FRONT _colopresso_timestamp_fields _name _format _mask _shift)

  string(TIMESTAMP _raw "${_format}" UTC)
  math(EXPR _value "${_raw}")
  math(EXPR _encoded "(${_value}) & ${_mask}")
  math(EXPR ENCODED_TIME "${ENCODED_TIME} | (${_encoded} << ${_shift})")
endwhile()

string(TIMESTAMP CMAKE_TIMESTAMP "%s" UTC)

add_definitions(-DCOLOPRESSO_BUILDTIME=${ENCODED_TIME})

message(STATUS "Build timestamp: ${CMAKE_TIMESTAMP}")
message(STATUS "COLOPRESSO_BUILDTIME: ${ENCODED_TIME} (0x${ENCODED_TIME})")
