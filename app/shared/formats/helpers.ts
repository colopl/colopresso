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

import { ColorArrayField, FormatField, FormatOptions, FormatSection, SelectField } from '../types';

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

function parseNumeric(value: unknown): number | undefined {
  if (typeof value === 'number') {
    return Number.isFinite(value) ? value : undefined;
  }

  if (typeof value === 'string' && value.trim() !== '') {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : undefined;
  }

  return undefined;
}

function coerceSelectValue(field: SelectField, value: unknown): unknown {
  const matched = field.options.find((option) => String(option.value) === String(value));
  if (matched) {
    return matched.value;
  }

  if (typeof value === 'string' && typeof field.defaultValue === 'number') {
    return parseNumeric(value) ?? value;
  }

  return value;
}

function coerceCheckboxValue(value: unknown): unknown {
  if (typeof value !== 'string') {
    return value;
  }

  const lowered = value.trim().toLowerCase();
  if (lowered === 'true') {
    return true;
  }
  if (lowered === 'false') {
    return false;
  }

  const numeric = parseNumeric(value);
  return numeric === undefined ? value : numeric !== 0;
}

// Values persisted from the UI or imported JSON may be strings (e.g. <select> values).
// Coerce them to the schema's types so every backend receives identical settings.
export function coerceFieldValue(field: FormatField, value: unknown): unknown {
  if (value === undefined || value === null) {
    return value;
  }

  switch (field.type) {
    case 'number':
    case 'range':
      return typeof value === 'string' ? (parseNumeric(value) ?? value) : value;
    case 'select':
      return coerceSelectValue(field, value);
    case 'checkbox':
      return coerceCheckboxValue(value);
    default:
      return value;
  }
}

export function normalizeOptionsWithSchema(sections: FormatSection[], raw: FormatOptions | undefined): FormatOptions {
  const normalized: FormatOptions = { ...buildDefaultOptions(sections), ...(raw ?? {}) };
  sections.forEach((section) => {
    section.fields.forEach((field) => {
      if (!field.id || !(field.id in normalized)) {
        return;
      }

      normalized[field.id] = coerceFieldValue(field, normalized[field.id]);
    });
  });

  return normalized;
}
