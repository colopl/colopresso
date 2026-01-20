/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2026 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

const isChromeExtension = typeof chrome !== 'undefined' && typeof chrome.storage !== 'undefined';

export const Storage = {
  async getItem<T = unknown>(key: string): Promise<T | undefined> {
    if (isChromeExtension) {
      const result = await chrome.storage.local.get(key);
      return result[key] as T | undefined;
    }

    try {
      const raw = localStorage.getItem(key);
      if (raw === null) {
        return undefined;
      }
      return JSON.parse(raw) as T;
    } catch (error) {
      console.warn('Storage#getItem failed', error);
      return undefined;
    }
  },

  async getJSON<T = unknown>(key: string, fallback?: T): Promise<T | undefined> {
    const value = await this.getItem<T>(key);
    if (value === undefined || value === null) {
      return fallback;
    }

    return value;
  },

  async setJSON(key: string, value: unknown): Promise<void> {
    await this.setItem(key, value);
  },

  async setItem(key: string, value: unknown): Promise<void> {
    if (isChromeExtension) {
      await chrome.storage.local.set({ [key]: value });
      return;
    }

    try {
      localStorage.setItem(key, JSON.stringify(value));
    } catch (error) {
      console.warn('Storage#setItem failed', error);
    }
  },

  async removeItem(key: string): Promise<void> {
    if (isChromeExtension) {
      await chrome.storage.local.remove(key);
      return;
    }

    try {
      localStorage.removeItem(key);
    } catch (error) {
      console.warn('Storage#removeItem failed', error);
    }
  },

  async clear(): Promise<void> {
    if (isChromeExtension) {
      await chrome.storage.local.clear();
      return;
    }

    try {
      localStorage.clear();
    } catch (error) {
      console.warn('Storage#clear failed', error);
    }
  },
};

type StorageType = typeof Storage;
export type StorageLike = StorageType;

export default Storage;
