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

import Storage from './storage';
import { FormatDefinition, FormatOptions, SETTINGS_SCHEMA_VERSION } from '../types';

const SELECTED_FORMAT_KEY = 'selectedFormatId';

export async function saveSelectedFormatId(formatId: string): Promise<void> {
  await Storage.setItem(SELECTED_FORMAT_KEY, formatId);
}

export async function loadSelectedFormatId(): Promise<string | null> {
  const value = await Storage.getItem<string>(SELECTED_FORMAT_KEY);
  return value ?? null;
}

function configKey(formatId: string): string {
  return `currentConfig_${formatId}`;
}

export async function loadFormatConfig(format: FormatDefinition): Promise<FormatOptions> {
  const stored = await Storage.getItem<FormatOptions & { _schemaVersion?: number }>(configKey(format.id));
  if (stored && stored._schemaVersion === SETTINGS_SCHEMA_VERSION) {
    const { _schemaVersion, ...rawConfig } = stored as Record<string, unknown>;
    return format.normalizeOptions ? format.normalizeOptions(rawConfig) : (rawConfig as FormatOptions);
  }
  const defaultConfig = format.getDefaultOptions();
  return defaultConfig;
}

export async function hasIncompatibleFormatConfig(format: FormatDefinition): Promise<boolean> {
  const stored = await Storage.getItem<FormatOptions & { _schemaVersion?: number }>(configKey(format.id));
  if (!stored) {
    return false;
  }
  if (stored._schemaVersion === undefined) {
    return true;
  }
  return stored._schemaVersion !== SETTINGS_SCHEMA_VERSION;
}

export async function saveFormatConfig(format: FormatDefinition, config: FormatOptions): Promise<void> {
  const stored: FormatOptions & { _schemaVersion: number } = {
    ...config,
    _schemaVersion: SETTINGS_SCHEMA_VERSION,
  };
  await Storage.setItem(configKey(format.id), stored);
}

export async function resetAllStoredData(): Promise<void> {
  await Storage.clear();
}
