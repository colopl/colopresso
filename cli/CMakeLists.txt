# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

if(NOT COLOPRESSO_WITH_FILE_OPS)
  message(FATAL_ERROR "CLI requires COLOPRESSO_WITH_FILE_OPS=ON. CLI will not be built.")
endif()

add_executable(colopresso_cli colopresso.c)

target_link_libraries(colopresso_cli PRIVATE colopresso)
target_include_directories(colopresso_cli PRIVATE ${CMAKE_SOURCE_DIR}/library/include)

set_target_properties(colopresso_cli PROPERTIES OUTPUT_NAME colopresso)

if(EMSCRIPTEN)
  set_target_properties(colopresso_cli PROPERTIES SUFFIX ".js")
else()
  if(UNIX AND NOT APPLE)
    target_link_libraries(colopresso_cli PRIVATE m pthread)
  elseif(APPLE)
    target_link_libraries(colopresso_cli PRIVATE m)
  endif()
endif()

install(TARGETS colopresso_cli RUNTIME DESTINATION bin)

if(WIN32)
  message(STATUS "CLI: Building with Windows getopt implementation")
else()
  message(STATUS "CLI: Building with system getopt")
endif()
message(STATUS "CLI: colopresso executable will be built")
