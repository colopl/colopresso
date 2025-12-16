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

export type LanguageCode = 'en-us' | 'ja-jp';

export interface LocalizedStringRef {
  key: string;
  fallback?: string;
}

export type LocalizedString = string | LocalizedStringRef;

export interface TranslationParams {
  [key: string]: string | number | boolean | undefined | null;
}

export interface TranslationBundle {
  locale: LanguageCode;
  messages: Record<string, unknown>;
}

export type FormatFieldType = 'number' | 'range' | 'checkbox' | 'select' | 'color-array' | 'info';

export interface FormatFieldBase<T extends FormatFieldType = FormatFieldType> {
  id: string;
  type: T;
  labelKey?: string;
  noteKey?: string;
}

export interface NumberField extends FormatFieldBase<'number'> {
  min?: number;
  max?: number;
  step?: number;
  defaultValue?: number;
}

export interface RangeField extends FormatFieldBase<'range'> {
  min?: number;
  max?: number;
  step?: number;
  defaultValue?: number;
}

export interface CheckboxField extends FormatFieldBase<'checkbox'> {
  defaultValue?: boolean;
}

export interface SelectFieldOption {
  value: string | number;
  labelKey: string;
}

export interface SelectField extends FormatFieldBase<'select'> {
  options: SelectFieldOption[];
  defaultValue?: string | number;
}

export interface ColorValue {
  r: number;
  g: number;
  b: number;
  a: number;
}

export interface ColorArrayField extends FormatFieldBase<'color-array'> {
  defaultValue?: ColorValue[];
}

export interface InfoField extends FormatFieldBase<'info'> {
  textKey: string;
}

export type FormatField = NumberField | RangeField | CheckboxField | SelectField | ColorArrayField | InfoField;

export interface FormatSection {
  section: string;
  labelKey: string;
  fields: FormatField[];
}

export type FormatOptions = Record<string, unknown>;

export interface ConvertRequestContext {
  Module: ColopressoModule;
  inputBytes: Uint8Array;
  options: FormatOptions;
}

export interface FormatDefinition {
  id: string;
  labelKey: string;
  outputExtension: string;
  optionsSchema: FormatSection[];
  getDefaultOptions(): FormatOptions;
  normalizeOptions?(raw?: FormatOptions): FormatOptions;
  convert(context: ConvertRequestContext): Promise<Uint8Array>;
}

export interface ColopressoModule {
  HEAPU8: Uint8Array;
  _malloc(size: number): number;
  _free(ptr: number): void;
  _cpres_free(ptr: number): void;
  getValue(ptr: number, type: 'i32'): number;
  [key: string]: unknown;
}

export interface StatusMessage {
  type: 'info' | 'error' | 'success';
  messageKey: string;
  durationMs?: number;
  params?: TranslationParams;
}

export interface ProgressState {
  current: number;
  total: number;
  state: 'idle' | 'processing' | 'done';
}

export interface FileEntry {
  id: string;
  name: string;
  status: 'pending' | 'processing' | 'success' | 'error';
  originalSize?: number;
  convertedSize?: number;
  errorMessageKey?: string;
  errorParams?: TranslationParams;
  errorMessageRaw?: string;
  errorCode?: string;
}

export interface AppSettingDefinition {
  key: string;
  type: 'checkbox';
  labelKey: string;
  descriptionKey?: string;
  defaultValue: boolean;
}

export const SETTINGS_SCHEMA_VERSION = 2;
export const SETTINGS_SCHEMA_VERSION_STORAGE_KEY = 'settingsSchemaVersion';

export interface SettingsState {
  deletePng?: boolean;
  createZip?: boolean;
  processFolder?: boolean;
  useOriginalOutput?: boolean;
  outputDirectoryPath?: string | null;
}

export interface BuildInfoPayload {
  version?: number;
  libwebpVersion?: number;
  libpngVersion?: number;
  libavifVersion?: number;
  pngxOxipngVersion?: number;
  pngxLibimagequantVersion?: number;
  buildtime?: number;
  releaseChannel?: string;
  architecture?: string;
}

export interface BuildInfoState {
  status: 'idle' | 'loading' | 'ready' | 'error';
  payload?: BuildInfoPayload;
}

export interface ProcessedFileResult {
  name: string;
  data: Uint8Array;
  originalSize: number;
  convertedSize: number;
}

export interface ZipSpecifier {
  filename: string;
  data: Uint8Array;
}

export interface EnvironmentDescriptor {
  type: 'chrome' | 'electron';
  titleKey: string;
  settings: AppSettingDefinition[];
}

export interface ProcessBatch {
  files: Array<{ name: string; data: Uint8Array; displayPath?: string }>;
  mode: 'direct';
}

export type DropHandlerResult = { kind: 'process'; batch: ProcessBatch } | { kind: 'status'; status: StatusMessage } | { kind: 'none' };

export interface DropZoneHandlers {
  onDrop: (dataTransfer: DataTransfer) => Promise<void>;
  onInputFiles: (files: FileList) => Promise<void>;
  onBrowseClick?: () => Promise<void>;
}

export interface BuildInfoProvider {
  load: () => Promise<BuildInfoPayload | null>;
}

export interface ModuleProvider {
  ensure: () => Promise<ColopressoModule>;
}

export interface ConversionResult {
  success: boolean;
  data?: Uint8Array;
  error?: string;
  formatId?: string;
}
