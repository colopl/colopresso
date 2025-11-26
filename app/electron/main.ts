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

import { app, BrowserWindow, dialog, ipcMain } from 'electron';
import type { FileFilter, IpcMainInvokeEvent } from 'electron';
import path from 'node:path';
import { promises as fs } from 'node:fs';

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

interface ElectronSelectFolderResult {
  success: boolean;
  folderPath?: string;
  canceled?: boolean;
  error?: string;
}

interface ElectronSaveDialogResult {
  success: boolean;
  filePath?: string;
  canceled?: boolean;
  error?: string;
}

function sanitizeRelativePath(input: string): string {
  if (typeof input !== 'string' || input.trim().length === 0) {
    throw new Error('Invalid relative path');
  }
  const parts = input
    .replace(/\\/g, '/')
    .split('/')
    .map((segment) => segment.trim())
    .filter((segment) => segment.length > 0);

  if (parts.length === 0) {
    throw new Error('Invalid relative path');
  }

  const sanitized: string[] = [];
  for (const part of parts) {
    if (part === '.' || part.length === 0) {
      continue;
    }
    if (part === '..') {
      throw new Error('Path traversal is not allowed');
    }
    sanitized.push(part);
  }

  if (sanitized.length === 0) {
    throw new Error('Invalid relative path');
  }

  return path.join(...sanitized);
}

const PNG_EXTENSION = '.png';
let mainWindow: BrowserWindow | null = null;

function extractErrorMessage(error: unknown): string {
  if (error instanceof Error) {
    return error.message;
  }
  return 'Unknown error';
}

function createWindow(): void {
  const windowOptions = {
    width: 540,
    minWidth: 540,
    height: 860,
    minHeight: 830,
    autoHideMenuBar: true,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, 'preload.js'),
      webSecurity: true,
    },
  } as const;

  mainWindow = new BrowserWindow(windowOptions);

  void mainWindow.loadFile(path.join(__dirname, 'index.html'));

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

async function collectPngFiles(folderPath: string, basePath?: string): Promise<ElectronDirectoryEntry[]> {
  const rootPath = basePath ?? folderPath;
  const entries = await fs.readdir(folderPath, { withFileTypes: true });
  const files: ElectronDirectoryEntry[] = [];

  for (const entry of entries) {
    const fullPath = path.join(folderPath, entry.name);

    if (entry.isDirectory()) {
      const nested = await collectPngFiles(fullPath, rootPath);
      if (nested.length > 0) {
        files.push(...nested);
      }
      continue;
    }

    if (!entry.isFile()) {
      continue;
    }

    if (!entry.name.toLowerCase().endsWith(PNG_EXTENSION)) {
      continue;
    }

    const data = await fs.readFile(fullPath);
    const relativeName = path.relative(rootPath, fullPath) || entry.name;

    files.push({
      name: relativeName,
      path: fullPath,
      data: new Uint8Array(data.buffer, data.byteOffset, data.byteLength),
    });
  }

  return files;
}

async function handleReadDirectory(_event: IpcMainInvokeEvent, folderPath: string): Promise<ElectronReadDirectoryResult> {
  try {
    const files = await collectPngFiles(folderPath);
    return { success: true, files };
  } catch (error) {
    return { success: false, error: extractErrorMessage(error) };
  }
}

async function handleWriteFile(_event: IpcMainInvokeEvent, filePath: string, data: Uint8Array): Promise<ElectronWriteResult> {
  try {
    const buffer = Buffer.isBuffer(data) ? data : Buffer.from(data.buffer, data.byteOffset, data.byteLength);
    await fs.writeFile(filePath, buffer);
    return { success: true };
  } catch (error) {
    return { success: false, error: extractErrorMessage(error) };
  }
}

async function handleWriteFileInDirectory(_event: IpcMainInvokeEvent, directoryPath: string, relativePath: string, data: Uint8Array): Promise<ElectronWriteResult> {
  try {
    if (typeof directoryPath !== 'string' || directoryPath.trim().length === 0) {
      throw new Error('Invalid directory path');
    }
    const basePath = path.resolve(directoryPath);
    const safeRelative = sanitizeRelativePath(relativePath);
    const targetPath = path.join(basePath, safeRelative);
    await fs.mkdir(path.dirname(targetPath), { recursive: true });
    const buffer = Buffer.isBuffer(data) ? data : Buffer.from(data.buffer, data.byteOffset, data.byteLength);
    await fs.writeFile(targetPath, buffer);
    return { success: true };
  } catch (error) {
    return { success: false, error: extractErrorMessage(error) };
  }
}

async function handleDeleteFile(_event: IpcMainInvokeEvent, filePath: string): Promise<ElectronWriteResult> {
  try {
    await fs.unlink(filePath);
    return { success: true };
  } catch (error) {
    return { success: false, error: extractErrorMessage(error) };
  }
}

