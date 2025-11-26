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

import { ColopressoModule, FormatOptions } from '../types';

const PNGX_RGBA_LOSSY_TYPES = new Set([1, 2]);
const CPRES_ERROR_OUTPUT_NOT_SMALLER = 9;

interface ConfigInstance {
  configPtr: number;
  protectedColorsPtr: number | null;
}

interface ProtectedColor {
  r: number;
  g: number;
  b: number;
  a: number;
}

export type ModuleWithHelpers = ColopressoModule & {
  HEAPU8: Uint8Array;
  getValue(ptr: number, type: 'i32'): number;
  _malloc(size: number): number;
  _free(ptr: number): void;
  _cpres_free(ptr: number): void;
  UTF8ToString?(ptr: number): string;
  _emscripten_get_last_error?(): number;
  _emscripten_get_last_webp_error?(): number;
  _emscripten_get_last_avif_error?(): number;
  _emscripten_config_create(): number;
  _emscripten_config_free(ptr: number): void;
  _emscripten_config_pngx_protected_colors(ptr: number, colorsPtr: number, count: number): void;
  _emscripten_config_webp_quality?(configPtr: number, value: number): void;
  _emscripten_config_webp_lossless?(configPtr: number, value: number): void;
  _emscripten_config_webp_method?(configPtr: number, value: number): void;
  _emscripten_config_webp_target_size?(configPtr: number, value: number): void;
  _emscripten_config_webp_target_psnr?(configPtr: number, value: number): void;
  _emscripten_config_webp_segments?(configPtr: number, value: number): void;
  _emscripten_config_webp_sns_strength?(configPtr: number, value: number): void;
  _emscripten_config_webp_filter_strength?(configPtr: number, value: number): void;
  _emscripten_config_webp_filter_sharpness?(configPtr: number, value: number): void;
  _emscripten_config_webp_filter_type?(configPtr: number, value: number): void;
  _emscripten_config_webp_autofilter?(configPtr: number, value: number): void;
  _emscripten_config_webp_alpha_compression?(configPtr: number, value: number): void;
  _emscripten_config_webp_alpha_filtering?(configPtr: number, value: number): void;
  _emscripten_config_webp_alpha_quality?(configPtr: number, value: number): void;
  _emscripten_config_webp_pass?(configPtr: number, value: number): void;
  _emscripten_config_webp_preprocessing?(configPtr: number, value: number): void;
  _emscripten_config_webp_partitions?(configPtr: number, value: number): void;
  _emscripten_config_webp_partition_limit?(configPtr: number, value: number): void;
  _emscripten_config_webp_emulate_jpeg_size?(configPtr: number, value: number): void;
  _emscripten_config_webp_low_memory?(configPtr: number, value: number): void;
  _emscripten_config_webp_near_lossless?(configPtr: number, value: number): void;
  _emscripten_config_webp_exact?(configPtr: number, value: number): void;
  _emscripten_config_webp_use_delta_palette?(configPtr: number, value: number): void;
  _emscripten_config_webp_use_sharp_yuv?(configPtr: number, value: number): void;
  _emscripten_config_avif_quality?(configPtr: number, value: number): void;
  _emscripten_config_avif_alpha_quality?(configPtr: number, value: number): void;
  _emscripten_config_avif_lossless?(configPtr: number, value: number): void;
  _emscripten_config_avif_speed?(configPtr: number, value: number): void;
  _emscripten_config_pngx_level?(configPtr: number, value: number): void;
  _emscripten_config_pngx_strip_safe?(configPtr: number, value: number): void;
  _emscripten_config_pngx_optimize_alpha?(configPtr: number, value: number): void;
  _emscripten_config_pngx_lossy_enable?(configPtr: number, value: number): void;
  _emscripten_config_pngx_lossy_type?(configPtr: number, value: number): void;
  _emscripten_config_pngx_lossy_max_colors?(configPtr: number, value: number): void;
  _emscripten_config_pngx_reduced_bits_rgb?(configPtr: number, value: number): void;
  _emscripten_config_pngx_reduced_alpha_bits?(configPtr: number, value: number): void;
  _emscripten_config_pngx_lossy_quality_min?(configPtr: number, value: number): void;
  _emscripten_config_pngx_lossy_quality_max?(configPtr: number, value: number): void;
  _emscripten_config_pngx_lossy_speed?(configPtr: number, value: number): void;
  _emscripten_config_pngx_lossy_dither_level?(configPtr: number, value: number): void;
  _emscripten_config_pngx_saliency_map_enable?(configPtr: number, value: number): void;
  _emscripten_config_pngx_chroma_anchor_enable?(configPtr: number, value: number): void;
  _emscripten_config_pngx_adaptive_dither_enable?(configPtr: number, value: number): void;
  _emscripten_config_pngx_gradient_boost_enable?(configPtr: number, value: number): void;
  _emscripten_config_pngx_chroma_weight_enable?(configPtr: number, value: number): void;
  _emscripten_config_pngx_postprocess_smooth_enable?(configPtr: number, value: number): void;
  _emscripten_config_pngx_postprocess_smooth_importance_cutoff?(configPtr: number, value: number): void;
  _emscripten_convert_png_to_webp?(pngPtr: number, pngSize: number, configPtr: number, outSizePtr: number): number;
  _emscripten_convert_png_to_avif?(pngPtr: number, pngSize: number, configPtr: number, outSizePtr: number): number;
  _emscripten_convert_png_to_pngx?(pngPtr: number, pngSize: number, configPtr: number, outSizePtr: number): number;
  _emscripten_get_error_string?(code: number): number;
  _emscripten_get_version?(): number;
  _emscripten_get_libwebp_version?(): number;
  _emscripten_get_libpng_version?(): number;
  _emscripten_get_libavif_version?(): number;
  _emscripten_get_pngx_oxipng_version?(): number;
  _emscripten_get_pngx_libimagequant_version?(): number;
  _emscripten_get_buildtime?(): number;
};

