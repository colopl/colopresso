# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2026 COLOPL, Inc.
# 
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

"""
colopresso - Image compression and color reduction library
"""

from .core import (
    Config,
    PngxLossyType,
    encode_webp,
    encode_avif,
    encode_pngx,
    get_version,
    get_libwebp_version,
    get_libpng_version,
    get_libavif_version,
    get_pngx_oxipng_version,
    get_pngx_libimagequant_version,
    get_buildtime,
    get_compiler_version_string,
    get_rust_version_string,
    is_threads_enabled,
    get_default_thread_count,
    get_max_thread_count,
    ColopressoError,
)

__all__ = [
    "Config",
    "PngxLossyType",
    "encode_webp",
    "encode_avif",
    "encode_pngx",
    "get_version",
    "get_libwebp_version",
    "get_libpng_version",
    "get_libavif_version",
    "get_pngx_oxipng_version",
    "get_pngx_libimagequant_version",
    "get_buildtime",
    "get_compiler_version_string",
    "get_rust_version_string",
    "is_threads_enabled",
    "get_default_thread_count",
    "get_max_thread_count",
    "ColopressoError",
]

__version__ = "0.0.0"
