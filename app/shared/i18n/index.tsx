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

import React, { createContext, useCallback, useContext, useEffect, useMemo, useState } from 'react';
import Storage from '../core/storage';
import { TranslationParams, LanguageCode } from '../types';
import { bundles, translateForLanguage as translateForLanguageCore } from './core';

const LANGUAGE_STORAGE_KEY = 'colopresso_ui_language';

let currentLanguageRef: LanguageCode = getDefaultLanguage();

interface LanguageInfo {
  code: LanguageCode;
  label: string;
  flag: string;
}

const languageCatalog: LanguageInfo[] = [
  { code: 'en-us', label: 'English', flag: '🇺🇸' },
  { code: 'ja-jp', label: '日本語', flag: '🇯🇵' },
];

interface I18nContextValue {
  language: LanguageCode;
  t: (key: string, params?: TranslationParams) => string;
  setLanguage: (code: LanguageCode) => Promise<void>;
  availableLanguages: LanguageInfo[];
}

const I18nContext = createContext<I18nContextValue | undefined>(undefined);

type LanguageListener = (detail?: { language: LanguageCode }) => void;

const languageListeners = new Set<LanguageListener>();

function notifyLanguageChange(language: LanguageCode): void {
  languageListeners.forEach((listener) => {
    try {
      listener({ language });
    } catch (error) {
      console.warn('language listener failed', error);
    }
  });
  if (typeof window !== 'undefined' && typeof window.dispatchEvent === 'function') {
    window.dispatchEvent(new CustomEvent('colopresso:languageChanged', { detail: { language } }));
  }
}

export function addLanguageChangeListener(listener: LanguageListener): () => void {
  if (typeof listener !== 'function') {
    return () => undefined;
  }

  languageListeners.add(listener);

  return () => {
    languageListeners.delete(listener);
  };
}

export function t(key: string, params?: TranslationParams): string {
  return translateForLanguageCore(bundles, currentLanguageRef, key, params);
}

function getDefaultLanguage(): LanguageCode {
  const navigatorLang = typeof navigator !== 'undefined' ? navigator.language.toLowerCase() : '';

  if (navigatorLang.startsWith('ja')) {
    return 'ja-jp';
  }

  return 'en-us';
}

export const I18nProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [language, setLanguageState] = useState<LanguageCode>(() => getDefaultLanguage());

  useEffect(() => {
    let active = true;
    (async () => {
      const stored = await Storage.getItem<LanguageCode>(LANGUAGE_STORAGE_KEY);
      if (stored && bundles[stored] && active) {
        setLanguageState(stored);
      }
    })();
    return () => {
      active = false;
    };
  }, []);

  const t = useCallback(
    (key: string, params?: TranslationParams) => {
      return translateForLanguageCore(bundles, language, key, params);
    },
    [language]
  );

  const setLanguage = useCallback(async (code: LanguageCode) => {
    if (!bundles[code]) {
      return;
    }
    setLanguageState(code);
    await Storage.setItem(LANGUAGE_STORAGE_KEY, code);
  }, []);

  const value = useMemo<I18nContextValue>(
    () => ({
      language,
      t,
      setLanguage,
      availableLanguages: languageCatalog,
    }),
    [language, setLanguage, t]
  );

  useEffect(() => {
    currentLanguageRef = language;
    notifyLanguageChange(language);
  }, [language]);

  return <I18nContext.Provider value={value}>{children}</I18nContext.Provider>;
};

export function useI18n(): I18nContextValue {
  const ctx = useContext(I18nContext);
  if (!ctx) {
    throw new Error('useI18n must be used within I18nProvider');
  }

  return ctx;
}

export function useTranslation(): (key: string, params?: TranslationParams) => string {
  const { t } = useI18n();

  return t;
}

export function useAvailableLanguages(): LanguageInfo[] {
  const { availableLanguages } = useI18n();

  return availableLanguages;
}

export function useCurrentLanguage(): LanguageCode {
  const { language } = useI18n();

  return language;
}