class OutputLargerThanInputError extends Error {
  public readonly code = 'output_larger_than_input';
  public readonly inputSize?: number;
  public readonly outputSize?: number;
  public readonly formatLabel: string;
  public readonly details: { inputSize?: number; outputSize?: number };

  constructor(formatLabel: string, inputSize?: number, outputSize?: number) {
    const describe = (value?: number) => (typeof value === 'number' && Number.isFinite(value) ? `${value} bytes` : 'unknown size');
    super(`${formatLabel} result is larger than input (input: ${describe(inputSize)}, output: ${describe(outputSize)})`);
    this.name = 'OutputLargerThanInputError';
    this.formatLabel = formatLabel;
    this.inputSize = inputSize;
    this.outputSize = outputSize;
    this.details = { inputSize, outputSize };
    Object.setPrototypeOf(this, new.target.prototype);
  }
}

class WasmAllocationError extends Error {
  public readonly code = 'wasm_allocation_failed';
  public readonly resource: string;

  constructor(resource: string) {
    super(`Failed to allocate ${resource} in WebAssembly heap`);
    this.name = 'WasmAllocationError';
    this.resource = resource;
    Object.setPrototypeOf(this, new.target.prototype);
  }
}

type ApplyFn = (configPtr: number, value: number) => void;

