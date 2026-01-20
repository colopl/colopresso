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

find_package(Python3 QUIET COMPONENTS Interpreter Development.Module)

if(NOT Python3_Development.Module_FOUND)
  message(STATUS "Python3::Module not found via CMake, using Python to determine paths")

  if(NOT Python3_EXECUTABLE)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
  endif()

  execute_process(
    COMMAND ${Python3_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_path('include'))"
    OUTPUT_VARIABLE _python_include_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _python_include_result
  )

  if(NOT _python_include_result EQUAL 0 OR NOT EXISTS "${_python_include_dir}")
    message(FATAL_ERROR "Failed to determine Python include directory")
  endif()

  if(WIN32)
    execute_process(
      COMMAND ${Python3_EXECUTABLE} -c "import sys; print(sys.prefix)"
      OUTPUT_VARIABLE _python_prefix
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
      COMMAND ${Python3_EXECUTABLE} -c "import sys; print(f'python{sys.version_info.major}{sys.version_info.minor}')"
      OUTPUT_VARIABLE _python_lib_name
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(_python_lib_dir "${_python_prefix}/libs")
    message(STATUS "Python prefix: ${_python_prefix}")
    message(STATUS "Python libs directory: ${_python_lib_dir}")
  endif()

  set(COLOPRESSO_PYTHON_INCLUDE_DIR "${_python_include_dir}")
  set(COLOPRESSO_PYTHON_MANUAL_CONFIG ON)
  message(STATUS "Python include directory: ${COLOPRESSO_PYTHON_INCLUDE_DIR}")
endif()

# Python 3.10+ stable ABI (Py_LIMITED_API)
set(COLOPRESSO_PYTHON_ABI_VERSION 0x030a0000)

add_library(colopresso_python MODULE
  ${CMAKE_CURRENT_SOURCE_DIR}/python/colopresso/_colopresso_ext.c
)

target_compile_definitions(colopresso_python PRIVATE
  Py_LIMITED_API=${COLOPRESSO_PYTHON_ABI_VERSION}
)

if(COLOPRESSO_PYTHON_MANUAL_CONFIG)
  target_include_directories(colopresso_python PRIVATE
    ${COLOPRESSO_PYTHON_INCLUDE_DIR}
  )
  if(WIN32 AND _python_lib_dir AND _python_lib_name)
    message(STATUS "Searching for Python libraries in: ${_python_lib_dir}")
    file(GLOB _libs_in_dir "${_python_lib_dir}/*.lib")
    message(STATUS "Available .lib files: ${_libs_in_dir}")

    find_library(_python3_lib
      NAMES python3
      PATHS "${_python_lib_dir}"
      NO_DEFAULT_PATH
    )
    if(_python3_lib)
      message(STATUS "Found python3.lib: ${_python3_lib}")
      target_link_libraries(colopresso_python PRIVATE "${_python3_lib}")
    else()
      message(STATUS "python3.lib not found, trying ${_python_lib_name}")
      find_library(_python_lib
        NAMES "${_python_lib_name}"
        PATHS "${_python_lib_dir}"
        NO_DEFAULT_PATH
      )
      if(_python_lib)
        message(STATUS "Found ${_python_lib_name}.lib: ${_python_lib}")
        target_link_libraries(colopresso_python PRIVATE "${_python_lib}")
      else()
        message(FATAL_ERROR "Could not find Python library in ${_python_lib_dir}")
      endif()
    endif()
  endif()
else()
  target_link_libraries(colopresso_python PRIVATE Python3::Module)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(colopresso_python PRIVATE
    -Wl,--whole-archive
    colopresso
    -Wl,--no-whole-archive
    m
  )
elseif(APPLE)
  add_dependencies(colopresso_python colopresso)
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
