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

import React from 'react';
import styles from '../styles/App.module.css';
import { TranslationParams } from '../types';

export interface SettingItemConfig {
  id: string;
  type: 'checkbox' | 'select' | 'custom';
  labelKey: string;
  descriptionKey?: string;
  disabled?: boolean;
  locked?: boolean;
  lockedNoteKey?: string;
  value?: boolean | string | number;
  options?: { value: string | number; labelKey: string }[];
  onChange?: (value: boolean | string | number) => void;
  customContent?: React.ReactNode;
}

interface SettingItemProps {
  item: SettingItemConfig;
  t: (key: string, params?: TranslationParams) => string;
  isProcessing: boolean;
}

const SettingItem: React.FC<SettingItemProps> = ({ item, t, isProcessing }) => {
  const isDisabled = item.disabled || isProcessing;

  const renderInput = () => {
    if (item.type === 'custom' && item.customContent) {
      return item.customContent;
    }

    if (item.type === 'checkbox') {
      return (
        <label htmlFor={item.id}>
          <input id={item.id} type="checkbox" checked={Boolean(item.value)} disabled={isDisabled || item.locked} onChange={(e) => item.onChange?.(e.target.checked)} />
          <span>{t(item.labelKey)}</span>
        </label>
      );
    }

    if (item.type === 'select' && item.options) {
      return (
        <label htmlFor={item.id}>
          <span>{t(item.labelKey)}</span>
          <select id={item.id} value={item.value as string | number} disabled={isDisabled || item.locked} onChange={(e) => item.onChange?.(e.target.value)}>
            {item.options.map((opt) => (
              <option key={opt.value} value={opt.value}>
                {t(opt.labelKey)}
              </option>
            ))}
          </select>
        </label>
      );
    }

    return null;
  };

  return (
    <div className={styles.settingsItem}>
      {renderInput()}
      {item.descriptionKey && <div className={styles.settingsDescription}>{t(item.descriptionKey)}</div>}
      {item.locked && item.lockedNoteKey && <div className={styles.settingLockedNote}>{t(item.lockedNoteKey)}</div>}
    </div>
  );
};

interface SettingsPanelProps {
  items: SettingItemConfig[];
  isProcessing: boolean;
  t: (key: string, params?: TranslationParams) => string;
  className?: string;
}

const SettingsPanel: React.FC<SettingsPanelProps> = ({ items, isProcessing, t, className }) => {
  const panelClassName = [styles.settingsMenu, isProcessing ? styles.settingsDisabled : '', className ?? ''].filter(Boolean).join(' ');

  return (
    <div id="settingsMenu" className={panelClassName} aria-disabled={isProcessing}>
      {items.map((item) => (
        <SettingItem key={item.id} item={item} t={t} isProcessing={isProcessing} />
      ))}
    </div>
  );
};

export default SettingsPanel;