function createConfig(Module: ModuleWithHelpers, userConfig: FormatOptions = {}): ConfigInstance {
  const configPtr = Module._emscripten_config_create();
  if (!Number.isFinite(configPtr) || configPtr === 0) {
    throw new WasmAllocationError('encoder configuration');
  }
  const apply = (methodName: string, value: unknown, transform?: (v: unknown) => number) => {
    const fn = Module[methodName] as ApplyFn | undefined;
    if (!fn) {
      return;
    }
    if (value === undefined || value === null) {
      return;
    }
    const compute = transform ?? ((v: unknown) => (typeof v === 'boolean' ? (v ? 1 : 0) : (v as number)));
    fn(configPtr, compute(value));
  };

  apply('_emscripten_config_webp_quality', userConfig.quality as number | undefined);
  apply('_emscripten_config_webp_lossless', userConfig.lossless as boolean | undefined);
  apply('_emscripten_config_webp_method', userConfig.method as number | undefined);
  apply('_emscripten_config_webp_target_size', userConfig.target_size as number | undefined);
  apply('_emscripten_config_webp_target_psnr', userConfig.target_psnr as number | undefined);
  apply('_emscripten_config_webp_segments', userConfig.segments as number | undefined);
  apply('_emscripten_config_webp_sns_strength', userConfig.sns_strength as number | undefined);
  apply('_emscripten_config_webp_filter_strength', userConfig.filter_strength as number | undefined);
  apply('_emscripten_config_webp_filter_sharpness', userConfig.filter_sharpness as number | undefined);
  apply('_emscripten_config_webp_filter_type', userConfig.filter_type as number | undefined);
  apply('_emscripten_config_webp_autofilter', userConfig.autofilter as boolean | undefined);
  apply('_emscripten_config_webp_alpha_compression', userConfig.alpha_compression as boolean | undefined);
  apply('_emscripten_config_webp_alpha_filtering', userConfig.alpha_filtering as number | undefined);
  apply('_emscripten_config_webp_alpha_quality', userConfig.alpha_quality as number | undefined);
  apply('_emscripten_config_webp_pass', userConfig.pass as number | undefined);
  apply('_emscripten_config_webp_preprocessing', userConfig.preprocessing as number | undefined);
  apply('_emscripten_config_webp_partitions', userConfig.partitions as number | undefined);
  apply('_emscripten_config_webp_partition_limit', userConfig.partition_limit as number | undefined);
  apply('_emscripten_config_webp_emulate_jpeg_size', userConfig.emulate_jpeg_size as boolean | undefined);
  apply('_emscripten_config_webp_low_memory', userConfig.low_memory as boolean | undefined);
  apply('_emscripten_config_webp_near_lossless', userConfig.near_lossless as number | undefined);
  apply('_emscripten_config_webp_exact', userConfig.exact as boolean | undefined);
  apply('_emscripten_config_webp_use_delta_palette', userConfig.use_delta_palette as boolean | undefined);
  apply('_emscripten_config_webp_use_sharp_yuv', userConfig.use_sharp_yuv as boolean | undefined);

  apply('_emscripten_config_avif_quality', (userConfig.avif_quality ?? userConfig.quality) as number | undefined);
  apply('_emscripten_config_avif_alpha_quality', (userConfig.avif_alpha_quality ?? userConfig.alpha_quality) as number | undefined);
  apply('_emscripten_config_avif_lossless', (userConfig.avif_lossless ?? userConfig.lossless) as boolean | undefined);
  apply('_emscripten_config_avif_speed', (userConfig.avif_speed ?? userConfig.speed) as number | undefined);

  apply('_emscripten_config_pngx_level', (userConfig.pngx_level ?? userConfig.level) as number | undefined);
  apply('_emscripten_config_pngx_strip_safe', userConfig.pngx_strip_safe as boolean | undefined);
  apply('_emscripten_config_pngx_optimize_alpha', userConfig.pngx_optimize_alpha as boolean | undefined);
  const lossyTypeSelection = getNumeric(userConfig.pngx_lossy_type);
  const sharedMaxColors = getNumeric(userConfig.pngx_lossy_max_colors);
  const legacyReducedColors = getNumeric((userConfig as Record<string, unknown>)['pngx_lossy_reduced_colors']);
  const resolvedReducedColors = (() => {
    if (lossyTypeSelection === 2) {
      if (sharedMaxColors === undefined || sharedMaxColors <= 1) {
        return -1;
      }
      const manual = Math.trunc(sharedMaxColors);
      return Math.min(32768, Math.max(2, manual));
    }
    if (legacyReducedColors !== undefined) {
      return legacyReducedColors;
    }
    return -1;
  })();
  const ditherAutoSelected = lossyTypeSelection === 1 && isTruthy((userConfig as Record<string, unknown>)['pngx_lossy_dither_auto']);
  const rawDitherLevel = getNumeric(userConfig.pngx_lossy_dither_level);
  const resolvedDitherLevel = (() => {
    if (ditherAutoSelected) {
      return -1;
    }
    if (rawDitherLevel === undefined) {
      return undefined;
    }
    if (rawDitherLevel < 0) {
      return -1;
    }
    if (rawDitherLevel <= 1) {
      return Math.min(1, Math.max(0, rawDitherLevel));
    }
    return Math.min(1, Math.max(0, rawDitherLevel / 100));
  })();

  apply('_emscripten_config_pngx_lossy_enable', userConfig.pngx_lossy_enable as boolean | undefined);
  apply('_emscripten_config_pngx_lossy_type', lossyTypeSelection);
  apply('_emscripten_config_pngx_lossy_max_colors', sharedMaxColors);
  apply('_emscripten_config_pngx_reduced_colors', resolvedReducedColors);
  apply('_emscripten_config_pngx_reduced_bits_rgb', userConfig.pngx_lossy_reduced_bits_rgb as number | undefined);
  apply('_emscripten_config_pngx_reduced_alpha_bits', userConfig.pngx_lossy_reduced_alpha_bits as number | undefined);
  apply('_emscripten_config_pngx_lossy_quality_min', userConfig.pngx_lossy_quality_min as number | undefined);
  apply('_emscripten_config_pngx_lossy_quality_max', userConfig.pngx_lossy_quality_max as number | undefined);
  apply('_emscripten_config_pngx_lossy_speed', userConfig.pngx_lossy_speed as number | undefined);
  apply('_emscripten_config_pngx_lossy_dither_level', resolvedDitherLevel);
  apply('_emscripten_config_pngx_saliency_map_enable', userConfig.pngx_saliency_map_enable as boolean | undefined);
  apply('_emscripten_config_pngx_chroma_anchor_enable', userConfig.pngx_chroma_anchor_enable as boolean | undefined);
  apply('_emscripten_config_pngx_adaptive_dither_enable', userConfig.pngx_adaptive_dither_enable as boolean | undefined);
  apply('_emscripten_config_pngx_gradient_boost_enable', userConfig.pngx_gradient_boost_enable as boolean | undefined);
  apply('_emscripten_config_pngx_chroma_weight_enable', userConfig.pngx_chroma_weight_enable as boolean | undefined);
  apply('_emscripten_config_pngx_postprocess_smooth_enable', userConfig.pngx_postprocess_smooth_enable as boolean | undefined);
  apply('_emscripten_config_pngx_postprocess_smooth_importance_cutoff', userConfig.pngx_postprocess_smooth_importance_cutoff as number | undefined);

  let protectedColorsPtr: number | null = null;
  const protectedColors = userConfig.pngx_protected_colors as ProtectedColor[] | undefined;
  if (protectedColors && Array.isArray(protectedColors) && protectedColors.length > 0) {
    const bytesPerColor = 4;
    protectedColorsPtr = Module._malloc(protectedColors.length * bytesPerColor);
    if (!protectedColorsPtr) {
      Module._emscripten_config_free(configPtr);
      throw new WasmAllocationError('protected color buffer');
    }
    protectedColors.forEach((color, index) => {
      Module.HEAPU8[protectedColorsPtr! + index * bytesPerColor + 0] = color.r;
      Module.HEAPU8[protectedColorsPtr! + index * bytesPerColor + 1] = color.g;
      Module.HEAPU8[protectedColorsPtr! + index * bytesPerColor + 2] = color.b;
      Module.HEAPU8[protectedColorsPtr! + index * bytesPerColor + 3] = color.a;
    });
    const setter = Module._emscripten_config_pngx_protected_colors?.bind(Module) as ((configPtr: number, colorsPtr: number, count: number) => void) | undefined;
    setter?.(configPtr, protectedColorsPtr, protectedColors.length);
  }

  return { configPtr, protectedColorsPtr };
}

