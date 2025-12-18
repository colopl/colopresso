# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

include(CTest)

enable_testing()
FetchContent_Declare(unity SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/unity)
FetchContent_MakeAvailable(unity)

set(COLOPRESSO_TEST_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
add_definitions(-DCOLOPRESSO_TEST_ASSETS_DIR="${COLOPRESSO_TEST_ASSETS_DIR}")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
if(COLOPRESSO_ENABLE_COVERAGE)
  set(_coverage_dir ${CMAKE_BINARY_DIR}/coverage)
  file(MAKE_DIRECTORY ${_coverage_dir})
  add_custom_target(coverage
    COMMAND ${LCOV} --initial --directory ${CMAKE_BINARY_DIR} --capture --output-file ${_coverage_dir}/base.info
    COMMAND ${CMAKE_CTEST_COMMAND} -C ${CMAKE_BUILD_TYPE} --parallel
    COMMAND ${LCOV} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${_coverage_dir}/test.info
    COMMAND ${LCOV} --add-tracefile ${_coverage_dir}/base.info --add-tracefile ${_coverage_dir}/test.info --output-file ${_coverage_dir}/total.info
    COMMAND ${LCOV} --remove ${_coverage_dir}/total.info '*/third_party/*' --ignore-errors unused --output-file ${_coverage_dir}/filtered.info
    COMMAND ${GENHTML} --demangle-cpp --legend --title "${CMAKE_PROJECT_NAME} Coverage Report" --output-directory ${_coverage_dir}/html ${_coverage_dir}/filtered.info
    COMMENT "Generating coverage report with lcov/gcov"
  )
  message(STATUS "Coverage report generation target 'coverage' is now available")
endif()

file(GLOB TEST_SOURCES "library/tests/test_*.c")

set(_colopresso_test_libs
  colopresso
  png_static
  zlibstatic
  webp
  avif_static
)
if(TARGET pngx_bridge)
  list(APPEND _colopresso_test_libs pngx_bridge)
endif()
list(APPEND _colopresso_test_libs unity)

function(colopresso_register_test SOURCE)
  get_filename_component(_test_name "${SOURCE}" NAME_WE)
  add_executable(${_test_name} "${SOURCE}")
  target_link_libraries(${_test_name} PRIVATE ${_colopresso_test_libs})
  target_include_directories(${_test_name} PRIVATE ${unity_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/library/include)
  set_target_properties(${_test_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
  add_test(NAME ${_test_name} COMMAND ${_test_name})

  if(COLOPRESSO_USE_MSAN)
    set_tests_properties(${_test_name} PROPERTIES
      ENVIRONMENT "MSAN_OPTIONS=halt_on_error=0:track_origins=2"
    )
  endif()

  if(COLOPRESSO_ENABLE_VALGRIND)
    set(_valgrind_supp "${CMAKE_SOURCE_DIR}/suppressions/valgrind.supp")

    set(_valgrind_args
      "--tool=memcheck"
      "--error-exitcode=1"
      "--suppressions=${_valgrind_supp}"
      "--leak-check=${COLOPRESSO_VALGRIND_LEAK_CHECK}"
      "--show-leak-kinds=${COLOPRESSO_VALGRIND_SHOW_LEAK_KINDS}"
      "--run-libc-freeres=no"
    )

    if(COLOPRESSO_VALGRIND_TRACK_ORIGINS)
      list(APPEND _valgrind_args "--track-origins=yes")
    endif()

    add_test(
      NAME ${_test_name}_valgrind
      COMMAND ${VALGRIND}
        ${_valgrind_args}
      $<TARGET_FILE:${_test_name}>)

    set_tests_properties(${_test_name}_valgrind PROPERTIES
      ENVIRONMENT "RAYON_NUM_THREADS=${COLOPRESSO_VALGRIND_RAYON_NUM_THREADS}"
    )
  endif()
endfunction()

foreach(TEST_SOURCE ${TEST_SOURCES})
  colopresso_register_test(${TEST_SOURCE})
endforeach()
