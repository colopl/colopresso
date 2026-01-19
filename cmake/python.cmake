# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025 COLOPL, Inc.

# ==============================================================================
# Python Bindings CMake Configuration
# ==============================================================================

# This file is included when COLOPRESSO_PYTHON_BINDINGS is ON
# It builds a shared library suitable for Python ctypes loading

if(NOT COLOPRESSO_PYTHON_BINDINGS)
  return()
endif()

message(STATUS "Building Python bindings")

# Build shared library for Python by wrapping the static library
add_library(colopresso_python SHARED
  ${CMAKE_CURRENT_SOURCE_DIR}/library/src/colopresso.c
)

# Link against the main static library
target_link_libraries(colopresso_python PRIVATE
  colopresso
)

# Ensure all symbols from static library are included
if(UNIX AND NOT APPLE)
  target_link_options(colopresso_python PRIVATE
    -Wl,--whole-archive
    $<TARGET_FILE:colopresso>
    -Wl,--no-whole-archive
  )
endif()

target_include_directories(colopresso_python PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/library/include
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/library/src
)

# Set output name for Python loading
set_target_properties(colopresso_python PROPERTIES
  OUTPUT_NAME "_colopresso"
  PREFIX ""
  SUFFIX ".so"
)

# Install for Python package
install(
  TARGETS colopresso_python
  LIBRARY DESTINATION colopresso
  COMPONENT python
)