function freeConfig(Module: ModuleWithHelpers, instance: ConfigInstance): void {
  if (instance.protectedColorsPtr) {
    Module._free(instance.protectedColorsPtr);
  }
  Module._emscripten_config_free(instance.configPtr);
}

function isTruthy(value: unknown): boolean {
  if (typeof value === 'string') {
    if (value.toLowerCase() === 'true') {
      return true;
    }
    if (value.toLowerCase() === 'false') {
      return false;
    }
  }
  return Boolean(value);
}

function getNumeric(value: unknown): number | undefined {
  if (typeof value === 'number' && Number.isFinite(value)) {
    return value;
  }
  if (typeof value === 'string') {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : undefined;
  }
  return undefined;
}

function allowsPngxRgbaOverflow(options?: FormatOptions): boolean {
  if (!options) {
    return false;
  }
  const lossyEnabledRaw = options.pngx_lossy_enable;
  const lossyEnabled = lossyEnabledRaw === undefined ? true : isTruthy(lossyEnabledRaw);
  if (!lossyEnabled) {
    return false;
  }
  const lossyTypeValue = getNumeric(options.pngx_lossy_type);
  if (lossyTypeValue === undefined) {
    return false;
  }
  return PNGX_RGBA_LOSSY_TYPES.has(lossyTypeValue);
}

