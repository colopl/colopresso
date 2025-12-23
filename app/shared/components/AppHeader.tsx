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

import React from 'react';
import styles from '../styles/App.module.css';
import { FormatDefinition, TranslationParams } from '../types';

interface AppHeaderProps {
  title: string;
  formats: FormatDefinition[];
  activeFormatId: string;
  language: string;
  availableLanguages: Array<{ code: string; label: string; flag: string }>;
  theme: 'light' | 'dark';
  isProcessing: boolean;
  onFormatChange: (formatId: string) => void;
  onLanguageChange: (event: React.ChangeEvent<HTMLSelectElement>) => void;
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
        <select className={styles.languageSelect} value={language} onChange={onLanguageChange} aria-label={t('header.languageSelect.aria')}>
          {availableLanguages.map((lang) => (
            <option key={lang.code} value={lang.code} title={lang.label}>
              {lang.flag}
            </option>
          ))}
        </select>
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
