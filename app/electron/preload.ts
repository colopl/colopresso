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

import { contextBridge, ipcRenderer, shell, webUtils } from 'electron';

const api: ElectronAPI = {
  getPathForFile: (file) => {
    try {
      return webUtils.getPathForFile(file);
    } catch (error) {
      console.warn('getPathForFile failed', error);
      return undefined;
    }
  },
  readDirectory: (folderPath) => ipcRenderer.invoke('read-directory', folderPath),
  writeFile: (filePath, data) => ipcRenderer.invoke('write-file', filePath, data),
  writeFileInDirectory: (directoryPath, relativePath, data) => ipcRenderer.invoke('write-in-directory', directoryPath, relativePath, data),
  deleteFile: (filePath) => ipcRenderer.invoke('delete-file', filePath),
  checkPathType: (filePath) => ipcRenderer.invoke('check-path-type', filePath),
  selectFolder: () => ipcRenderer.invoke('select-folder'),
  saveFileDialog: (defaultFileName) => ipcRenderer.invoke('save-file-dialog', defaultFileName),
  saveZipDialog: (defaultFileName) => ipcRenderer.invoke('save-zip-dialog', defaultFileName),
  saveJsonDialog: (defaultFileName) => ipcRenderer.invoke('save-json-dialog', defaultFileName),
  openExternal: (url) => void shell.openExternal(url),
};

contextBridge.exposeInMainWorld('electronAPI', api);