interface ExecuteConversionOptions {
  Module: ModuleWithHelpers;
  pngData: Uint8Array;
  buildConfig: (Module: ModuleWithHelpers) => ConfigInstance;
  invokeConversion: (Module: ModuleWithHelpers, pngPtr: number, pngSize: number, configPtr: number, outSizePtr: number) => number;
  formatLabel: string;
  describeFailure?: (Module: ModuleWithHelpers) => string;
  enforceSmallerOutput?: boolean;
}

function executeConversion(options: ExecuteConversionOptions): Uint8Array {
  const { Module, pngData, buildConfig, invokeConversion, formatLabel, describeFailure, enforceSmallerOutput = false } = options;
  const pngSize = pngData.length;
  const pngPtr = Module._malloc(pngSize);
  if (!pngPtr) {
    throw new WasmAllocationError('input PNG buffer');
  }
  const outSizePtr = Module._malloc(4);
  if (!outSizePtr) {
    Module._free(pngPtr);
    throw new WasmAllocationError('output size buffer');
  }
  let configInstance: ConfigInstance | null = null;

  try {
    configInstance = buildConfig(Module);
    Module.HEAPU8.set(pngData, pngPtr);

    const resultPtr = invokeConversion(Module, pngPtr, pngSize, configInstance.configPtr, outSizePtr);
    const resultSize = Module.getValue(outSizePtr, 'i32');

    if (!resultPtr || resultPtr === 0 || resultSize <= 0) {
      if (resultPtr) {
        Module._cpres_free(resultPtr);
      }
      const lastError = Module._emscripten_get_last_error ? Module._emscripten_get_last_error() : 0;
      if (lastError === CPRES_ERROR_OUTPUT_NOT_SMALLER) {
        throw new OutputLargerThanInputError(formatLabel, pngSize, resultSize > 0 ? resultSize : undefined);
      }
      throw new Error(describeFailure ? describeFailure(Module) : `${formatLabel} conversion failed`);
    }

    if (enforceSmallerOutput && resultSize > pngSize) {
      Module._cpres_free(resultPtr);
      throw new OutputLargerThanInputError(formatLabel, pngSize, resultSize);
    }

    const resultData = Module.HEAPU8.slice(resultPtr, resultPtr + resultSize);
    Module._cpres_free(resultPtr);
    return resultData;
  } finally {
    Module._free(pngPtr);
    Module._free(outSizePtr);
    if (configInstance) {
      freeConfig(Module, configInstance);
    }
  }
}

export async function initializeModule(factory: (opts?: Record<string, unknown>) => Promise<ColopressoModule>, options: Record<string, unknown> = {}): Promise<ModuleWithHelpers> {
  const Module = (await factory(options)) as ModuleWithHelpers;
  return Module;
}

