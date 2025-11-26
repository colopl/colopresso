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

import { FormatDefinition, FormatOptions } from '../types';
import { avifFormat } from './avif';
import { pngxFormat } from './pngx';
import { webpFormat } from './webp';

const definitions: FormatDefinition[] = [webpFormat, avifFormat, pngxFormat];
const registry = new Map<string, FormatDefinition>(definitions.map((def) => [def.id, def]));

let activeFormatId: string = definitions[0]?.id ?? 'webp';

const listeners = new Set<(format: FormatDefinition) => void>();

export function getFormats(): FormatDefinition[] {
  return definitions.slice();
}

export function getDefaultFormat(): FormatDefinition {
  const [first] = definitions;
  if (!first) {
    throw new Error('No formats registered');
  }
  return first;
}

export function getFormat(id: string): FormatDefinition | undefined {
  return registry.get(id);
}

export function getActiveFormat(): FormatDefinition {
  const format = registry.get(activeFormatId);
  if (!format) {
    throw new Error('No active format registered');
  }
  return format;
}

export function setActiveFormat(id: string): void {
  if (!registry.has(id)) {
    throw new Error(`Unknown format: ${id}`);
  }
  activeFormatId = id;
}

export function activateFormat(id: string): void {
  const current = getActiveFormat();
  if (current.id === id) {
    return;
  }
  setActiveFormat(id);
  const next = getActiveFormat();
  listeners.forEach((listener) => {
    try {
      listener(next);
    } catch (error) {
      console.warn('format change listener failed', error);
    }
  });
}

export function onFormatChange(listener: (format: FormatDefinition) => void): () => void {
  listeners.add(listener);
  return () => listeners.delete(listener);
}

export function normalizeFormatOptions(format: FormatDefinition, options?: FormatOptions): FormatOptions {
  if (format.normalizeOptions) {
    return format.normalizeOptions(options);
  }
  if (options) {
    return options;
  }
  return format.getDefaultOptions();
}

export async function convertWithFormat(params: { Module: unknown; inputBytes: Uint8Array; format: FormatDefinition; options?: FormatOptions }): Promise<Uint8Array> {
  const normalizedOptions = normalizeFormatOptions(params.format, params.options);
  return params.format.convert({ Module: params.Module as any, inputBytes: params.inputBytes, options: normalizedOptions });
}

export function getFormatDisplayName(format: FormatDefinition, translate: (key: string) => string): string {
  return translate(format.labelKey);
}
