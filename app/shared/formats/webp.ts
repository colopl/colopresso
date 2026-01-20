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

import { convertPngToWebp } from '../core/converter';
import { FormatDefinition, FormatSection, FormatOptions } from '../types';
import { buildDefaultOptions, normalizeOptionsWithSchema } from './helpers';

const sections: FormatSection[] = [
  {
    section: 'basic',
    labelKey: 'formats.common.sections.basic',
    fields: [
      { id: 'quality', type: 'number', min: 0, max: 100, defaultValue: 80, labelKey: 'formats.webp.fields.quality' },
      { id: 'lossless', type: 'checkbox', defaultValue: false, labelKey: 'formats.webp.fields.lossless' },
      { id: 'method', type: 'number', min: 0, max: 6, defaultValue: 6, labelKey: 'formats.webp.fields.method' },
    ],
  },
  {
    section: 'target',
    labelKey: 'formats.common.sections.target',
    fields: [
      { id: 'target_size', type: 'number', min: 0, defaultValue: 0, labelKey: 'formats.webp.fields.target_size' },
      { id: 'target_psnr', type: 'number', min: 0, max: 100, step: 0.1, defaultValue: 0, labelKey: 'formats.webp.fields.target_psnr' },
    ],
  },
  {
    section: 'filter',
    labelKey: 'formats.common.sections.filter',
    fields: [
      { id: 'filter_strength', type: 'number', min: 0, max: 100, defaultValue: 60, labelKey: 'formats.webp.fields.filter_strength' },
      { id: 'filter_sharpness', type: 'number', min: 0, max: 7, defaultValue: 0, labelKey: 'formats.webp.fields.filter_sharpness' },
      {
        id: 'filter_type',
        type: 'select',
        defaultValue: 1,
        labelKey: 'formats.webp.fields.filter_type',
        options: [
          { value: 0, labelKey: 'formats.webp.fields.filter_type_simple' },
          { value: 1, labelKey: 'formats.webp.fields.filter_type_strong' },
        ],
      },
      { id: 'autofilter', type: 'checkbox', defaultValue: true, labelKey: 'formats.webp.fields.autofilter' },
    ],
  },
  {
    section: 'alpha',
    labelKey: 'formats.common.sections.alpha',
    fields: [
      { id: 'alpha_compression', type: 'checkbox', defaultValue: true, labelKey: 'formats.webp.fields.alpha_compression' },
      { id: 'alpha_filtering', type: 'number', min: 0, max: 2, defaultValue: 1, labelKey: 'formats.webp.fields.alpha_filtering' },
      { id: 'alpha_quality', type: 'number', min: 0, max: 100, defaultValue: 100, labelKey: 'formats.webp.fields.alpha_quality' },
    ],
  },
  {
    section: 'advanced',
    labelKey: 'formats.common.sections.advanced',
    fields: [
      { id: 'segments', type: 'number', min: 1, max: 4, defaultValue: 4, labelKey: 'formats.webp.fields.segments' },
      { id: 'sns_strength', type: 'number', min: 0, max: 100, defaultValue: 50, labelKey: 'formats.webp.fields.sns_strength' },
      { id: 'pass', type: 'number', min: 1, max: 10, defaultValue: 1, labelKey: 'formats.webp.fields.pass' },
      { id: 'preprocessing', type: 'number', min: 0, max: 2, defaultValue: 0, labelKey: 'formats.webp.fields.preprocessing' },
      { id: 'partitions', type: 'number', min: 0, max: 3, defaultValue: 0, labelKey: 'formats.webp.fields.partitions' },
      { id: 'partition_limit', type: 'number', min: 0, max: 100, defaultValue: 0, labelKey: 'formats.webp.fields.partition_limit' },
      { id: 'near_lossless', type: 'number', min: 0, max: 100, defaultValue: 100, labelKey: 'formats.webp.fields.near_lossless' },
      { id: 'emulate_jpeg_size', type: 'checkbox', defaultValue: false, labelKey: 'formats.webp.fields.emulate_jpeg_size' },
      { id: 'low_memory', type: 'checkbox', defaultValue: false, labelKey: 'formats.webp.fields.low_memory' },
      { id: 'exact', type: 'checkbox', defaultValue: false, labelKey: 'formats.webp.fields.exact' },
      { id: 'use_delta_palette', type: 'checkbox', defaultValue: false, labelKey: 'formats.webp.fields.use_delta_palette' },
      { id: 'use_sharp_yuv', type: 'checkbox', defaultValue: false, labelKey: 'formats.webp.fields.use_sharp_yuv' },
    ],
  },
];

const defaultOptions = buildDefaultOptions(sections);

export const webpFormat: FormatDefinition = {
  id: 'webp',
  labelKey: 'formats.webp.name',
  outputExtension: 'webp',
  optionsSchema: sections,
  getDefaultOptions: () => ({ ...defaultOptions }),
  normalizeOptions: (raw?: FormatOptions) => normalizeOptionsWithSchema(sections, raw),
  async convert({ Module, inputBytes, options }): Promise<Uint8Array> {
    const normalized = normalizeOptionsWithSchema(sections, options);
    return convertPngToWebp(Module, inputBytes, normalized);
  },
};
