# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

if(NOT COLOPRESSO_PYTHON_BINDINGS)
  return()
endif()

message(STATUS "Building Python bindings with stable ABI (Limited API for Python 3.10+)")

find_package(Python3 REQUIRED COMPONENTS Development.Module)

# Python 3.10+ stable ABI (Py_LIMITED_API)
set(COLOPRESSO_PYTHON_ABI_VERSION 0x030a0000)

add_library(colopresso_python MODULE
  ${CMAKE_CURRENT_SOURCE_DIR}/python/colopresso/_colopresso_ext.c
)

target_compile_definitions(colopresso_python PRIVATE
  Py_LIMITED_API=${COLOPRESSO_PYTHON_ABI_VERSION}
)

target_link_libraries(colopresso_python PRIVATE
  Python3::Module
)

if(UNIX AND NOT APPLE)
  target_link_libraries(colopresso_python PRIVATE
    -Wl,--whole-archive
    colopresso
    -Wl,--no-whole-archive
    m
  )
elseif(APPLE)
  target_link_libraries(colopresso_python PRIVATE
    -Wl,-force_load,$<TARGET_FILE:colopresso>
  )
else()
  target_link_libraries(colopresso_python PRIVATE colopresso)
endif()

target_include_directories(colopresso_python PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/library/include
  ${CMAKE_CURRENT_BINARY_DIR}/include
)

set_target_properties(colopresso_python PROPERTIES
  OUTPUT_NAME "_colopresso"
  PREFIX ""
)

if(WIN32)
  set_target_properties(colopresso_python PROPERTIES SUFFIX ".pyd")
elseif(APPLE)
  set_target_properties(colopresso_python PROPERTIES SUFFIX ".abi3.so")
else()
  set_target_properties(colopresso_python PROPERTIES SUFFIX ".abi3.so")
endif()

install(
  TARGETS colopresso_python
  LIBRARY DESTINATION colopresso
  COMPONENT python
)
