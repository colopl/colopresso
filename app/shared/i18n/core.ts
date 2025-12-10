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

import { LanguageCode, TranslationBundle, TranslationParams } from '../types';
import enUS from './lang/enUS';
import jaJP from './lang/jaJP';

export const bundles: Record<LanguageCode, TranslationBundle> = {
  'en-us': enUS,
  'ja-jp': jaJP,
};

export function resolveMessage(tree: Record<string, unknown>, key: string): unknown {
  const parts = key.split('.');
  let current: unknown = tree;
  for (const part of parts) {
    if (typeof current !== 'object' || current === null) {
      return undefined;
    }
    const next = (current as Record<string, unknown>)[part];
    current = next;
  }
  return current;
}

export function format(template: string, params?: TranslationParams): string {
  if (!params) {
    return template;
  }
  return template.replace(/\{(.*?)\}/g, (match, name) => {
    const value = params[name.trim()];
    if (value === undefined || value === null) {
      return match;
    }
    return String(value);
  });
}

export function translateForLanguage(
  bundles: Record<LanguageCode, TranslationBundle>,
  language: LanguageCode,
  key: string,
  params?: TranslationParams
): string {
  const bundle = bundles[language] ?? bundles['en-us'];
  const raw = resolveMessage(bundle.messages as Record<string, unknown>, key);
  if (typeof raw === 'string') {
    return format(raw, params);
  }
  return key;
}