export function convertPngToWebp(Module: ColopressoModule, pngData: Uint8Array, userConfig: FormatOptions = {}): Uint8Array {
  const mod = Module as ModuleWithHelpers;
  return executeConversion({
    Module: mod,
    pngData,
    buildConfig: () => createConfig(mod, userConfig),
    invokeConversion: (m, pngPtr, pngSize, configPtr, outSizePtr) => m._emscripten_convert_png_to_webp?.(pngPtr, pngSize, configPtr, outSizePtr) ?? 0,
    formatLabel: 'WebP',
    describeFailure: (m) => {
      const code = m._emscripten_get_last_error ? m._emscripten_get_last_error() : 0;
      const webp = m._emscripten_get_last_webp_error ? m._emscripten_get_last_webp_error() : 0;
      const messagePtr = m._emscripten_get_error_string ? m._emscripten_get_error_string(code) : 0;
      const message = messagePtr && m.UTF8ToString ? m.UTF8ToString(messagePtr) : 'Conversion failed';
      return `Conversion failed: ${message} (cpres: ${code}, webp: ${webp})`;
    },
    enforceSmallerOutput: true,
  });
}

export function convertPngToAvif(Module: ColopressoModule, pngData: Uint8Array, userConfig: FormatOptions = {}): Uint8Array {
  const mod = Module as ModuleWithHelpers;
  return executeConversion({
    Module: mod,
    pngData,
    buildConfig: () =>
      createConfig(mod, {
        avif_quality: userConfig.quality,
        avif_alpha_quality: userConfig.alpha_quality,
        avif_lossless: userConfig.lossless,
        avif_speed: userConfig.speed,
      }),
    invokeConversion: (m, pngPtr, pngSize, configPtr, outSizePtr) => m._emscripten_convert_png_to_avif?.(pngPtr, pngSize, configPtr, outSizePtr) ?? 0,
    formatLabel: 'AVIF',
    describeFailure: (m) => {
      const code = m._emscripten_get_last_error ? m._emscripten_get_last_error() : 0;
      const avifError = m._emscripten_get_last_avif_error ? m._emscripten_get_last_avif_error() : 0;
      return `Conversion failed (cpres: ${code}, avif: ${avifError})`;
    },
    enforceSmallerOutput: true,
  });
}

export function convertPngToPngx(Module: ColopressoModule, pngData: Uint8Array, userConfig: FormatOptions = {}): Uint8Array {
  const mod = Module as ModuleWithHelpers;
  const allowRgbaOverflow = allowsPngxRgbaOverflow(userConfig);
  return executeConversion({
    Module: mod,
    pngData,
    buildConfig: () => createConfig(mod, userConfig),
    invokeConversion: (m, pngPtr, pngSize, configPtr, outSizePtr) => m._emscripten_convert_png_to_pngx?.(pngPtr, pngSize, configPtr, outSizePtr) ?? 0,
    formatLabel: 'PNGX',
    describeFailure: (m) => {
      const code = m._emscripten_get_last_error ? m._emscripten_get_last_error() : 0;
      return `PNGX conversion failed (cpres: ${code})`;
    },
    enforceSmallerOutput: !allowRgbaOverflow,
  });
}

export function getVersionInfo(Module: ColopressoModule) {
  const mod = Module as ModuleWithHelpers;
  return {
    version: mod._emscripten_get_version ? mod._emscripten_get_version() : undefined,
    libwebpVersion: mod._emscripten_get_libwebp_version ? mod._emscripten_get_libwebp_version() : undefined,
    libpngVersion: mod._emscripten_get_libpng_version ? mod._emscripten_get_libpng_version() : undefined,
    libavifVersion: mod._emscripten_get_libavif_version ? mod._emscripten_get_libavif_version() : undefined,
    pngxOxipngVersion: mod._emscripten_get_pngx_oxipng_version ? mod._emscripten_get_pngx_oxipng_version() : undefined,
    pngxLibimagequantVersion: mod._emscripten_get_pngx_libimagequant_version ? mod._emscripten_get_pngx_libimagequant_version() : undefined,
    buildtime: mod._emscripten_get_buildtime ? mod._emscripten_get_buildtime() : undefined,
  };
}
