# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

"""
Core Python bindings for colopresso (stable ABI wrapper)
"""

from __future__ import annotations

from dataclasses import dataclass, field, asdict
from enum import IntEnum
from typing import List, Optional, Tuple

from . import _colopresso


class PngxLossyType(IntEnum):
    """PNGX lossy compression type"""
    PALETTE256 = _colopresso.PNGX_LOSSY_TYPE_PALETTE256
    LIMITED_RGBA4444 = _colopresso.PNGX_LOSSY_TYPE_LIMITED_RGBA4444
    REDUCED_RGBA32 = _colopresso.PNGX_LOSSY_TYPE_REDUCED_RGBA32


class ColopressoError(Exception):
    """Exception raised for colopresso errors"""
    
    ERROR_CODES = {
        0: "OK",
        1: "File not found",
        2: "Invalid PNG",
        3: "Invalid format",
        4: "Out of memory",
        5: "Encode failed",
        6: "Decode failed",
        7: "IO error",
        8: "Invalid parameter",
        9: "Output not smaller",
    }
    
    def __init__(self, code: int, message: Optional[str] = None):
        self.code = code
        self.message = message or self.ERROR_CODES.get(code, f"Unknown error ({code})")
        super().__init__(self.message)


@dataclass
class Config:
    """Configuration for colopresso encoders"""
    
    # WebP
    webp_quality: float = 80.0
    webp_lossless: bool = False
    webp_method: int = 6
    webp_target_size: int = 0
    webp_target_psnr: float = 0.0
    webp_segments: int = 4
    webp_sns_strength: int = 50
    webp_filter_strength: int = 60
    webp_filter_sharpness: int = 0
    webp_filter_type: int = 1
    webp_autofilter: bool = True
    webp_alpha_compression: bool = True
    webp_alpha_filtering: int = 1
    webp_alpha_quality: int = 100
    webp_pass: int = 1
    webp_preprocessing: int = 0
    webp_partitions: int = 0
    webp_partition_limit: int = 0
    webp_emulate_jpeg_size: bool = False
    webp_thread_level: int = 0
    webp_low_memory: bool = False
    webp_near_lossless: int = 100
    webp_exact: bool = False
    webp_use_delta_palette: bool = False
    webp_use_sharp_yuv: bool = False
    
    # AVIF
    avif_quality: float = 50.0
    avif_alpha_quality: int = 100
    avif_lossless: bool = False
    avif_speed: int = 6
    avif_threads: int = 1
    
    # PNGX (PNG)
    pngx_level: int = 5
    pngx_strip_safe: bool = True
    pngx_optimize_alpha: bool = True
    pngx_lossy_enable: bool = True
    pngx_lossy_type: int = 0  # PngxLossyType.PALETTE256
    pngx_lossy_max_colors: int = 256
    pngx_lossy_reduced_colors: int = -1
    pngx_lossy_reduced_bits_rgb: int = 4
    pngx_lossy_reduced_alpha_bits: int = 4
    pngx_lossy_quality_min: int = 80
    pngx_lossy_quality_max: int = 95
    pngx_lossy_speed: int = 3
    pngx_lossy_dither_level: float = 0.6
    pngx_saliency_map_enable: bool = True
    pngx_chroma_anchor_enable: bool = True
    pngx_adaptive_dither_enable: bool = True
    pngx_gradient_boost_enable: bool = True
    pngx_chroma_weight_enable: bool = True
    pngx_postprocess_smooth_enable: bool = True
    pngx_postprocess_smooth_importance_cutoff: float = 0.6
    pngx_palette256_gradient_profile_enable: bool = True
    pngx_palette256_gradient_dither_floor: float = 0.78
    pngx_palette256_alpha_bleed_enable: bool = True
    pngx_palette256_alpha_bleed_max_distance: int = 64
    pngx_palette256_alpha_bleed_opaque_threshold: int = 248
    pngx_palette256_alpha_bleed_soft_limit: int = 160
    pngx_palette256_profile_opaque_ratio_threshold: float = 0.90
    pngx_palette256_profile_gradient_mean_max: float = 0.16
    pngx_palette256_profile_saturation_mean_max: float = 0.42
    pngx_palette256_tune_opaque_ratio_threshold: float = 0.90
    pngx_palette256_tune_gradient_mean_max: float = 0.14
    pngx_palette256_tune_saturation_mean_max: float = 0.35
    pngx_palette256_tune_speed_max: int = 1
    pngx_palette256_tune_quality_min_floor: int = 90
    pngx_palette256_tune_quality_max_target: int = 100
    pngx_threads: int = 1
    pngx_protected_colors: Optional[List[Tuple[int, int, int, int]]] = None
    
    def _to_dict(self) -> dict:
        """Convert to dictionary for C extension"""
        d = asdict(self)
        if isinstance(d.get("pngx_lossy_type"), PngxLossyType):
            d["pngx_lossy_type"] = int(d["pngx_lossy_type"])
        return d


