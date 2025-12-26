# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

install(TARGETS colopresso
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY library/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
  PATTERN "*.in" EXCLUDE
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/colopresso_config.h"
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(COLOPRESSO_WITH_FILE_OPS)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/include/colopresso/file.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/colopresso
  )
endif()