async function handleCheckPathType(_event: IpcMainInvokeEvent, filePath: string): Promise<ElectronCheckPathResult> {
  try {
    const stats = await fs.stat(filePath);
    return {
      success: true,
      isDirectory: stats.isDirectory(),
      isFile: stats.isFile(),
    };
  } catch (error) {
    return { success: false, isDirectory: false, isFile: false, error: extractErrorMessage(error) };
  }
}

async function handleSelectFolder(): Promise<ElectronSelectFolderResult> {
  if (!mainWindow) {
    return { success: false, error: 'Main window is not available' };
  }

  try {
    const result = await dialog.showOpenDialog(mainWindow, {
      properties: ['openDirectory'],
    });

    if (result.canceled) {
      return { success: false, canceled: true };
    }

    const folderPath = result.filePaths[0];

    if (!folderPath) {
      return { success: false, error: 'No folder selected' };
    }

    return { success: true, folderPath };
  } catch (error) {
    return { success: false, error: extractErrorMessage(error) };
  }
}

function buildSaveDialogFilters(extension: string): FileFilter[] {
  switch (extension) {
    case 'avif':
      return [
        { name: 'AVIF Images', extensions: ['avif'] },
        { name: 'All Files', extensions: ['*'] },
      ];
    case 'webp':
      return [
        { name: 'WebP Images', extensions: ['webp'] },
        { name: 'All Files', extensions: ['*'] },
      ];
    default:
      return [
        { name: 'Image', extensions: [extension && extension.length > 0 ? extension : '*'] },
        { name: 'All Files', extensions: ['*'] },
      ];
  }
}

async function handleSaveFileDialog(_event: IpcMainInvokeEvent, defaultFileName: string): Promise<ElectronSaveDialogResult> {
  if (!mainWindow) {
    return { success: false, error: 'Main window is not available' };
  }

  try {
    const ext = path.extname(defaultFileName).toLowerCase().replace(/^\./, '');
    const result = await dialog.showSaveDialog(mainWindow, {
      defaultPath: defaultFileName,
      filters: buildSaveDialogFilters(ext),
    });

    if (result.canceled) {
      return { success: false, canceled: true };
    }

    return { success: true, filePath: result.filePath ?? undefined };
  } catch (error) {
    return { success: false, error: extractErrorMessage(error) };
  }
}

async function handleSaveZipDialog(_event: IpcMainInvokeEvent, defaultFileName: string): Promise<ElectronSaveDialogResult> {
  if (!mainWindow) {
    return { success: false, error: 'Main window is not available' };
  }

  try {
    const result = await dialog.showSaveDialog(mainWindow, {
      defaultPath: defaultFileName,
      filters: [
        { name: 'ZIP Archives', extensions: ['zip'] },
        { name: 'All Files', extensions: ['*'] },
      ],
    });

    if (result.canceled) {
      return { success: false, canceled: true };
    }

    return { success: true, filePath: result.filePath ?? undefined };
  } catch (error) {
    return { success: false, error: extractErrorMessage(error) };
  }
}

async function handleSaveJsonDialog(_event: IpcMainInvokeEvent, defaultFileName: string): Promise<ElectronSaveDialogResult> {
  if (!mainWindow) {
    return { success: false, error: 'Main window is not available' };
  }

  try {
    const result = await dialog.showSaveDialog(mainWindow, {
      defaultPath: defaultFileName,
      filters: [
        { name: 'JSON Files', extensions: ['json'] },
        { name: 'All Files', extensions: ['*'] },
      ],
    });

    if (result.canceled) {
      return { success: false, canceled: true };
    }

    return { success: true, filePath: result.filePath ?? undefined };
  } catch (error) {
    return { success: false, error: extractErrorMessage(error) };
  }
}

function registerIpcHandlers(): void {
  ipcMain.handle('read-directory', handleReadDirectory);
  ipcMain.handle('write-file', handleWriteFile);
  ipcMain.handle('write-in-directory', handleWriteFileInDirectory);
  ipcMain.handle('delete-file', handleDeleteFile);
  ipcMain.handle('check-path-type', handleCheckPathType);
  ipcMain.handle('select-folder', () => handleSelectFolder());
  ipcMain.handle('save-file-dialog', handleSaveFileDialog);
  ipcMain.handle('save-zip-dialog', handleSaveZipDialog);
  ipcMain.handle('save-json-dialog', handleSaveJsonDialog);
}

app.whenReady().then(() => {
  createWindow();
  registerIpcHandlers();
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (BrowserWindow.getAllWindows().length === 0) {
    createWindow();
  }
});
