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

interface ElectronSaveDialogResult {
  success: boolean;
  filePath?: string;
  error?: string;
  canceled?: boolean;
}

interface ElectronWriteResult {
  success: boolean;
  error?: string;
}

interface ElectronCheckPathResult {
  success: boolean;
  isDirectory: boolean;
  isFile: boolean;
  error?: string;
}

interface ElectronDirectoryEntry {
  name: string;
  path: string;
  data: Uint8Array;
}

interface ElectronReadDirectoryResult {
  success: boolean;
  files?: ElectronDirectoryEntry[];
  error?: string;
}

interface ElectronSelectFolderResult {
  success: boolean;
  folderPath?: string;
  canceled?: boolean;
  error?: string;
}

interface ElectronInstallUpdateResult {
  success: boolean;
  error?: string;
}

interface ElectronDownloadUpdateResult {
  success: boolean;
  error?: string;
}

interface ElectronConfirmInstallUpdateResult {
  confirmed: boolean;
}

interface ElectronPngxBridgeFilesResult {
  success: boolean;
  jsSource?: string;
  wasmBytes?: ArrayBuffer;
  error?: string;
}

interface ElectronAPI {
  getPathForFile?: (file: File) => string | undefined;
  saveJsonDialog?: (defaultFileName: string) => Promise<ElectronSaveDialogResult>;
  saveZipDialog?: (defaultFileName: string) => Promise<ElectronSaveDialogResult>;
  saveFileDialog: (defaultFileName: string) => Promise<ElectronSaveDialogResult>;
  writeFile: (filePath: string, data: Uint8Array) => Promise<ElectronWriteResult>;
  writeFileInDirectory?: (directoryPath: string, relativePath: string, data: Uint8Array) => Promise<ElectronWriteResult>;
  deleteFile: (filePath: string) => Promise<ElectronWriteResult>;
  checkPathType: (path: string) => Promise<ElectronCheckPathResult>;
  readDirectory: (path: string) => Promise<ElectronReadDirectoryResult>;
  selectFolder: () => Promise<ElectronSelectFolderResult>;
  openExternal?: (url: string) => void;
  onUpdateDownloadStart?: (handler: (payload: Record<string, unknown>) => void) => () => void;
  onUpdateDownloadProgress?: (handler: (payload: Record<string, unknown>) => void) => () => void;
  onUpdateDownloadComplete?: (handler: (payload: Record<string, unknown>) => void) => () => void;
  onUpdateDownloadError?: (handler: (payload: Record<string, unknown>) => void) => () => void;
  onUpdateDownloadDeferred?: (handler: (payload: Record<string, unknown>) => void) => () => void;
  onUpdateInstallDeferred?: (handler: (payload: Record<string, unknown>) => void) => () => void;
  downloadUpdateNow?: () => Promise<ElectronDownloadUpdateResult>;
  confirmInstallUpdate?: (payload: { version?: string }) => Promise<ElectronConfirmInstallUpdateResult>;
  installUpdateNow?: () => Promise<ElectronInstallUpdateResult>;
  getUpdateChannel?: () => Promise<string | undefined>;
  getArchitecture?: () => Promise<string | undefined>;
  readPngxBridgeFiles?: (basePath: string) => Promise<ElectronPngxBridgeFilesResult>;
}

declare global {
  interface Window {
    electronAPI?: ElectronAPI;
  }
}
