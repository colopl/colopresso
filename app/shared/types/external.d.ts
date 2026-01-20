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

import type { ColopressoModule } from './index';

declare global {
  interface ElectronAPI {
    readDirectory: (folderPath: string) => Promise<{ success: boolean; files?: Array<{ name: string; path: string; data: Uint8Array }>; error?: string }>;
    writeFile: (filePath: string, data: Uint8Array) => Promise<{ success: boolean; error?: string }>;
    deleteFile: (filePath: string) => Promise<{ success: boolean; error?: string }>;
    checkPathType: (filePath: string) => Promise<{ success: boolean; isDirectory: boolean; isFile: boolean; error?: string }>;
    selectFolder: () => Promise<{ success: boolean; folderPath?: string; error?: string; canceled?: boolean }>;
    saveFileDialog: (defaultFileName: string) => Promise<{ success: boolean; filePath?: string; error?: string; canceled?: boolean }>;
    saveZipDialog?: (defaultFileName: string) => Promise<{ success: boolean; filePath?: string; error?: string; canceled?: boolean }>;
    saveJsonDialog?: (defaultFileName: string) => Promise<{ success: boolean; filePath?: string; error?: string; canceled?: boolean }>;
    getPathForFile?: (file: File) => string | undefined;
    openExternal?: (url: string) => void;
    onUpdateDownloadStart?: (handler: (payload: Record<string, unknown>) => void) => () => void;
    onUpdateDownloadProgress?: (handler: (payload: Record<string, unknown>) => void) => () => void;
    onUpdateDownloadComplete?: (handler: (payload: Record<string, unknown>) => void) => () => void;
    onUpdateDownloadError?: (handler: (payload: Record<string, unknown>) => void) => () => void;
  }

  interface Window {
    electronAPI?: ElectronAPI;
  }
}

declare module '*colopresso.js' {
  const factory: (options?: Record<string, unknown>) => Promise<ColopressoModule>;
  export default factory;
}
