/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2025 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

use imagequant::{new as iq_new, Error as IqError, RGBA as IqRGBA};
use oxipng::Options;
use std::os::raw::c_int;
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::ptr;
use std::slice;
#[cfg(not(target_family = "wasm"))]
use std::sync::Once;

#[cfg(not(target_family = "wasm"))]
static RAYON_INIT: Once = Once::new();

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum PngxResult {
    Success = 0,
    InvalidInput = 1,
    OptimizationFailed = 2,
    IoError = 3,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct RgbaColor {
    pub r: u8,
    pub g: u8,
    pub b: u8,
    pub a: u8,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct PngxBridgeLosslessOptions {
    pub optimization_level: u8,
    pub strip_safe: bool,
    pub optimize_alpha: bool,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct PngxBridgeQuantParams {
    pub speed: i32,
    pub quality_min: u8,
    pub quality_max: u8,
    pub max_colors: u32,
    pub min_posterization: i32,
    pub dithering_level: f32,
    pub importance_map: *const u8,
    pub importance_map_len: usize,
    pub fixed_colors: *const RgbaColor,
    pub fixed_colors_len: usize,
    pub remap: bool,
}

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct PngxBridgeQuantOutput {
    pub palette: *mut RgbaColor,
    pub palette_len: usize,
    pub indices: *mut u8,
    pub indices_len: usize,
    pub quality: i32,
}

#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum PngxBridgeQuantStatus {
    Ok = 0,
    QualityTooLow = 1,
    Error = 2,
}

fn convert_lossless_options(opts: &PngxBridgeLosslessOptions) -> Options {
    let mut options = Options::from_preset(opts.optimization_level);
    if opts.strip_safe {
        options.strip = oxipng::StripChunks::Safe;
    }
    options.optimize_alpha = opts.optimize_alpha;
    options
}

fn allocate_copy<T: Copy>(slice: &[T]) -> Result<*mut T, String> {
    if slice.is_empty() {
        return Ok(ptr::null_mut());
    }
    let bytes = slice
        .len()
        .checked_mul(std::mem::size_of::<T>())
        .ok_or_else(|| {
            format!(
                "Integer overflow when calculating allocation size: {} * {}",
                slice.len(),
                std::mem::size_of::<T>()
            )
        })?;
    let ptr = unsafe { libc::malloc(bytes) as *mut T };
    if ptr.is_null() {
        return Err(format!("Memory allocation failed for {} bytes", bytes));
    }
    unsafe {
        ptr::copy_nonoverlapping(slice.as_ptr(), ptr, slice.len());
    }
    Ok(ptr)
}

fn iq_rgba_from_slice(src: &[RgbaColor]) -> Vec<IqRGBA> {
    let mut out = Vec::with_capacity(src.len());
    for px in src {
        out.push(IqRGBA {
            r: px.r,
            g: px.g,
            b: px.b,
            a: px.a,
        });
    }
    out
}

fn convert_palette(palette: &[IqRGBA]) -> Vec<RgbaColor> {
    let mut out = Vec::with_capacity(palette.len());
    for px in palette {
        out.push(RgbaColor {
            r: px.r,
            g: px.g,
            b: px.b,
            a: px.a,
        });
    }
    out
}

struct QuantizeOutcome {
    palette: Option<Vec<RgbaColor>>,
    indices: Option<Vec<u8>>,
    quality: i32,
}

enum QuantizeError {
    QualityTooLow,
    Generic,
}

fn quantize_image(
    pixels: &[RgbaColor],
    width: usize,
    height: usize,
    params: &PngxBridgeQuantParams,
) -> Result<QuantizeOutcome, QuantizeError> {
    if width == 0 || height == 0 || pixels.len() != width * height {
        return Err(QuantizeError::Generic);
    }

    let mut attr = iq_new();
    debug_assert!((1..=10).contains(&params.speed));
    attr.set_speed(params.speed)
        .map_err(|_| QuantizeError::Generic)?;

    let quality_min = params.quality_min;
    let mut quality_max = params.quality_max;
    if quality_max < quality_min {
        quality_max = quality_min;
    }
    attr.set_quality(quality_min, quality_max)
        .map_err(|_| QuantizeError::Generic)?;

    debug_assert!((2..=256).contains(&params.max_colors));
    attr.set_max_colors(params.max_colors)
        .map_err(|_| QuantizeError::Generic)?;

    if params.min_posterization >= 0 {
        let posterization = params.min_posterization as u8;
        attr.set_min_posterization(posterization)
            .map_err(|_| QuantizeError::Generic)?;
    }

    let rgba_vec = iq_rgba_from_slice(pixels);
    let mut image = attr
        .new_image(rgba_vec, width, height, 0.0)
        .map_err(|_| QuantizeError::Generic)?;

    if !params.importance_map.is_null() && params.importance_map_len == pixels.len() {
        let map =
            unsafe { slice::from_raw_parts(params.importance_map, params.importance_map_len) };
        image
            .set_importance_map(map.to_vec())
            .map_err(|_| QuantizeError::Generic)?;
    }

    if !params.fixed_colors.is_null() && params.fixed_colors_len > 0 {
        let fixed = unsafe { slice::from_raw_parts(params.fixed_colors, params.fixed_colors_len) };
        for color in fixed {
            let iq_color = IqRGBA {
                r: color.r,
                g: color.g,
                b: color.b,
                a: color.a,
            };
            if image.add_fixed_color(iq_color).is_err() {
                return Err(QuantizeError::Generic);
            }
        }
    }

    let mut result = match attr.quantize(&mut image) {
        Ok(res) => res,
        Err(IqError::QualityTooLow) => return Err(QuantizeError::QualityTooLow),
        Err(_) => return Err(QuantizeError::Generic),
    };

    let quality = result
        .quantization_quality()
        .map(|q| i32::from(q))
        .unwrap_or(-1);

    if !params.remap {
        return Ok(QuantizeOutcome {
            palette: None,
            indices: None,
            quality,
        });
    }

    if params.dithering_level >= 0.0 && params.dithering_level.is_finite() {
        let dithering_level = if cfg!(pngx_bridge_msan) {
            0.0
        } else {
            params.dithering_level
        };
        if result.set_dithering_level(dithering_level).is_err() {
            return Err(QuantizeError::Generic);
        }
    }

    let (palette, indices) = result
        .remapped(&mut image)
        .map_err(|_| QuantizeError::Generic)?;
    let palette_rgba = convert_palette(&palette);

    Ok(QuantizeOutcome {
        palette: Some(palette_rgba),
        indices: Some(indices),
        quality,
    })
}

#[no_mangle]
pub unsafe extern "C" fn pngx_bridge_optimize_lossless(
    input_data: *const u8,
    input_size: usize,
    output_data: *mut *mut u8,
    output_size: *mut usize,
    options: *const PngxBridgeLosslessOptions,
) -> PngxResult {
    if input_data.is_null() || output_data.is_null() || output_size.is_null() {
        return PngxResult::InvalidInput;
    }

    let input_slice = slice::from_raw_parts(input_data, input_size);
    let default_opts = PngxBridgeLosslessOptions {
        optimization_level: 5,
        strip_safe: true,
        optimize_alpha: true,
    };
    let opts_ref = if options.is_null() {
        &default_opts
    } else {
        &*options
    };

    let rust_opts = convert_lossless_options(opts_ref);
    let attempt = catch_unwind(AssertUnwindSafe(|| {
        oxipng::optimize_from_memory(input_slice, &rust_opts)
    }));
    let result = match attempt {
        Ok(Ok(output_vec)) => output_vec,
        Ok(Err(_)) | Err(_) => input_slice.to_vec(),
    };

    let len = result.len();
    if len == 0 {
        *output_data = ptr::null_mut();
        *output_size = 0;
        return PngxResult::Success;
    }

    let buffer = unsafe { libc::malloc(len) as *mut u8 };
    if buffer.is_null() {
        return PngxResult::IoError;
    }

    unsafe {
        ptr::copy_nonoverlapping(result.as_ptr(), buffer, len);
    }
    *output_data = buffer;
    *output_size = len;
    PngxResult::Success
}

#[no_mangle]
pub unsafe extern "C" fn pngx_bridge_quantize(
    pixels: *const RgbaColor,
    pixel_count: usize,
    width: u32,
    height: u32,
    params: *const PngxBridgeQuantParams,
    output: *mut PngxBridgeQuantOutput,
) -> PngxBridgeQuantStatus {
    if pixels.is_null() || params.is_null() || output.is_null() {
        return PngxBridgeQuantStatus::Error;
    }

    let mut out = PngxBridgeQuantOutput {
        palette: ptr::null_mut(),
        palette_len: 0,
        indices: ptr::null_mut(),
        indices_len: 0,
        quality: -1,
    };

    let params_ref = &*params;
    let expected = (width as usize).checked_mul(height as usize);
    if expected != Some(pixel_count) {
        unsafe {
            *output = out;
        }
        return PngxBridgeQuantStatus::Error;
    }

    let pixels_slice = slice::from_raw_parts(pixels, pixel_count);

    let outcome = catch_unwind(AssertUnwindSafe(|| {
        quantize_image(pixels_slice, width as usize, height as usize, params_ref)
    }));

    let outcome = match outcome {
        Ok(result) => result,
        Err(_) => {
            unsafe {
                *output = out;
            }
            return PngxBridgeQuantStatus::Error;
        }
    };

    match outcome {
        Ok(result) => {
            out.quality = result.quality;
            if params_ref.remap {
                if let Some(palette) = result.palette {
                    match allocate_copy(&palette) {
                        Ok(ptr_palette) => {
                            out.palette = ptr_palette;
                            out.palette_len = palette.len();
                        }
                        Err(_) => {
                            unsafe {
                                *output = out;
                            }
                            return PngxBridgeQuantStatus::Error;
                        }
                    }
                }
                if let Some(indices) = result.indices {
                    match allocate_copy(&indices) {
                        Ok(ptr_indices) => {
                            out.indices = ptr_indices;
                            out.indices_len = indices.len();
                        }
                        Err(_) => {
                            if !out.palette.is_null() {
                                unsafe {
                                    libc::free(out.palette as *mut libc::c_void);
                                }
                                out.palette = ptr::null_mut();
                                out.palette_len = 0;
                            }
                            unsafe {
                                *output = out;
                            }
                            return PngxBridgeQuantStatus::Error;
                        }
                    }
                }
            }
            unsafe {
                *output = out;
            }
            PngxBridgeQuantStatus::Ok
        }
        Err(QuantizeError::QualityTooLow) => {
            unsafe {
                *output = out;
            }
            PngxBridgeQuantStatus::QualityTooLow
        }
        Err(QuantizeError::Generic) => {
            unsafe {
                *output = out;
            }
            PngxBridgeQuantStatus::Error
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn pngx_bridge_free(ptr: *mut u8) {
    if !ptr.is_null() {
        libc::free(ptr as *mut libc::c_void);
    }
}

macro_rules! parse_version_env {
    ($name:expr, $default:expr) => {{
        const S: &str = match option_env!($name) {
            Some(s) => s,
            None => "",
        };
        const fn parse(s: &str) -> u32 {
            let bytes = s.as_bytes();
            if bytes.is_empty() {
                return $default;
            }
            let mut i = 0;
            let mut result: u32 = 0;
            while i < bytes.len() {
                let b = bytes[i];
                if b < b'0' || b > b'9' {
                    return $default;
                }
                result = result * 10 + (b - b'0') as u32;
                i += 1;
            }
            result
        }
        parse(S)
    }};
}

#[no_mangle]
pub extern "C" fn pngx_bridge_oxipng_version() -> u32 {
    parse_version_env!("PNGX_BRIDGE_OXIPNG_VERSION", 0)
}

#[no_mangle]
pub extern "C" fn pngx_bridge_libimagequant_version() -> u32 {
    parse_version_env!("PNGX_BRIDGE_IMAGEQUANT_VERSION", 0)
}

#[no_mangle]
pub extern "C" fn pngx_bridge_init_threads(num_threads: c_int) -> bool {
    #[cfg(not(target_family = "wasm"))]
    {
        let mut success = false;
        RAYON_INIT.call_once(|| {
            let builder = if num_threads > 0 {
                rayon::ThreadPoolBuilder::new().num_threads(num_threads as usize)
            } else {
                rayon::ThreadPoolBuilder::new()
            };

            if builder.build_global().is_ok() {
                success = true;
            }
        });

        success || rayon::current_num_threads() > 0
    }

    #[cfg(target_family = "wasm")]
    {
        let _ = num_threads;
        false
    }
}
