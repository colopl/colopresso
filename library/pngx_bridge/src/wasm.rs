/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2026 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

use wasm_bindgen::prelude::*;

use crate::{
    convert_lossless_options, quantize_image, PngxBridgeLosslessOptions, PngxBridgeQuantParams,
    QuantizeError, RgbaColor,
};

#[cfg(feature = "wasm-bindgen-rayon")]
mod wasm_c_alloc {
    use std::alloc::{alloc, alloc_zeroed, dealloc, realloc as std_realloc, Layout};

    const ALLOC_ALIGN: usize = 16;

    #[no_mangle]
    pub unsafe extern "C" fn malloc(size: usize) -> *mut u8 {
        if size == 0 {
            return std::ptr::null_mut();
        }

        let total_size = size + ALLOC_ALIGN;
        let layout = match Layout::from_size_align(total_size, ALLOC_ALIGN) {
            Ok(l) => l,
            Err(_) => return std::ptr::null_mut(),
        };

        let ptr = alloc(layout);
        if ptr.is_null() {
            return std::ptr::null_mut();
        }

        *(ptr as *mut usize) = size;
        ptr.add(ALLOC_ALIGN)
    }

    #[no_mangle]
    pub unsafe extern "C" fn calloc(nmemb: usize, size: usize) -> *mut u8 {
        let total = match nmemb.checked_mul(size) {
            Some(t) => t,
            None => return std::ptr::null_mut(),
        };

        if total == 0 {
            return std::ptr::null_mut();
        }

        let total_size = total + ALLOC_ALIGN;
        let layout = match Layout::from_size_align(total_size, ALLOC_ALIGN) {
            Ok(l) => l,
            Err(_) => return std::ptr::null_mut(),
        };

        let ptr = alloc_zeroed(layout);
        if ptr.is_null() {
            return std::ptr::null_mut();
        }

        *(ptr as *mut usize) = total;
        ptr.add(ALLOC_ALIGN)
    }

    #[no_mangle]
    pub unsafe extern "C" fn realloc(ptr: *mut u8, new_size: usize) -> *mut u8 {
        if ptr.is_null() {
            return malloc(new_size);
        }

        if new_size == 0 {
            free(ptr);
            return std::ptr::null_mut();
        }

        let real_ptr = ptr.sub(ALLOC_ALIGN);
        let old_size = *(real_ptr as *const usize);
        let old_total = old_size + ALLOC_ALIGN;
        let new_total = new_size + ALLOC_ALIGN;
        let old_layout = match Layout::from_size_align(old_total, ALLOC_ALIGN) {
            Ok(l) => l,
            Err(_) => return std::ptr::null_mut(),
        };

        let new_ptr = std_realloc(real_ptr, old_layout, new_total);
        if new_ptr.is_null() {
            return std::ptr::null_mut();
        }

        *(new_ptr as *mut usize) = new_size;
        new_ptr.add(ALLOC_ALIGN)
    }

    #[no_mangle]
    pub unsafe extern "C" fn free(ptr: *mut u8) {
        if ptr.is_null() {
            return;
        }

        let real_ptr = ptr.sub(ALLOC_ALIGN);
        let size = *(real_ptr as *const usize);
        let total_size = size + ALLOC_ALIGN;
        let layout = match Layout::from_size_align(total_size, ALLOC_ALIGN) {
            Ok(l) => l,
            Err(_) => return,
        };

        dealloc(real_ptr, layout);
    }
}

#[wasm_bindgen]
pub fn pngx_wasm_is_threads_enabled() -> bool {
    cfg!(feature = "wasm-bindgen-rayon")
}

thread_local! {
    static LAST_OXIPNG_ERROR: std::cell::RefCell<String> = const { std::cell::RefCell::new(String::new()) };
}

fn set_last_oxipng_error(message: String) {
    LAST_OXIPNG_ERROR.with(|slot| {
        *slot.borrow_mut() = message;
    });
}

fn clear_last_oxipng_error() {
    set_last_oxipng_error(String::new());
}

#[wasm_bindgen]
pub struct WasmLosslessOptions {
    optimization_level: u8,
    strip_safe: bool,
    optimize_alpha: bool,
}

#[wasm_bindgen]
impl WasmLosslessOptions {
    #[wasm_bindgen(constructor)]
    pub fn new() -> Self {
        Self {
            optimization_level: 5,
            strip_safe: true,
            optimize_alpha: true,
        }
    }

    #[wasm_bindgen(setter)]
    pub fn set_optimization_level(&mut self, level: u8) {
        self.optimization_level = level;
    }

    #[wasm_bindgen(setter)]
    pub fn set_strip_safe(&mut self, strip: bool) {
        self.strip_safe = strip;
    }

