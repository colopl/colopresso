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

import { BuildInfoPayload, FormatOptions, SettingsState, StatusMessage } from './index';

export type ElectronSettingKey = 'deletePng' | 'createZip';
export type ChromeSettingKey = 'processFolder' | 'createZip';
export type PlatformSettingKey = ElectronSettingKey | ChromeSettingKey;

export interface FileCandidate {
  file: File;
  path: string;
  data?: Uint8Array;
}

export interface ConversionOutcome {
  filename: string;
  data: Uint8Array;
  originalSize: number;
  convertedSize: number;
}

export interface ConversionResult {
  outputData: Uint8Array;
  pngData: Uint8Array;
  formatId: string;
  conversionTimeMs?: number;
}

export interface SaveFileResult {
  success: boolean;
  canceled?: boolean;
  error?: string;
}

export interface PlatformConfig {
  type: 'electron' | 'chrome';
  titleKey: string;
  dropZoneIconEmoji: string;
  dropZoneTextKey: string;
  dropZoneHintKey: string;
  defaultSettings: SettingsState;
  settingsStorageKey: string;
}

export interface SettingItemConfig {
  id: string;
  key: PlatformSettingKey;
  labelKey: string;
  descriptionKey: string;
  lockedByFixedOutput?: boolean;
}

export interface PlatformAdapter {
  readonly config: PlatformConfig;

  getSettingItems(): SettingItemConfig[];
  loadBuildInfo(): Promise<{ buildInfo: BuildInfoPayload | null; isThreadsEnabled: boolean }>;
  convertFile(candidate: FileCandidate, formatId: string, options: FormatOptions): Promise<ConversionResult>;
  collectFilesFromDrop(dataTransfer: DataTransfer, settings: SettingsState, applyStatus: (status: StatusMessage | null) => void): Promise<FileCandidate[]>;
  collectFilesFromInput(fileList: FileList): FileCandidate[];
  saveFile(filename: string, data: Uint8Array, relativePath?: string): Promise<SaveFileResult>;
  saveZip(files: ConversionOutcome[], applyStatus: (status: StatusMessage | null) => void): Promise<void>;
  handleBrowseClick?(isProcessing: boolean, applyStatus: (status: StatusMessage | null) => void): Promise<FileCandidate[] | null>;
  initialize?(): Promise<void>;
  cleanup?(): void;
  getHeaderTitle(t: (key: string, params?: Record<string, unknown>) => string): string;
  isFixedOutputActive?(settings: SettingsState): boolean;
  getOutputDirectoryPath?(settings: SettingsState): string;
  renderUpdateCta?(): React.ReactNode;
  renderExtraSettings?(
    settings: SettingsState,
    isProcessing: boolean,
    onSettingChange: (key: PlatformSettingKey, value: boolean) => void,
    t: (key: string, params?: Record<string, unknown>) => string
  ): React.ReactNode;
}

export interface PlatformContextValue {
  adapter: PlatformAdapter;
}
