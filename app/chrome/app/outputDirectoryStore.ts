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

const DB_NAME = 'colopresso-output';
const STORE_NAME = 'handles';
const OUTPUT_DIRECTORY_KEY = 'outputDirectory';

function openDatabase(): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    if (typeof indexedDB === 'undefined') {
      reject(new Error('indexedDB is unavailable in this environment'));
      return;
    }
    const request = indexedDB.open(DB_NAME, 1);
    request.onupgradeneeded = () => {
      const { result } = request;
      if (!result.objectStoreNames.contains(STORE_NAME)) {
        result.createObjectStore(STORE_NAME);
      }
    };
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error ?? new Error('Failed to open IndexedDB'));
  });
}

async function runTransaction<T>(mode: IDBTransactionMode, runner: (store: IDBObjectStore) => IDBRequest<T>): Promise<T> {
  const db = await openDatabase();
  try {
    return await new Promise<T>((resolve, reject) => {
      const tx = db.transaction(STORE_NAME, mode);
      const store = tx.objectStore(STORE_NAME);
      const request = runner(store);
      request.onsuccess = () => resolve(request.result as T);
      request.onerror = () => reject(request.error ?? tx.error ?? new Error('IndexedDB request failed'));
    });
  } finally {
    db.close();
  }
}

export async function saveOutputDirectoryHandle(handle: FileSystemDirectoryHandle): Promise<void> {
  await runTransaction('readwrite', (store) => store.put(handle, OUTPUT_DIRECTORY_KEY));
}

export async function loadOutputDirectoryHandle(): Promise<FileSystemDirectoryHandle | null> {
  try {
    const handle = (await runTransaction<FileSystemDirectoryHandle | undefined>('readonly', (store) => store.get(OUTPUT_DIRECTORY_KEY))) ?? null;
    return handle;
  } catch (error) {
    console.warn('Failed to load output directory handle', error);
    return null;
  }
}

export async function clearOutputDirectoryHandle(): Promise<void> {
  await runTransaction('readwrite', (store) => store.delete(OUTPUT_DIRECTORY_KEY));
}
