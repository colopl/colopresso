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

import React, { useEffect, useMemo, useRef, useState } from 'react';
import styles from '../styles/App.module.css';
import { FormatDefinition, LanguageCode, TranslationParams } from '../types';

interface AppHeaderProps {
  title: string;
  formats: FormatDefinition[];
  activeFormatId: string;
  language: LanguageCode;
  availableLanguages: Array<{ code: LanguageCode; label: string; flag: string }>;
  theme: 'light' | 'dark';
  isProcessing: boolean;
  onFormatChange: (formatId: string) => void;
  onLanguageChange: (language: LanguageCode) => void;
  onThemeToggle: () => void;
  onAdvancedSettingsClick: () => void;
  t: (key: string, params?: TranslationParams) => string;
  extraActions?: React.ReactNode;
}

const AppHeader: React.FC<AppHeaderProps> = ({
  title,
  formats,
  activeFormatId,
  language,
  availableLanguages,
  theme,
  isProcessing,
  onFormatChange,
  onLanguageChange,
  onThemeToggle,
  onAdvancedSettingsClick,
  t,
  extraActions,
}) => {
  const settingsButtonClassName = [styles.settingsButton, isProcessing ? styles.settingsButtonDisabled : ''].filter(Boolean).join(' ');
  const [isLanguageMenuOpen, setIsLanguageMenuOpen] = useState(false);
  const languagePickerRef = useRef<HTMLDivElement | null>(null);
  const selectedLanguage = useMemo(() => availableLanguages.find((lang) => lang.code === language) ?? availableLanguages[0], [availableLanguages, language]);

  useEffect(() => {
    if (!isLanguageMenuOpen) {
      return undefined;
    }

    const handlePointerDown = (event: PointerEvent) => {
      if (!languagePickerRef.current?.contains(event.target as Node)) {
        setIsLanguageMenuOpen(false);
      }
    };

    document.addEventListener('pointerdown', handlePointerDown);

    return () => {
      document.removeEventListener('pointerdown', handlePointerDown);
    };
  }, [isLanguageMenuOpen]);

  const selectLanguage = (code: LanguageCode) => {
    setIsLanguageMenuOpen(false);
    if (code !== language) {
      onLanguageChange(code);
    }
  };

  const languageFlagClassName = (code: LanguageCode): string => {
    switch (code) {
      case 'ja-jp':
        return `${styles.languageFlag} ${styles.languageFlagJp}`;
      case 'en-us':
      default:
        return `${styles.languageFlag} ${styles.languageFlagUs}`;
    }
  };

  return (
    <div className={styles.header}>
      <h1 className={styles.title}>{title}</h1>
      <div className={styles.headerActions}>
        <div className={styles.formatTabs}>
          {formats.map((format) => (
            <button
              key={format.id}
              type="button"
              className={[styles.formatTab, format.id === activeFormatId ? styles.formatTabActive : '', format.id === activeFormatId && format.id === 'pngx' ? styles.formatTabPngxActive : '']
                .filter(Boolean)
                .join(' ')}
              data-format-id={format.id}
              onClick={() => onFormatChange(format.id)}
            >
              {t(format.labelKey)}
            </button>
          ))}
        </div>
        {extraActions}
        <div className={styles.languagePicker} ref={languagePickerRef}>
          <button
            type="button"
            className={styles.languagePickerButton}
            onClick={() => setIsLanguageMenuOpen((open) => !open)}
            aria-label={`${t('header.languageSelect.aria')}: ${selectedLanguage?.label ?? language}`}
            aria-haspopup="listbox"
            aria-expanded={isLanguageMenuOpen}
            title={selectedLanguage?.label ?? t('header.languageSelect.title')}
          >
            <span className={languageFlagClassName(selectedLanguage?.code ?? language)} aria-hidden="true" />
          </button>
          {isLanguageMenuOpen && (
            <div className={styles.languageMenu} role="listbox" aria-label={t('header.languageSelect.title')}>
              {availableLanguages.map((lang) => (
                <button
                  key={lang.code}
                  type="button"
                  className={[styles.languageMenuItem, lang.code === language ? styles.languageMenuItemActive : ''].filter(Boolean).join(' ')}
                  onClick={() => selectLanguage(lang.code)}
                  role="option"
                  aria-selected={lang.code === language}
                >
                  <span className={languageFlagClassName(lang.code)} aria-hidden="true" />
                  <span>{lang.label}</span>
                </button>
              ))}
            </div>
          )}
        </div>
        <button type="button" className={styles.themeToggleButton} onClick={onThemeToggle} title={t('header.themeToggle.title')}>
          {theme === 'dark' ? '☀️' : '🌙'}
        </button>
        <button
          type="button"
          className={settingsButtonClassName}
          onClick={isProcessing ? undefined : onAdvancedSettingsClick}
          aria-haspopup="dialog"
          aria-label={t('settingsMenu.advancedSettings')}
          title={t('settingsMenu.advancedSettings')}
          aria-disabled={isProcessing}
        >
          ⚙️
        </button>
      </div>
    </div>
  );
};

export default AppHeader;
