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

import Storage from './storage';

export type Theme = 'light' | 'dark';

const THEME_STORAGE_KEY = 'colopresso_ui_theme';

type ThemeVariables = Record<string, string>;

const THEME_VARIABLES: Record<Theme, ThemeVariables> = {
  light: {
    '--color-bg-page': '#f5f5f5',
    '--color-bg-surface': '#ffffff',
    '--color-bg-alt': '#f9f9f9',
    '--color-bg-muted': '#f5f5f5',
    '--color-fg-base': '#333333',
    '--color-fg-muted': '#666666',
    '--color-fg-invert': '#ffffff',
    '--color-border': '#e0e0e0',
    '--color-border-strong': '#cccccc',
    '--color-accent': '#2563eb',
    '--color-accent-hover': '#1d4ed8',
    '--color-success-bg': '#e8f5e9',
    '--color-success-fg': '#2e7d32',
    '--color-error-bg': '#ffebee',
    '--color-error-fg': '#c62828',
    '--color-info-bg': '#e3f2fd',
    '--color-info-fg': '#1565c0',
    '--color-pending-bg': '#fff3e0',
    '--color-pending-fg': '#e65100',
    '--color-processing-bg': '#e3f2fd',
    '--color-processing-fg': '#1565c0',
    '--color-done-bg': '#e8f5e9',
    '--color-done-fg': '#2e7d32',
    '--color-dropzone-bg': '#fafafa',
    '--color-dropzone-border': '#cccccc',
    '--color-modal-overlay': 'rgba(0, 0, 0, 0.5)',
    '--color-shadow': 'rgba(0, 0, 0, 0.1)',
  },
  dark: {
    '--color-bg-page': '#1d1f21',
    '--color-bg-surface': '#26292c',
    '--color-bg-alt': '#2e3236',
    '--color-bg-muted': '#303437',
    '--color-fg-base': '#e5e7eb',
    '--color-fg-muted': '#9ca3af',
    '--color-fg-invert': '#111111',
    '--color-border': '#3c4146',
    '--color-border-strong': '#4b5258',
    '--color-accent': '#3b82f6',
    '--color-accent-hover': '#2563eb',
    '--color-success-bg': '#1b3a28',
    '--color-success-fg': '#6ee7b7',
    '--color-error-bg': '#3a1e24',
    '--color-error-fg': '#fca5a5',
    '--color-info-bg': '#1e3a5f',
    '--color-info-fg': '#93c5fd',
    '--color-pending-bg': '#47351c',
    '--color-pending-fg': '#f6d199',
    '--color-processing-bg': '#1e3a5f',
    '--color-processing-fg': '#93c5fd',
    '--color-done-bg': '#1b3a28',
    '--color-done-fg': '#6ee7b7',
    '--color-dropzone-bg': '#2b2f33',
    '--color-dropzone-border': '#4b5258',
    '--color-modal-overlay': 'rgba(0, 0, 0, 0.6)',
    '--color-shadow': 'rgba(0, 0, 0, 0.6)',
  },
};

function applyThemeVariables(theme: Theme): void {
  if (typeof document === 'undefined') {
    return;
  }

  const variables = THEME_VARIABLES[theme];
  const root = document.documentElement;
  Object.entries(variables).forEach(([name, value]) => {
    root.style.setProperty(name, value);
  });
}

function ensureColorSchemeMeta(theme: Theme): void {
  if (typeof document === 'undefined') {
    return;
  }
  let meta = document.querySelector('meta[name="color-scheme"]') as HTMLMetaElement | null;
  if (!meta) {
    meta = document.createElement('meta');
    meta.name = 'color-scheme';
    document.head.appendChild(meta);
  }
  meta.content = theme === 'dark' ? 'dark light' : 'light dark';
}

export function getPreferredTheme(): Theme {
  try {
    if (typeof window !== 'undefined' && window.matchMedia) {
      if (window.matchMedia('(prefers-color-scheme: dark)').matches) {
        return 'dark';
      }
      if (window.matchMedia('(prefers-color-scheme: light)').matches) {
        return 'light';
      }
    }
  } catch (error) {
    console.warn('getPreferredTheme failed', error);
  }
  return 'light';
}

export async function loadTheme(): Promise<Theme> {
  const stored = await Storage.getItem<Theme>(THEME_STORAGE_KEY);
  if (stored === 'dark' || stored === 'light') {
    return stored;
  }
  const preferred = getPreferredTheme();
  await Storage.setItem(THEME_STORAGE_KEY, preferred);
  return preferred;
}

export async function saveTheme(theme: Theme): Promise<void> {
  if (theme !== 'dark' && theme !== 'light') {
    return;
  }
  await Storage.setItem(THEME_STORAGE_KEY, theme);
}

export function applyThemeToDocument(theme: Theme): void {
  if (typeof document === 'undefined') {
    return;
  }

  const body = document.body;
  body.classList.remove('theme-light', 'theme-dark');
  body.classList.add(theme === 'dark' ? 'theme-dark' : 'theme-light');
  body.dataset.theme = theme;

  applyThemeVariables(theme);
  ensureColorSchemeMeta(theme);
}

export function getThemeVariables(theme: Theme): ThemeVariables {
  return THEME_VARIABLES[theme];
}