    #[wasm_bindgen(setter)]
    pub fn set_optimize_alpha(&mut self, optimize: bool) {
        self.optimize_alpha = optimize;
    }
}

impl Default for WasmLosslessOptions {
    fn default() -> Self {
        Self::new()
    }
}

#[wasm_bindgen]
pub struct WasmQuantParams {
    speed: i32,
    quality_min: u8,
    quality_max: u8,
    max_colors: u32,
    min_posterization: i32,
    dithering_level: f32,
    remap: bool,
}

#[wasm_bindgen]
impl WasmQuantParams {
    #[wasm_bindgen(constructor)]
    pub fn new() -> Self {
        Self {
            speed: 3,
            quality_min: 0,
            quality_max: 100,
            max_colors: 256,
            min_posterization: 0,
            dithering_level: 1.0,
            remap: true,
        }
    }

    #[wasm_bindgen(setter)]
    pub fn set_speed(&mut self, speed: i32) {
        self.speed = speed;
    }

    #[wasm_bindgen(setter)]
    pub fn set_quality_min(&mut self, min: u8) {
        self.quality_min = min;
    }

    #[wasm_bindgen(setter)]
    pub fn set_quality_max(&mut self, max: u8) {
        self.quality_max = max;
    }

    #[wasm_bindgen(setter)]
    pub fn set_max_colors(&mut self, colors: u32) {
        self.max_colors = colors;
    }

    #[wasm_bindgen(setter)]
    pub fn set_min_posterization(&mut self, posterization: i32) {
        self.min_posterization = posterization;
    }

    #[wasm_bindgen(setter)]
    pub fn set_dithering_level(&mut self, level: f32) {
        self.dithering_level = level;
    }

    #[wasm_bindgen(setter)]
    pub fn set_remap(&mut self, remap: bool) {
        self.remap = remap;
    }
}

impl Default for WasmQuantParams {
    fn default() -> Self {
        Self::new()
    }
}

#[wasm_bindgen]
pub struct WasmQuantResult {
    palette: Vec<u8>,
    indices: Vec<u8>,
    quality: i32,
    status: i32,
}

#[wasm_bindgen]
impl WasmQuantResult {
    #[wasm_bindgen(getter)]
    pub fn palette(&self) -> Vec<u8> {
        self.palette.clone()
    }

    #[wasm_bindgen(getter)]
    pub fn indices(&self) -> Vec<u8> {
        self.indices.clone()
    }

    #[wasm_bindgen(getter)]
    pub fn quality(&self) -> i32 {
        self.quality
    }

    #[wasm_bindgen(getter)]
    pub fn status(&self) -> i32 {
        self.status
    }
}

#[wasm_bindgen]
pub fn pngx_wasm_optimize_lossless(
    input_data: &[u8],
    options: Option<WasmLosslessOptions>,
) -> Result<Vec<u8>, JsError> {
    let opts = options.unwrap_or_default();
    let bridge_opts = PngxBridgeLosslessOptions {
        optimization_level: opts.optimization_level,
        strip_safe: opts.strip_safe,
        optimize_alpha: opts.optimize_alpha,
    };

    let rust_opts = convert_lossless_options(&bridge_opts);

    match oxipng::optimize_from_memory(input_data, &rust_opts) {
        Ok(output) => {
            clear_last_oxipng_error();
            Ok(output)
        }
        Err(err) => {
            set_last_oxipng_error(format!("{err:?}"));
            Ok(input_data.to_vec())
        }
    }
}

#[wasm_bindgen]
pub fn pngx_wasm_last_oxipng_error() -> String {
    LAST_OXIPNG_ERROR.with(|slot| slot.borrow().clone())
}

#[wasm_bindgen]
pub fn pngx_wasm_quantize(
    pixels: &[u8],
    width: u32,
    height: u32,
    params: Option<WasmQuantParams>,
) -> WasmQuantResult {
    let pixel_count = (width as usize) * (height as usize);
    let expected_len = pixel_count * 4;
    if width == 0 || height == 0 {
        return WasmQuantResult {
            palette: Vec::new(),
            indices: Vec::new(),
            quality: -1,
            status: 4,
        };
    }

    if pixels.len() != expected_len {
        return WasmQuantResult {
            palette: Vec::new(),
            indices: Vec::new(),
            quality: (pixels.len() as i32),
            status: 3,
        };
    }

    let rgba_pixels: Vec<RgbaColor> = pixels
        .chunks_exact(4)
        .map(|c| RgbaColor {
            r: c[0],
            g: c[1],
            b: c[2],
            a: c[3],
        })
        .collect();

    let p = params.unwrap_or_default();
    let bridge_params = PngxBridgeQuantParams {
        speed: p.speed,
        quality_min: p.quality_min,
        quality_max: p.quality_max,
        max_colors: p.max_colors,
        min_posterization: p.min_posterization,
        dithering_level: p.dithering_level,
        importance_map: std::ptr::null(),
        importance_map_len: 0,
        fixed_colors: std::ptr::null(),
        fixed_colors_len: 0,
        remap: p.remap,
    };

    match quantize_image(
        &rgba_pixels,
        width as usize,
        height as usize,
        &bridge_params,
    ) {
        Ok(outcome) => {
            let palette_bytes: Vec<u8> = outcome
                .palette
                .unwrap_or_default()
                .iter()
                .flat_map(|c| [c.r, c.g, c.b, c.a])
                .collect();
            let indices = outcome.indices.unwrap_or_default();

            WasmQuantResult {
                palette: palette_bytes,
                indices,
                quality: outcome.quality,
                status: 0,
            }
        }
        Err(QuantizeError::QualityTooLow) => WasmQuantResult {
            palette: Vec::new(),
            indices: Vec::new(),
            quality: -1,
            status: 1,
        },
        Err(QuantizeError::Generic) => WasmQuantResult {
            palette: Vec::new(),
            indices: Vec::new(),
            quality: -1,
            status: 2,
        },
    }
}

