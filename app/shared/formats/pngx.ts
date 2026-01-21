/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2025-2026 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

import { convertPngToPngx, pngxPalette256Prepare, pngxPalette256Finalize, pngxPalette256Cleanup, OutputLargerThanInputError } from '../core/converter';
import { isPngxBridgeInitialized, pngxOptimizeLossless, pngxQuantize } from '../core/pngxBridge';
import { FormatDefinition, FormatOptions, FormatSection } from '../types';
import { buildDefaultOptions, normalizeOptionsWithSchema } from './helpers';

function clampNumber(value: number, min: number, max: number): number {
  if (!Number.isFinite(value)) {
    return min;
  }

  return Math.min(max, Math.max(min, value));
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

const sections: FormatSection[] = [
  {
    section: 'basic',
    labelKey: 'formats.common.sections.basic',
    fields: [
      { id: 'pngx_level', type: 'number', min: 0, max: 6, defaultValue: 5, labelKey: 'formats.pngx.fields.level' },
      { id: 'pngx_strip_safe', type: 'checkbox', defaultValue: true, labelKey: 'formats.pngx.fields.strip_safe' },
      { id: 'pngx_optimize_alpha', type: 'checkbox', defaultValue: true, labelKey: 'formats.pngx.fields.optimize_alpha' },
      {
        id: 'pngx_threads',
        type: 'number',
        min: 0,
        max: 64,
        maxIsHardwareConcurrency: true,
        defaultValue: 0,
        labelKey: 'formats.pngx.fields.threads',
        noteKey: 'settingsModal.notes.pngxThreads',
        requiresThreads: true,
        excludeFromExport: true,
        requiresReload: true,
      },
    ],
  },
  {
    section: 'lossy',
    labelKey: 'formats.common.sections.lossy',
    fields: [
      { id: 'pngx_lossy_enable', type: 'checkbox', defaultValue: true, labelKey: 'formats.pngx.fields.lossy_enable' },
      {
        id: 'pngx_lossy_type',
        type: 'select',
        defaultValue: 0,
        labelKey: 'formats.pngx.fields.lossy_type',
        options: [
          { value: 0, labelKey: 'formats.pngx.options.lossy_type.palette256' },
          { value: 1, labelKey: 'formats.pngx.options.lossy_type.limited_rgba16bit' },
          { value: 2, labelKey: 'formats.pngx.options.lossy_type.reduced_rgba32' },
        ],
      },
      {
        id: 'pngx_lossy_max_colors',
        type: 'number',
        min: 1,
        max: 32768,
        defaultValue: 256,
        labelKey: 'formats.pngx.fields.lossy_max_colors',
        noteKey: 'settingsModal.notes.pngxMaxColorsPalette',
      },
      {
        id: 'pngx_lossy_reduced_bits_rgb',
        type: 'number',
        min: 1,
        max: 8,
        defaultValue: 4,
        labelKey: 'formats.pngx.fields.lossy_reduced_bits_rgb',
        noteKey: 'settingsModal.notes.pngxReducedBitsRgb',
      },
      {
        id: 'pngx_lossy_reduced_alpha_bits',
        type: 'number',
        min: 1,
        max: 8,
        defaultValue: 4,
        labelKey: 'formats.pngx.fields.lossy_reduced_alpha_bits',
        noteKey: 'settingsModal.notes.pngxReducedAlphaBits',
      },
      { id: 'pngx_lossy_quality_min', type: 'number', min: 0, max: 100, defaultValue: 80, labelKey: 'formats.pngx.fields.lossy_quality_min' },
      { id: 'pngx_lossy_quality_max', type: 'number', min: 0, max: 100, defaultValue: 95, labelKey: 'formats.pngx.fields.lossy_quality_max' },
      { id: 'pngx_lossy_speed', type: 'number', min: 1, max: 10, defaultValue: 3, labelKey: 'formats.pngx.fields.lossy_speed' },
      {
        id: 'pngx_lossy_dither_level',
        type: 'range',
        min: 0,
        max: 100,
        step: 1,
        defaultValue: 60,
        labelKey: 'formats.pngx.fields.lossy_dither_level',
        noteKey: 'settingsModal.notes.pngxDitherDefault',
      },
      { id: 'pngx_lossy_dither_auto', type: 'checkbox', defaultValue: false, labelKey: 'formats.pngx.fields.lossy_dither_auto' },
      { id: 'pngx_saliency_map_enable', type: 'checkbox', defaultValue: true, labelKey: 'formats.pngx.fields.saliency_map' },
      { id: 'pngx_chroma_anchor_enable', type: 'checkbox', defaultValue: true, labelKey: 'formats.pngx.fields.chroma_anchor' },
      { id: 'pngx_adaptive_dither_enable', type: 'checkbox', defaultValue: true, labelKey: 'formats.pngx.fields.adaptive_dither' },
      { id: 'pngx_gradient_boost_enable', type: 'checkbox', defaultValue: true, labelKey: 'formats.pngx.fields.gradient_boost' },
      { id: 'pngx_chroma_weight_enable', type: 'checkbox', defaultValue: true, labelKey: 'formats.pngx.fields.chroma_weight' },
      { id: 'pngx_postprocess_smooth_enable', type: 'checkbox', defaultValue: true, labelKey: 'formats.pngx.fields.postprocess_smooth' },
      {
        id: 'pngx_postprocess_smooth_importance_cutoff',
        type: 'number',
        min: -1,
        max: 1,
        step: 0.05,
        defaultValue: 0.6,
        labelKey: 'formats.pngx.fields.postprocess_smooth_cutoff',
      },
      { id: 'pngx_protected_colors', type: 'color-array', defaultValue: [], labelKey: 'formats.pngx.fields.protected_colors' },
    ],
  },
  {
    section: 'palette256',
    labelKey: 'formats.pngx.sections.palette256',
    fields: [
      {
        id: 'pngx_palette256_gradient_profile_enable',
        type: 'checkbox',
        defaultValue: true,
        labelKey: 'formats.pngx.fields.palette256_gradient_profile_enable',
      },
      {
        id: 'pngx_palette256_gradient_dither_floor',
        type: 'number',
        min: -1,
        max: 1,
        step: 0.01,
        defaultValue: 0.78,
        labelKey: 'formats.pngx.fields.palette256_gradient_dither_floor',
        noteKey: 'settingsModal.notes.pngxPalette256GradientDitherFloor',
      },
      {
        id: 'pngx_palette256_alpha_bleed_enable',
        type: 'checkbox',
        defaultValue: false,
        labelKey: 'formats.pngx.fields.palette256_alpha_bleed_enable',
        noteKey: 'settingsModal.notes.pngxPalette256AlphaBleed',
      },
      {
        id: 'pngx_palette256_alpha_bleed_max_distance',
        type: 'number',
        min: 0,
        max: 65535,
        defaultValue: 64,
        labelKey: 'formats.pngx.fields.palette256_alpha_bleed_max_distance',
        noteKey: 'settingsModal.notes.pngxPalette256AlphaBleed',
      },
      {
        id: 'pngx_palette256_alpha_bleed_opaque_threshold',
        type: 'number',
        min: 0,
        max: 255,
        defaultValue: 248,
        labelKey: 'formats.pngx.fields.palette256_alpha_bleed_opaque_threshold',
        noteKey: 'settingsModal.notes.pngxPalette256AlphaBleed',
      },
      {
        id: 'pngx_palette256_alpha_bleed_soft_limit',
        type: 'number',
        min: 0,
        max: 255,
        defaultValue: 160,
        labelKey: 'formats.pngx.fields.palette256_alpha_bleed_soft_limit',
        noteKey: 'settingsModal.notes.pngxPalette256AlphaBleed',
      },
      {
        id: 'pngx_palette256_profile_opaque_ratio_threshold',
        type: 'number',
        min: -1,
        max: 1,
        step: 0.01,
        defaultValue: 0.9,
        labelKey: 'formats.pngx.fields.palette256_profile_opaque_ratio_threshold',
      },
      {
        id: 'pngx_palette256_profile_gradient_mean_max',
        type: 'number',
        min: -1,
        max: 1,
        step: 0.01,
        defaultValue: 0.16,
        labelKey: 'formats.pngx.fields.palette256_profile_gradient_mean_max',
      },
      {
        id: 'pngx_palette256_profile_saturation_mean_max',
        type: 'number',
        min: -1,
        max: 1,
        step: 0.01,
        defaultValue: 0.42,
        labelKey: 'formats.pngx.fields.palette256_profile_saturation_mean_max',
      },
      {
        id: 'pngx_palette256_tune_opaque_ratio_threshold',
        type: 'number',
        min: -1,
        max: 1,
        step: 0.01,
        defaultValue: 0.9,
        labelKey: 'formats.pngx.fields.palette256_tune_opaque_ratio_threshold',
      },
      {
        id: 'pngx_palette256_tune_gradient_mean_max',
        type: 'number',
        min: -1,
        max: 1,
        step: 0.01,
        defaultValue: 0.14,
        labelKey: 'formats.pngx.fields.palette256_tune_gradient_mean_max',
      },
      {
        id: 'pngx_palette256_tune_saturation_mean_max',
        type: 'number',
        min: -1,
        max: 1,
        step: 0.01,
        defaultValue: 0.35,
        labelKey: 'formats.pngx.fields.palette256_tune_saturation_mean_max',
      },
      {
        id: 'pngx_palette256_tune_speed_max',
        type: 'number',
        min: -1,
        max: 10,
        step: 1,
        defaultValue: 1,
        labelKey: 'formats.pngx.fields.palette256_tune_speed_max',
      },
      {
        id: 'pngx_palette256_tune_quality_min_floor',
        type: 'number',
        min: -1,
        max: 100,
        step: 1,
        defaultValue: 90,
        labelKey: 'formats.pngx.fields.palette256_tune_quality_min_floor',
      },
      {
        id: 'pngx_palette256_tune_quality_max_target',
        type: 'number',
        min: -1,
        max: 100,
        step: 1,
        defaultValue: 100,
        labelKey: 'formats.pngx.fields.palette256_tune_quality_max_target',
      },
    ],
  },
];

const defaultOptions = buildDefaultOptions(sections);

function normalizePngxOptions(raw?: FormatOptions): FormatOptions {
  const normalized = normalizeOptionsWithSchema(sections, raw);
  const normalizedRecord = normalized as Record<string, unknown>;
  const lossyType = getNumeric(normalizedRecord['pngx_lossy_type']) ?? 0;
  const legacyReduced = raw ? getNumeric((raw as Record<string, unknown>)['pngx_lossy_reduced_colors']) : undefined;
  const providedMax = getNumeric(normalizedRecord['pngx_lossy_max_colors']);
  const rawRecord = raw ? (raw as Record<string, unknown>) : undefined;

  if (lossyType === 2) {
    const source = legacyReduced ?? providedMax;
    if (source === undefined || source <= 1) {
      normalizedRecord['pngx_lossy_max_colors'] = 1;
    } else {
      const manual = clampNumber(Math.trunc(source), 2, 32768);
      normalizedRecord['pngx_lossy_max_colors'] = manual;
    }
  } else {
    const paletteValue = clampNumber(Math.trunc(providedMax ?? 256), 2, 256);
    normalizedRecord['pngx_lossy_max_colors'] = paletteValue;
  }

  let sliderValue: number;
  const rawDitherValue = getNumeric(normalizedRecord['pngx_lossy_dither_level']);
  const rawDitherAuto = rawRecord ? rawRecord['pngx_lossy_dither_auto'] : undefined;
  if (rawDitherValue === undefined || Number.isNaN(rawDitherValue)) {
    sliderValue = 60;
  } else if (rawDitherValue < 0) {
    sliderValue = 60;
  } else if (rawDitherValue <= 1) {
    sliderValue = clampNumber(Math.round(rawDitherValue * 100), 0, 100);
  } else {
    sliderValue = clampNumber(Math.round(rawDitherValue), 0, 100);
  }

  let ditherAuto = Boolean(rawDitherAuto);
  if (!ditherAuto && rawDitherValue !== undefined && rawDitherValue < 0) {
    ditherAuto = true;
  }

  if (lossyType !== 1) {
    ditherAuto = false;
  }

  normalizedRecord['pngx_lossy_dither_auto'] = ditherAuto;
  normalizedRecord['pngx_lossy_dither_level'] = ditherAuto ? -1 : sliderValue;

  return normalized;
}

export const pngxFormat: FormatDefinition = {
  id: 'pngx',
  labelKey: 'formats.pngx.name',
  outputExtension: 'png',
  optionsSchema: sections,
  getDefaultOptions: () => ({ ...defaultOptions }),
  normalizeOptions: (raw?: FormatOptions) => normalizePngxOptions(raw),
  async convert({ Module, inputBytes, options }): Promise<Uint8Array> {
    const normalized = normalizePngxOptions(options);
    const bridgeInitialized = isPngxBridgeInitialized();

    if (bridgeInitialized) {
      const lossyEnabled = normalized.pngx_lossy_enable as boolean | undefined;
      const lossyTypeRaw = normalized.pngx_lossy_type;
      const lossyType = typeof lossyTypeRaw === 'number' ? lossyTypeRaw : typeof lossyTypeRaw === 'string' ? parseInt(lossyTypeRaw, 10) : 0;
      const optimizationLevel = (normalized.pngx_level as number | undefined) ?? 5;
      const stripSafe = (normalized.pngx_strip_safe as boolean | undefined) ?? true;
      const optimizeAlpha = (normalized.pngx_optimize_alpha as boolean | undefined) ?? true;

      if (lossyEnabled && (lossyType === 1 || lossyType === 2)) {
        return convertPngToPngx(Module, inputBytes, normalized);
      }

      if (lossyEnabled && lossyType === 0) {
        let prepareResult;

        try {
          prepareResult = pngxPalette256Prepare(Module, inputBytes, normalized);
        } catch {
          const losslessResult = pngxOptimizeLossless(inputBytes, { optimizationLevel, stripSafe, optimizeAlpha });
          return losslessResult.length < inputBytes.length ? losslessResult : inputBytes;
        }

        let quantResult = pngxQuantize(prepareResult.rgba, prepareResult.width, prepareResult.height, {
          speed: prepareResult.speed,
          qualityMin: prepareResult.qualityMin,
          qualityMax: prepareResult.qualityMax,
          maxColors: prepareResult.maxColors,
          ditheringLevel: prepareResult.ditherLevel,
          remap: true,
          importanceMap: prepareResult.importanceMap ?? undefined,
          fixedColors: prepareResult.fixedColors ?? undefined,
        });

        if (quantResult.status === 1 && prepareResult.qualityMin > 0) {
          quantResult = pngxQuantize(prepareResult.rgba, prepareResult.width, prepareResult.height, {
            speed: prepareResult.speed,
            qualityMin: 0,
            qualityMax: prepareResult.qualityMax,
            maxColors: prepareResult.maxColors,
            ditheringLevel: prepareResult.ditherLevel,
            remap: true,
            importanceMap: prepareResult.importanceMap ?? undefined,
            fixedColors: prepareResult.fixedColors ?? undefined,
          });
        }

        if (quantResult.status !== 0) {
          pngxPalette256Cleanup(Module);
          const losslessResult = pngxOptimizeLossless(inputBytes, { optimizationLevel, stripSafe, optimizeAlpha });
          return losslessResult.length < inputBytes.length ? losslessResult : inputBytes;
        }

        let quantizedPng;
        try {
          quantizedPng = pngxPalette256Finalize(Module, quantResult.indices, quantResult.palette);
        } catch {
          pngxPalette256Cleanup(Module);
          const losslessResult = pngxOptimizeLossless(inputBytes, { optimizationLevel, stripSafe, optimizeAlpha });
          return losslessResult.length < inputBytes.length ? losslessResult : inputBytes;
        } finally {
          pngxPalette256Cleanup(Module);
        }

        const optimized = pngxOptimizeLossless(quantizedPng, { optimizationLevel, stripSafe, optimizeAlpha });
        const lossyCandidate = optimized.length < quantizedPng.length ? optimized : quantizedPng;
        if (lossyCandidate.length < inputBytes.length) {
          return lossyCandidate;
        }

        const losslessOriginal = pngxOptimizeLossless(inputBytes, { optimizationLevel, stripSafe, optimizeAlpha });
        if (losslessOriginal.length < inputBytes.length) {
          return losslessOriginal;
        }

        throw new OutputLargerThanInputError('PNG', inputBytes.length, lossyCandidate.length);
      }

      const result = pngxOptimizeLossless(inputBytes, { optimizationLevel, stripSafe, optimizeAlpha });
      return result.length < inputBytes.length ? result : inputBytes;
    }

    return convertPngToPngx(Module, inputBytes, normalized);
  },
};
