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

import { convertPngToAvif } from '../core/converter';
import { FormatDefinition, FormatOptions, FormatSection } from '../types';
import { buildDefaultOptions, normalizeOptionsWithSchema } from './helpers';

const sections: FormatSection[] = [
  {
    section: 'basic',
    labelKey: 'formats.common.sections.basic',
    fields: [
      { id: 'quality', type: 'number', min: 0, max: 100, defaultValue: 50, labelKey: 'formats.avif.fields.quality' },
      { id: 'alpha_quality', type: 'number', min: 0, max: 100, defaultValue: 100, labelKey: 'formats.avif.fields.alpha_quality' },
      { id: 'lossless', type: 'checkbox', defaultValue: false, labelKey: 'formats.avif.fields.lossless' },
      { id: 'speed', type: 'number', min: 0, max: 10, defaultValue: 5, labelKey: 'formats.avif.fields.speed' },
    ],
  },
];

const defaultOptions = buildDefaultOptions(sections);

export const avifFormat: FormatDefinition = {
  id: 'avif',
  labelKey: 'formats.avif.name',
  outputExtension: 'avif',
  optionsSchema: sections,
  getDefaultOptions: () => ({ ...defaultOptions }),
  normalizeOptions: (raw?: FormatOptions) => normalizeOptionsWithSchema(sections, raw),
  async convert({ Module, inputBytes, options }): Promise<Uint8Array> {
    const normalized = normalizeOptionsWithSchema(sections, options);
    return convertPngToAvif(Module, inputBytes, normalized);
  },
};