#[wasm_bindgen]
pub fn pngx_wasm_quantize_advanced(
    pixels: &[u8],
    width: u32,
    height: u32,
    speed: i32,
    quality_min: u8,
    quality_max: u8,
    max_colors: u32,
    min_posterization: i32,
    dithering_level: f32,
    remap: bool,
    importance_map: Option<Vec<u8>>,
    fixed_colors: Option<Vec<u8>>,
) -> WasmQuantResult {
    let pixel_count = (width as usize) * (height as usize);

    if pixels.len() != pixel_count * 4 {
        return WasmQuantResult {
            palette: Vec::new(),
            indices: Vec::new(),
            quality: -1,
            status: 2,
        };
    }

    let rgba_pixels: Vec<RgbaColor> = pixels
        .chunks_exact(4)
        .map(|c| RgbaColor {
            r: c[0],
            g: c[1],
            b: c[2],
            a: c[3],
        })
        .collect();

    let imp_map = importance_map.unwrap_or_default();

    let imp_ptr = if imp_map.len() == pixel_count {
        imp_map.as_ptr()
    } else {
        std::ptr::null()
    };

    let imp_len = if imp_map.len() == pixel_count {
        imp_map.len()
    } else {
        0
    };

    let fixed = fixed_colors.unwrap_or_default();
    let fixed_rgba: Vec<RgbaColor> = fixed
        .chunks_exact(4)
        .map(|c| RgbaColor {
            r: c[0],
            g: c[1],
            b: c[2],
            a: c[3],
        })
        .collect();
    let fixed_ptr = if fixed_rgba.is_empty() {
        std::ptr::null()
    } else {
        fixed_rgba.as_ptr()
    };
    let fixed_len = fixed_rgba.len();

    let bridge_params = PngxBridgeQuantParams {
        speed,
        quality_min,
        quality_max,
        max_colors,
        min_posterization,
        dithering_level,
        importance_map: imp_ptr,
        importance_map_len: imp_len,
        fixed_colors: fixed_ptr,
        fixed_colors_len: fixed_len,
        remap,
    };

    match quantize_image(
        &rgba_pixels,
        width as usize,
        height as usize,
        &bridge_params,
    ) {
        Ok(outcome) => {
            let palette_bytes: Vec<u8> = outcome
                .palette
                .unwrap_or_default()
                .iter()
                .flat_map(|c| [c.r, c.g, c.b, c.a])
                .collect();
            let indices = outcome.indices.unwrap_or_default();

            WasmQuantResult {
                palette: palette_bytes,
                indices,
                quality: outcome.quality,
                status: 0,
            }
        }
        Err(QuantizeError::QualityTooLow) => WasmQuantResult {
            palette: Vec::new(),
            indices: Vec::new(),
            quality: -1,
            status: 1,
        },
        Err(QuantizeError::Generic) => WasmQuantResult {
            palette: Vec::new(),
            indices: Vec::new(),
            quality: -1,
            status: 2,
        },
    }
}

#[wasm_bindgen]
pub fn pngx_wasm_oxipng_version() -> u32 {
    crate::pngx_bridge_oxipng_version()
}

#[wasm_bindgen]
pub fn pngx_wasm_libimagequant_version() -> u32 {
    crate::pngx_bridge_libimagequant_version()
}

#[wasm_bindgen]
pub fn pngx_wasm_rust_version_string() -> String {
    let ptr = crate::pngx_bridge_rust_version_string();
    if ptr.is_null() {
        return String::from("unknown");
    }
    unsafe {
        std::ffi::CStr::from_ptr(ptr)
            .to_str()
            .unwrap_or("unknown")
            .to_string()
    }
}