def _wrap_error(func):
    """Wrap C extension errors into ColopressoError"""
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except _colopresso.ColopressoError as e:
            code = e.args[0] if e.args else -1
            message = e.args[1] if len(e.args) > 1 else None
            raise ColopressoError(code, message) from None
    return wrapper


@_wrap_error
def encode_webp(png_data: bytes, config: Optional[Config] = None) -> bytes:
    """
    Encode PNG data to WebP format.
    
    Args:
        png_data: Raw PNG file data
        config: Optional configuration (uses defaults if not provided)
    
    Returns:
        WebP encoded data
    
    Raises:
        ColopressoError: If encoding fails
    """
    config_dict = config._to_dict() if config else None
    return _colopresso.encode_webp(png_data, config_dict)


@_wrap_error
def encode_avif(png_data: bytes, config: Optional[Config] = None) -> bytes:
    """
    Encode PNG data to AVIF format.
    
    Args:
        png_data: Raw PNG file data
        config: Optional configuration (uses defaults if not provided)
    
    Returns:
        AVIF encoded data
    
    Raises:
        ColopressoError: If encoding fails
    """
    config_dict = config._to_dict() if config else None
    return _colopresso.encode_avif(png_data, config_dict)


@_wrap_error
def encode_pngx(png_data: bytes, config: Optional[Config] = None) -> bytes:
    """
    Optimize PNG data using PNGX encoder.
    
    Args:
        png_data: Raw PNG file data
        config: Optional configuration (uses defaults if not provided)
            For PALETTE256 mode, you can specify protected colors using
            config.pngx_protected_colors as a list of (r, g, b, a) tuples.
            These colors will always be included in the palette.
    
    Returns:
        Optimized PNG data
    
    Raises:
        ColopressoError: If encoding fails
    """
    config_dict = config._to_dict() if config else None
    return _colopresso.encode_pngx(png_data, config_dict)


def get_version() -> int:
    """Get colopresso version number"""
    return _colopresso.get_version()


def get_libwebp_version() -> int:
    """Get libwebp version number"""
    return _colopresso.get_libwebp_version()


def get_libpng_version() -> int:
    """Get libpng version number"""
    return _colopresso.get_libpng_version()


def get_libavif_version() -> int:
    """Get libavif version number"""
    return _colopresso.get_libavif_version()


def get_pngx_oxipng_version() -> int:
    """Get oxipng version number"""
    return _colopresso.get_pngx_oxipng_version()


def get_pngx_libimagequant_version() -> int:
    """Get libimagequant version number"""
    return _colopresso.get_pngx_libimagequant_version()


def get_buildtime() -> int:
    """Get build timestamp"""
    return _colopresso.get_buildtime()


def get_compiler_version_string() -> str:
    """Get compiler version string"""
    return _colopresso.get_compiler_version_string()


def get_rust_version_string() -> str:
    """Get Rust version string"""
    return _colopresso.get_rust_version_string()


def is_threads_enabled() -> bool:
    """Check if threading is enabled"""
    return _colopresso.is_threads_enabled()


def get_default_thread_count() -> int:
    """Get default thread count"""
    return _colopresso.get_default_thread_count()


def get_max_thread_count() -> int:
    """Get maximum thread count"""
    return _colopresso.get_max_thread_count()
