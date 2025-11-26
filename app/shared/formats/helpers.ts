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

import { ColorArrayField, FormatField, FormatOptions, FormatSection } from '../types';

function fieldDefault(field: FormatField): unknown {
  switch (field.type) {
    case 'number':
    case 'range':
    case 'select':
    case 'checkbox':
    case 'info':
      return (field as { defaultValue?: unknown }).defaultValue;
    case 'color-array':
      return (field as ColorArrayField).defaultValue ?? [];
    default:
      return undefined;
  }
}

export function buildDefaultOptions(sections: FormatSection[]): FormatOptions {
  const defaults: FormatOptions = {};
  sections.forEach((section) => {
    section.fields.forEach((field) => {
      if (!field.id) {
        return;
      }
      const value = fieldDefault(field);
      if (value !== undefined) {
        defaults[field.id] = value;
      }
    });
  });
  return defaults;
}

export function normalizeOptionsWithSchema(sections: FormatSection[], raw: FormatOptions | undefined): FormatOptions {
  const defaults = buildDefaultOptions(sections);
  return { ...defaults, ...(raw ?? {}) };
}
