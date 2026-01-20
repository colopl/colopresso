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

import { getFormat, getFormatDisplayName } from '../formats';
import Storage from './storage';
import { t, addLanguageChangeListener } from '../i18n';
import { FormatDefinition, FormatOptions, FormatField, ColorValue, SETTINGS_SCHEMA_VERSION } from '../types';

const ADVANCED_SETTINGS_STYLES = `
.modal {
  display: none;
  position: fixed;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  background: var(--color-modal-overlay);
  z-index: 1000;
  overflow-y: auto;
}

.modal.active {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 20px;
}

.modal-content {
  background: var(--color-bg-surface);
  border-radius: 8px;
  max-width: 900px;
  width: 100%;
  max-height: 90vh;
  display: flex;
  flex-direction: column;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
  border: 1px solid var(--color-border);
  color: var(--color-fg-base);
}

.modal-header {
  padding: 20px;
  border-bottom: 1px solid var(--color-border);
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.modal-header h2 {
  margin: 0;
  font-size: 18px;
  color: var(--color-fg-base);
}

.modal-close {
  background: none;
  border: none;
  font-size: 28px;
  color: var(--color-fg-muted);
  cursor: pointer;
  padding: 0;
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 4px;
  transition: all 0.2s;
}

.modal-close:hover {
  background: var(--color-bg-muted);
}

.modal-body {
  padding: 20px;
  overflow-y: auto;
  flex: 1;
}

.modal-footer {
  padding: 16px 20px;
  border-top: 1px solid var(--color-border);
  display: flex;
  gap: 12px;
  justify-content: flex-end;
}

.btn-primary,
.btn-secondary {
  padding: 8px 16px;
  border: none;
  border-radius: 4px;
  font-size: 13px;
  cursor: pointer;
  transition: all 0.2s;
}

.btn-primary {
  background: #4caf50;
  color: #ffffff;
}

.btn-primary:hover {
  background: #388e3c;
}

.btn-secondary {
  background: var(--color-bg-muted);
  color: var(--color-fg-base);
  border: 1px solid var(--color-border);
}

.btn-secondary:hover {
  background: var(--color-bg-alt);
}

.profile-section {
  margin-bottom: 20px;
  padding: 16px;
  background: var(--color-bg-alt);
  border-radius: 4px;
  display: flex;
  gap: 8px;
  align-items: center;
  flex-wrap: wrap;
  border: 1px solid var(--color-border);
}

.profile-section label {
  font-size: 13px;
  font-weight: bold;
  color: var(--color-fg-base);
}

.reload-required-section {
  margin-bottom: 20px;
  padding: 16px;
  background: var(--color-bg-muted);
  border-radius: 4px;
  border: 1px solid var(--color-border);
}

.reload-required-section h3 {
  margin: 0 0 12px 0;
  font-size: 14px;
  color: var(--color-fg-base);
  border-bottom: 1px solid var(--color-border);
  padding-bottom: 8px;
  position: relative;
}

.reload-required-section h3::after {
  content: '';
  position: absolute;
  left: 0;
  bottom: 0;
  width: 100%;
  height: 2px;
  background: var(--color-accent);
  opacity: 0.15;
  pointer-events: none;
}

.reload-required-section .reload-warning {
  color: #c53030;
  font-size: 12px;
  margin-top: 8px;
  margin-bottom: 16px;
  padding-bottom: 12px;
  border-bottom: 1px solid var(--color-border);
}

.reload-required-section .config-item {
  margin-bottom: 0;
}

.profile-section select {
  flex: 1;
  min-width: 150px;
  padding: 6px 8px;
  font-size: 13px;
  border: 1px solid var(--color-border-strong);
  border-radius: 4px;
  background: var(--color-bg-surface);
  color: var(--color-fg-base);
}

.profile-btn {
  padding: 6px 12px;
  font-size: 12px;
  background: var(--color-accent);
  color: var(--color-fg-invert);
  border: none;
  border-radius: 4px;
  cursor: pointer;
  transition: all 0.2s;
}

.profile-btn:hover {
  background: var(--color-accent-hover);
}

.config-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
  gap: 20px;
}

.config-group {
  background: var(--color-bg-muted);
  padding: 16px;
  border-radius: 4px;
  border: 1px solid var(--color-border);
}

.config-group h3 {
  margin: 0 0 12px 0;
  font-size: 14px;
  color: var(--color-fg-base);
  border-bottom: 1px solid var(--color-border);
  padding-bottom: 8px;
  position: relative;
}

.config-group h3::after {
  content: '';
  position: absolute;
  left: 0;
  bottom: 0;
  width: 100%;
  height: 2px;
  background: var(--color-accent);
  opacity: 0.15;
  pointer-events: none;
}

.config-item {
  margin-bottom: 12px;
}

.config-item:last-child {
  margin-bottom: 0;
}

.config-item.disabled {
  opacity: 0.6;
}

.config-item.disabled input,
.config-item.disabled select,
.config-item.disabled button {
  cursor: not-allowed;
}

.config-item label {
  display: block;
  font-size: 12px;
  color: var(--color-fg-muted);
  margin-bottom: 4px;
}

.config-item input[type='number'],
.config-item select {
  width: 100%;
  padding: 6px 8px;
  font-size: 12px;
  border: 1px solid var(--color-border-strong);
  border-radius: 4px;
  background: var(--color-bg-surface);
  color: var(--color-fg-base);
}

.range-control {
  display: flex;
  align-items: center;
  gap: 8px;
}

.range-control input[type='range'] {
  flex: 1;
}

.range-control .range-value {
  min-width: 32px;
  text-align: right;
  font-size: 12px;
  font-variant-numeric: tabular-nums;
  color: var(--color-fg-base);
}

.config-item input[type='checkbox'] {
  margin-right: 6px;
}

.range-control {
  display: flex;
  align-items: center;
  gap: 8px;
  flex-wrap: wrap;
}

.range-control input[type='range'] {
  flex: 1;
  min-width: 160px;
}

.range-control .range-value {
  min-width: 48px;
  text-align: right;
  font-size: 12px;
  font-variant-numeric: tabular-nums;
  color: var(--color-fg-base);
}

.range-inline-slot {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-left: auto;
}

.range-inline-checkbox {
  margin: 0;
  padding: 0;
  display: flex;
  align-items: center;
  gap: 6px;
}

.range-inline-checkbox label {
  margin: 0;
  display: flex;
  align-items: center;
  font-size: 12px;
  color: var(--color-fg-base);
}

.range-inline-checkbox input[type='checkbox'] {
  margin-right: 4px;
}
.config-item .hint {
  font-size: 11px;
  color: var(--color-fg-muted);
  font-style: italic;
  display: block;
  margin-top: 2px;
}

.config-item.error label {
  color: var(--color-error-fg);
}

.field-note {
  font-size: 12px;
  color: var(--color-fg-muted);
  margin-top: 4px;
}

.field-error {
  font-size: 12px;
  color: var(--color-error-fg);
  margin-top: 4px;
}

.name-prompt-overlay {
  position: fixed;
  inset: 0;
  background: var(--color-modal-overlay);
  z-index: 10002;
  display: flex;
  align-items: center;
  justify-content: center;
}

.name-prompt-dialog {
  max-width: 340px;
  width: 90%;
  background: var(--color-bg-surface);
  padding: 18px;
  border-radius: 8px;
  box-shadow: 0 4px 20px var(--color-shadow);
  display: flex;
  flex-direction: column;
  gap: 12px;
  border: 1px solid var(--color-border);
}

.name-prompt-title {
  margin: 0;
  font-size: 16px;
  color: var(--color-fg-base);
}

.name-prompt-input {
  padding: 8px;
  font-size: 14px;
  border: 1px solid var(--color-border-strong);
  border-radius: 4px;
  background: var(--color-bg-surface);
  color: var(--color-fg-base);
}

.name-prompt-input:focus {
  outline: 2px solid var(--color-accent);
  outline-offset: 0;
}

.name-prompt-actions {
  text-align: right;
  display: flex;
  gap: 8px;
  justify-content: flex-end;
}

.color-array-item {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.color-array-container {
  display: flex;
  flex-direction: column;
  gap: 8px;
  padding: 8px;
  background: var(--color-bg-alt);
  border: 1px solid var(--color-border);
  border-radius: 4px;
  max-height: 300px;
  overflow-y: auto;
}

.color-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px;
  background: var(--color-bg-surface);
  border: 1px solid var(--color-border);
  border-radius: 4px;
}

.color-preview {
  width: 80px;
  height: 36px;
  border: 1px solid var(--color-border);
  border-radius: 4px;
  display: flex;
  align-items: center;
  justify-content: center;
  font-family: monospace;
  font-size: 11px;
  font-weight: bold;
  text-shadow: 0 0 2px rgba(0, 0, 0, 0.3);
  user-select: none;
  flex-shrink: 0;
}

.hex-prefix {
  font-family: monospace;
  font-size: 14px;
  font-weight: bold;
  color: var(--color-fg-muted);
  user-select: none;
}

.hex-input {
  width: 80px;
  padding: 6px 8px;
  font-family: monospace;
  font-size: 14px;
  border: 1px solid var(--color-border);
  border-radius: 4px;
  background: var(--color-bg-surface);
  color: var(--color-fg-base);
  text-transform: uppercase;
}

.hex-input:focus {
  outline: 2px solid var(--color-accent);
  outline-offset: 0;
  border-color: var(--color-accent);
}

.color-picker {
  width: 60px;
  height: 36px;
  border: 1px solid var(--color-border);
  border-radius: 4px;
  cursor: pointer;
}

.alpha-slider {
  flex: 1;
  min-width: 100px;
}

.alpha-value {
  min-width: 40px;
  text-align: right;
  font-family: monospace;
  color: var(--color-fg-muted);
}

.btn-remove-color {
  width: 28px;
  height: 28px;
  padding: 0;
  border: none;
  background: var(--color-error-bg);
  color: var(--color-error-fg);
  border-radius: 4px;
  cursor: pointer;
  font-size: 20px;
  line-height: 1;
  transition: background-color 0.2s;
}

.btn-remove-color:hover {
  background: var(--color-error-fg);
  color: var(--color-fg-invert);
}

.btn-add-color {
  padding: 8px 16px;
  background: var(--color-accent);
  color: var(--color-fg-invert);
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 14px;
  transition: background-color 0.2s;
}

.btn-add-color:hover {
  background: var(--color-accent-hover);
}

.theme-dark .profile-section {
  background: var(--color-bg-muted);
  border-color: var(--color-border-strong);
}

.theme-dark .profile-section label {
  color: var(--color-fg-base);
}

.theme-dark .reload-required-section {
  background: var(--color-bg-muted);
  border-color: var(--color-border-strong);
}

.theme-dark .reload-required-section .reload-warning {
  color: #fc8181;
}

.theme-dark .profile-section select {
  background: var(--color-bg-surface);
  color: var(--color-fg-base);
  border-color: var(--color-border-strong);
}

.theme-dark .profile-btn {
  background: var(--color-accent);
  color: var(--color-fg-invert);
}

.theme-dark .profile-btn:hover {
  background: var(--color-accent-hover);
}

.theme-dark .config-group h3::after {
  opacity: 0.35;
}

.theme-dark .config-group h3 {
  color: var(--color-fg-base);
}

.theme-dark .modal-content {
  box-shadow: 0 4px 28px rgba(0, 0, 0, 0.7);
}

.theme-dark .config-group {
  background: var(--color-bg-muted);
}

.theme-dark .config-item label {
  color: var(--color-fg-muted);
}

.theme-dark .name-prompt-overlay {
  background: var(--color-modal-overlay);
}

.theme-dark .name-prompt-dialog {
  background: var(--color-bg-surface);
  border-color: var(--color-border);
  box-shadow: 0 4px 20px var(--color-shadow);
}

.theme-dark .name-prompt-title {
  color: var(--color-fg-base);
}

.theme-dark .name-prompt-input {
  background: var(--color-bg-surface);
  color: var(--color-fg-base);
  border-color: var(--color-border-strong);
}

.theme-dark .name-prompt-actions .btn-secondary {
  background: var(--color-bg-muted);
  color: var(--color-fg-base);
  border-color: var(--color-border);
}

.theme-dark .name-prompt-actions .btn-secondary:hover {
  background: var(--color-bg-alt);
}
`;

export interface AdvancedSettingsDeps {
  getDefaultConfigForFormat: (format: FormatDefinition) => FormatOptions;
  saveCurrentConfigForFormat: (format: FormatDefinition, config: FormatOptions) => Promise<void> | void;
  isThreadsEnabled?: boolean;
}

export interface AdvancedSettingsController {
  open: () => Promise<void>;
  dispose: () => void;
}

interface ProfileDictionary {
  [name: string]: FormatOptions & { _formatId?: string; _schemaVersion?: number };
}

const lastProfileSelection = new Map<string, string>();

let activeManager: AdvancedSettingsManager | null = null;
let stylesInjected = false;
let threadsEnabledFlag = true;

function ensureStylesInjected(): void {
  if (stylesInjected) {
    return;
  }

  const styleElement = document.createElement('style');
  styleElement.type = 'text/css';
  styleElement.textContent = ADVANCED_SETTINGS_STYLES;
  document.head.appendChild(styleElement);
  stylesInjected = true;
}

export async function setupFormatAwareAdvancedSettings(
  format: FormatDefinition,
  initialConfig: FormatOptions,
  onApply: (config: FormatOptions) => void,
  deps: AdvancedSettingsDeps
): Promise<AdvancedSettingsController> {
  if (!deps.getDefaultConfigForFormat || !deps.saveCurrentConfigForFormat) {
    throw new Error('setupFormatAwareAdvancedSettings: missing required deps');
  }

  threadsEnabledFlag = deps.isThreadsEnabled ?? true;

  ensureStylesInjected();

  if (activeManager) {
    activeManager.dispose();
  }

  activeManager = new AdvancedSettingsManager(format, initialConfig, onApply, deps);
  return activeManager.getController();
}

class AdvancedSettingsManager {
  private readonly format: FormatDefinition;

  private currentConfig: FormatOptions | null;

  private languageDisposer: (() => void) | null = null;

  private configAppliedListener: ((event: Event) => void) | null = null;

  private readonly onApply: (config: FormatOptions) => void;

  private readonly deps: AdvancedSettingsDeps;

  constructor(format: FormatDefinition, initialConfig: FormatOptions, onApply: (config: FormatOptions) => void, deps: AdvancedSettingsDeps) {
    this.format = format;
    this.currentConfig = cloneConfig(initialConfig);
    this.onApply = onApply;
    this.deps = deps;

    this.ensureLanguageListener();
    this.ensureConfigAppliedListener();
  }

  dispose(): void {
    if (this.languageDisposer) {
      this.languageDisposer();
      this.languageDisposer = null;
    }

    if (this.configAppliedListener) {
      window.removeEventListener('colopresso:configApplied', this.configAppliedListener);
      this.configAppliedListener = null;
    }

    const modal = document.getElementById('advancedModal');
    if (modal) {
      modal.remove();
    }
  }

  getController(): AdvancedSettingsController {
    return {
      open: async () => {
        await this.open();
      },
      dispose: () => {
        this.dispose();
      },
    };
  }

  private ensureLanguageListener(): void {
    if (this.languageDisposer) {
      return;
    }

    this.languageDisposer = addLanguageChangeListener(() => {
      const modal = document.getElementById('advancedModal');
      if (modal) {
        modal.remove();
      }
    });
  }

  private ensureConfigAppliedListener(): void {
    if (this.configAppliedListener) {
      return;
    }

    this.configAppliedListener = (event: Event) => {
      const customEvent = event as CustomEvent<{ config: FormatOptions; format: FormatDefinition }>;
      const detail = customEvent.detail;
      if (!detail || !detail.config || !detail.format) {
        return;
      }
      if (detail.format.id !== this.format.id) {
        return;
      }
      this.currentConfig = cloneConfig(detail.config);
      lastProfileSelection.set(detail.format.id, '__current');
    };
    window.addEventListener('colopresso:configApplied', this.configAppliedListener);
  }

  async open(): Promise<void> {
    if (!document.getElementById('advancedModal')) {
      await this.ensureModal(this.format);
    }

    this.populateAndDisplay(this.format);
  }

  private async ensureModal(format: FormatDefinition): Promise<void> {
    if (document.getElementById('advancedModal')) {
      return;
    }

    const config = this.currentConfig ?? {};
    const html = buildModalHTML(format, config);
    document.body.insertAdjacentHTML('beforeend', html);
    await this.wireModalInternals(format);
  }

  private populateAndDisplay(format: FormatDefinition): void {
    const modal = document.getElementById('advancedModal');
    if (!modal) {
      return;
    }

    const config = this.currentConfig ?? {};
    lastProfileSelection.set(format.id, '__current');
    applyConfigToModalFields(format, config);
    void refreshProfileSelect(format, config, lastProfileSelection.get(format.id) ?? '__current', true);
    openModalWithFocusTrap(modal);
  }

  private async wireModalInternals(format: FormatDefinition): Promise<void> {
    const modal = document.getElementById('advancedModal');
    const btnClose = document.getElementById('advancedModalClose');
    const btnApply = document.getElementById('advancedApplyBtn');
    const btnReset = document.getElementById('advancedResetBtn');

    if (!modal) {
      return;
    }

    const handleClose = () => closeModal(modal);
    if (btnClose) {
      btnClose.addEventListener('click', handleClose);
    }

    modal.addEventListener('click', (event) => {
      if (event.target === modal) {
        handleClose();
      }
    });

    const modalContent = modal.querySelector('.modal-content');
    if (modalContent) {
      modalContent.addEventListener('click', (event) => {
        event.stopPropagation();
      });
    }

    const lossyTypeSelect = modal.querySelector<HTMLSelectElement>("[data-field-id='pngx_lossy_type']");
    if (lossyTypeSelect) {
      lossyTypeSelect.addEventListener('change', () => {
        updatePngxLossyMaxColorsState(true);
      });
    }
    const ditherAutoInput = modal.querySelector<HTMLInputElement>("[data-field-id='pngx_lossy_dither_auto']");
    if (ditherAutoInput) {
      ditherAutoInput.addEventListener('change', () => {
        updatePngxLossyMaxColorsState();
      });
    }

    updatePngxLossyMaxColorsState();

    if (btnReset) {
      btnReset.addEventListener('click', () => {
        const defaultConfig = this.deps.getDefaultConfigForFormat(format) ?? {};
        applyConfigToModalFields(format, defaultConfig);
        lastProfileSelection.set(format.id, '__current');
        const selectElement = document.getElementById('profileSelect') as HTMLSelectElement | null;
        if (selectElement) {
          selectElement.value = '__current';
        }
        modal.querySelectorAll('.config-item.error').forEach((node) => node.classList.remove('error'));
        modal.querySelectorAll('.field-error').forEach((node) => {
          node.textContent = '';
        });
      });
    }

    if (btnApply) {
      btnApply.addEventListener('click', async () => {
        const config = readConfigFromModal(format);
        const errors = validateConfig(format, config);
        showValidationErrors(errors);
        if (Object.keys(errors).length > 0) {
          alert(t('settingsModal.alerts.applyValidationError'));
          return;
        }

        const needsReload = hasReloadRequiredFieldsChanged(format, this.currentConfig, config);

        await this.persistConfig(format, config);
        this.onApply(config);
        window.dispatchEvent(new CustomEvent('colopresso:configApplied', { detail: { config, format } }));
        closeModal(modal);

        if (needsReload) {
          const shouldReload = confirm(t('settingsModal.alerts.reloadRequired'));
          if (shouldReload) {
            sessionStorage.setItem('checkForUpdatesAfterReload', 'true');
            window.location.reload();
          }
        }
      });
    }

    await this.setupProfiles(format);
    this.setupColorArrayHandlers();
  }

  private async persistConfig(format: FormatDefinition, config: FormatOptions): Promise<void> {
    this.currentConfig = cloneConfig(config);
    lastProfileSelection.set(format.id, '__current');

    try {
      await this.deps.saveCurrentConfigForFormat(format, config);
    } catch (error) {
      console.warn('AdvancedSettingsManager: failed to persist config', error);
    }
  }

  private async setupProfiles(format: FormatDefinition): Promise<void> {
    const selectElement = document.getElementById('profileSelect') as HTMLSelectElement | null;
    const btnSave = document.getElementById('profileSaveBtn');
    const btnDelete = document.getElementById('profileDeleteBtn');
    const btnExport = document.getElementById('profileExportBtn');
    const btnImport = document.getElementById('profileImportBtn');
    const inputImport = document.getElementById('profileImportInput') as HTMLInputElement | null;

    await refreshProfileSelect(format, this.currentConfig, lastProfileSelection.get(format.id), true);

    if (selectElement) {
      selectElement.addEventListener('change', async () => {
        if (selectElement.value === '__current') {
          lastProfileSelection.set(format.id, '__current');
          const snapshot = cloneConfig(this.currentConfig) ?? format.getDefaultOptions();
          applyConfigToModalFields(format, snapshot);
          return;
        }

        const profiles = await loadProfiles(format);
        const profile = profiles[selectElement.value];
        if (!profile) {
          return;
        }

        if (profile._formatId && profile._formatId !== format.id) {
          const profileFormat = getFormat(profile._formatId);
          const profileLabel = profileFormat ? t(profileFormat.labelKey) : profile._formatId;
          alert(t('settingsModal.alerts.profileDifferentFormat', { profileFormat: profileLabel }));
          selectElement.value = '__current';
          return;
        }
        lastProfileSelection.set(format.id, selectElement.value);
        applyConfigToModalFields(format, profile);
      });
    }

    if (btnSave) {
      btnSave.addEventListener('click', async () => {
        const name = await showNamePrompt(t('settingsModal.prompts.profileName'), '');
        if (!name) {
          return;
        }

        const config = readConfigFromModal(format);
        const errors = validateConfig(format, config);
        showValidationErrors(errors);
        if (Object.keys(errors).length > 0) {
          alert(t('settingsModal.alerts.fixBeforeSave'));
          return;
        }

        const profiles = await loadProfiles(format);
        const excludedIds = getExcludedFieldIds(format.id);
        const configToSave = stripExcludedFields(config, excludedIds) as FormatOptions;
        const duplicate = findDuplicateProfile(configToSave, profiles);
        if (duplicate) {
          alert(t('settingsModal.alerts.duplicateProfile', { name: duplicate }));
          return;
        }

        profiles[name] = { ...configToSave, _formatId: format.id };
        await saveProfiles(format, profiles);
        lastProfileSelection.set(format.id, name);
        await refreshProfileSelect(format, config, name, true);

        if (selectElement) {
          selectElement.value = name;
        }

        alert(t('settingsModal.alerts.profileSaved', { name }));
      });
    }

    if (btnDelete) {
      btnDelete.addEventListener('click', async () => {
        if (!selectElement || selectElement.value === '__current') {
          return;
        }

        if (!confirm(t('settingsModal.alerts.deleteConfirm', { name: selectElement.value }))) {
          return;
        }
        const target = selectElement.value;
        const profiles = await loadProfiles(format);
        delete profiles[target];
        await saveProfiles(format, profiles);
        if (lastProfileSelection.get(format.id) === target) {
          lastProfileSelection.set(format.id, '__current');
        }
        await refreshProfileSelect(format, null, lastProfileSelection.get(format.id), true);
        alert(t('settingsModal.alerts.profileDeleted', { name: target }));
      });
    }

    if (btnExport) {
      btnExport.addEventListener('click', async () => {
        const config = readConfigFromModal(format);
        const currentProfileName = selectElement && selectElement.value !== '__current' ? selectElement.value : 'current';

        try {
          await exportProfileFile(currentProfileName, { ...config, _formatId: format.id });
          alert(t('settingsModal.alerts.exportSuccess', { name: currentProfileName }));
        } catch (error) {
          const err = error as Error & { canceled?: boolean };
          if (err.canceled) {
            alert(t('settingsModal.alerts.exportCanceled'));
          } else {
            alert(t('settingsModal.alerts.exportFailed', { error: err.message }));
          }
        }
      });
    }

    if (btnImport && inputImport) {
      btnImport.addEventListener('click', () => inputImport.click());
      inputImport.addEventListener('change', async (event) => {
        const target = event.target as HTMLInputElement;
        const file = target.files?.[0];
        if (!file) {
          return;
        }

        try {
          const text = await file.text();
          const json = JSON.parse(text);
          let profileFormat: string | null = null;
          let config: FormatOptions | null = null;

          if (typeof json.version === 'number') {
            if (json.version !== SETTINGS_SCHEMA_VERSION) {
              throw new Error(t('settingsModal.alerts.importVersionMismatch'));
            }
            if (!json.format) {
              throw new Error(t('settingsModal.alerts.importMissingFormat'));
            }
            profileFormat = json.format;
            config = json.config;
          } else {
            config = json;
            profileFormat = json._formatId ?? null;
          }

          if (!profileFormat) {
            throw new Error(t('settingsModal.alerts.importMissingFormat'));
          }

          if (profileFormat !== format.id) {
            const profileFormatObj = getFormat(profileFormat);
            const profileFormatLabel = profileFormatObj ? t(profileFormatObj.labelKey) : profileFormat;
            const currentFormatLabel = getFormatDisplayName(format, (key) => t(key));
            throw new Error(t('settingsModal.alerts.importDifferentFormat', { profileFormat: profileFormatLabel, currentFormat: currentFormatLabel }));
          }

          if (!config || typeof config !== 'object') {
            throw new Error(t('settingsModal.alerts.importInvalid'));
          }

          const profiles = await loadProfiles(format);
          const duplicateName = findDuplicateProfile(config, profiles);
          if (duplicateName) {
            throw new Error(t('settingsModal.alerts.importDuplicate', { name: duplicateName }));
          }

          const name = await showNamePrompt(t('settingsModal.prompts.importedProfileName'), 'imported');
          if (!name) {
            return;
          }

          const excludedIds = getExcludedFieldIds(format.id);
          const configToSave = stripExcludedFields(config, excludedIds) as FormatOptions;
          profiles[name] = { ...configToSave, _formatId: format.id };
          await saveProfiles(format, profiles);
          lastProfileSelection.set(format.id, name);
          await refreshProfileSelect(format, null, name, true);
          if (selectElement) {
            selectElement.value = name;
          }

          applyConfigToModalFields(format, config);
          alert(t('settingsModal.alerts.importSuccess', { name }));
        } catch (error) {
          const err = error as Error;
          alert(t('settingsModal.alerts.importFailed', { error: err.message }));
        } finally {
          target.value = '';
        }
      });
    }
  }

  private setupColorArrayHandlers(): void {
    document.querySelectorAll<HTMLButtonElement>('.btn-add-color').forEach((button) => {
      button.addEventListener('click', () => {
        const fieldId = button.dataset.fieldId;
        if (!fieldId) {
          return;
        }

        const container = document.querySelector<HTMLDivElement>(`.color-array-container[data-field-id="${fieldId}"]`);
        if (!container) {
          return;
        }

        const colors = JSON.parse(container.dataset.colors || '[]') as ColorValue[];
        colors.push({ r: 255, g: 255, b: 255, a: 255 });
        container.dataset.colors = JSON.stringify(colors);
        renderColorArray(container, colors, fieldId);
      });
    });
  }
}

function cloneConfig<T>(config: T): T {
  if (config === null || config === undefined) {
    return config;
  }
  if (typeof structuredClone === 'function') {
    try {
      return structuredClone(config);
    } catch (error) {
      console.warn('structuredClone failed', error);
    }
  }
  try {
    return JSON.parse(JSON.stringify(config));
  } catch (error) {
    console.warn('cloneConfig fallback failed', error);
    return config;
  }
}

function hasReloadRequiredFieldsChanged(format: FormatDefinition, oldConfig: FormatOptions | null, newConfig: FormatOptions): boolean {
  if (!oldConfig) {
    return false;
  }

  for (const section of format.optionsSchema) {
    for (const field of section.fields) {
      if (field.requiresReload) {
        const oldValue = oldConfig[field.id];
        const newValue = newConfig[field.id];
        if (oldValue !== newValue) {
          return true;
        }
      }
    }
  }
  return false;
}

function fieldLabel(field: FormatField): string {
  const labelKey = (field as { labelKey?: string }).labelKey;

  if (labelKey) {
    return t(labelKey);
  }

  return field.id || '';
}

function fieldNote(field: FormatField): string {
  const noteKey = (field as { noteKey?: string }).noteKey;
  if (!noteKey) {
    return '';
  }
  if (noteKey === 'settingsModal.notes.pngxThreads') {
    const numThreads = typeof navigator !== 'undefined' ? navigator.hardwareConcurrency : 1;
    return t(noteKey, { numThreads });
  }
  return t(noteKey);
}

function fieldDefaultValue(field: FormatField): unknown {
  return (field as { defaultValue?: unknown }).defaultValue;
}

function infoText(field: FormatField): string {
  return (field as { textKey?: string }).textKey ? t((field as { textKey?: string }).textKey as string) : '';
}

function buildFieldHTML(field: FormatField, value: unknown): string {
  const idAttr = `id="field_${field.id}"`;
  const common = `data-field-id="${field.id}" data-field-type="${field.type}" aria-describedby="err_${field.id}" ${idAttr}`;
  const labelText = fieldLabel(field);
  const noteText = fieldNote(field);
  const fallbackValue = value ?? fieldDefaultValue(field) ?? '';

  switch (field.type) {
    case 'number': {
      const hardwareConcurrency = typeof navigator !== 'undefined' ? navigator.hardwareConcurrency : 64;
      const maxValue = field.maxIsHardwareConcurrency ? Math.min(field.max ?? hardwareConcurrency, hardwareConcurrency) : field.max;
      return `<div class="config-item" data-wrapper="${field.id}"><label for="field_${field.id}">${labelText ? `${labelText}: ` : ''}</label><input type="number" ${common} value="${fallbackValue ?? ''}" ${field.min !== undefined ? `min='${field.min}'` : ''} ${maxValue !== undefined ? `max='${maxValue}'` : ''} ${field.step !== undefined ? `step='${field.step}'` : ''}>${noteText ? `<div class="field-note">${noteText}</div>` : ''}<div class="field-error" id="err_${field.id}" aria-live="polite"></div></div>`;
    }
    case 'checkbox': {
      const inlineAttr = field.id === 'pngx_lossy_dither_auto' ? ` data-inline-for='pngx_lossy_dither_level'` : '';
      const wrapperClass = field.id === 'pngx_lossy_dither_auto' ? 'config-item range-inline-checkbox' : 'config-item';
      return `<div class="${wrapperClass}" data-wrapper="${field.id}"${inlineAttr}><label><input type="checkbox" ${common} ${Boolean(fallbackValue) ? 'checked' : ''}> ${labelText || field.id}</label>${noteText ? `<div class="field-note">${noteText}</div>` : ''}<div class="field-error" id="err_${field.id}" aria-live="polite"></div></div>`;
    }
    case 'select':
      return `<div class="config-item" data-wrapper="${field.id}"><label for="field_${field.id}">${labelText ? `${labelText}: ` : ''}</label><select ${common}>${(field.options || [])
        .map((option) => {
          const optionLabel = option.labelKey ? t(option.labelKey) : String(option.value);
          const optionValue = String(option.value);
          const selectedValue = fallbackValue != null ? String(fallbackValue) : '';
          return `<option value="${optionValue}" ${optionValue === selectedValue ? 'selected' : ''}>${optionLabel}</option>`;
        })
        .join('')}</select>${noteText ? `<div class="field-note">${noteText}</div>` : ''}<div class="field-error" id="err_${field.id}" aria-live="polite"></div></div>`;
    case 'range': {
      const minAttr = field.min !== undefined ? `min='${field.min}'` : '';
      const maxAttr = field.max !== undefined ? `max='${field.max}'` : '';
      const stepAttr = field.step !== undefined ? `step='${field.step}'` : '';
      const numericalValue = Number.isFinite(Number(fallbackValue)) ? Number(fallbackValue) : (field.min ?? 0);
      const valueAttr = String(numericalValue ?? 0);
      const displayInitial = field.id === 'pngx_lossy_dither_level' ? `${valueAttr}%` : valueAttr;
      return `<div class="config-item" data-wrapper="${field.id}"><label for="field_${field.id}">${labelText ? `${labelText}: ` : ''}</label><div class="range-control" data-range-for="${field.id}"><input type="range" ${common} value="${valueAttr}" ${minAttr} ${maxAttr} ${stepAttr}><span class="range-value" data-range-value-for="${field.id}">${displayInitial}</span><div class="range-inline-slot" data-range-inline-for="${field.id}"></div></div>${noteText ? `<div class="field-note">${noteText}</div>` : ''}<div class="field-error" id="err_${field.id}" aria-live="polite"></div></div>`;
    }
    case 'color-array':
      return `<div class="config-item color-array-item" data-wrapper="${field.id}">
        <label>${labelText ? `${labelText}:` : ''}</label>
        ${noteText ? `<div class="field-note">${noteText}</div>` : ''}
        <div class="color-array-container" ${common}></div>
        <button type="button" class="btn-add-color" data-field-id="${field.id}">${t('settingsModal.addColor')}</button>
        <div class="field-error" id="err_${field.id}" aria-live="polite"></div>
      </div>`;
    case 'info':
      return `<div class="config-item info">${infoText(field)}</div>`;
    default:
      return assertUnreachable(field);
  }
}

function buildModalHTML(format: FormatDefinition, config: FormatOptions): string {
  const formatLabel = getFormatDisplayName(format, (key) => t(key));
  const reloadRequiredFields: { field: FormatField; value: unknown }[] = [];
  const sectionsHTML = (format.optionsSchema || [])
    .map((section) => {
      const sectionLabel = section.labelKey ? t(section.labelKey) : section.section;
      const visibleFields = section.fields.filter((field) => {
        if (field.requiresThreads && !threadsEnabledFlag) {
          return false;
        }
        if (field.requiresReload) {
          reloadRequiredFields.push({ field, value: (config as Record<string, unknown>)[field.id] });
          return false;
        }
        return true;
      });
      if (visibleFields.length === 0) {
        return '';
      }
      const fieldsHTML = visibleFields.map((field) => buildFieldHTML(field, (config as Record<string, unknown>)[field.id])).join('\n');
      return `<div class="config-group" data-section="${section.section}"><h3>${sectionLabel}</h3>${fieldsHTML}</div>`;
    })
    .filter((html) => html.length > 0)
    .join('\n');

  let reloadRequiredSectionHTML = '';
  if (reloadRequiredFields.length > 0) {
    const fieldsHTML = reloadRequiredFields.map(({ field, value }) => buildFieldHTML(field, value)).join('\n');
    reloadRequiredSectionHTML = `
        <div class="reload-required-section">
          <h3>${t('settingsModal.environmentSettings')}</h3>
          <div class="reload-warning">${t('settingsModal.alerts.reloadRequiredWarning')}</div>
          ${fieldsHTML}
        </div>`;
  }

  return `
  <div class="modal" id="advancedModal" role="dialog" aria-modal="true" aria-label="${t('settingsModal.ariaLabel', { format: formatLabel })}">
    <div class="modal-content" tabindex="-1">
      <div class="modal-header">
        <h2>${t('settingsModal.title', { format: formatLabel })}</h2>
        <button class="modal-close" id="advancedModalClose" aria-label="${t('settingsModal.closeAria')}">&times;</button>
      </div>
      <div class="modal-body">${reloadRequiredSectionHTML}
        <div class="profile-section" id="profileSection">
          <label for="profileSelect">${t('settingsModal.profileLabel')}</label>
          <select id="profileSelect" aria-label="${t('settingsModal.profileSelectAria')}">
            <option value="__current">${t('settingsModal.profileCurrent')}</option>
          </select>
          <button class="profile-btn" id="profileSaveBtn">${t('settingsModal.buttons.save')}</button>
          <button class="profile-btn" id="profileDeleteBtn">${t('settingsModal.buttons.delete')}</button>
          <button class="profile-btn" id="profileExportBtn">${t('settingsModal.buttons.export')}</button>
          <button class="profile-btn" id="profileImportBtn">${t('settingsModal.buttons.import')}</button>
          <input type="file" id="profileImportInput" accept=".json" style="display:none" />
        </div>
        <div class="config-grid">${sectionsHTML}</div>
      </div>
      <div class="modal-footer">
        <button class="btn-secondary" id="advancedResetBtn">${t('settingsModal.buttons.reset')}</button>
        <button class="btn-primary" id="advancedApplyBtn">${t('settingsModal.buttons.apply')}</button>
      </div>
    </div>
  </div>`;
}

function readConfigFromModal(format: FormatDefinition): FormatOptions {
  const result: Record<string, unknown> = {};

  (format.optionsSchema || []).forEach((section) =>
    section.fields.forEach((field) => {
      if (!field.id) {
        return;
      }

      const element = document.querySelector(`[data-field-id="${field.id}"]`) as HTMLElement | null;
      if (!element) {
        return;
      }

      switch (field.type) {
        case 'checkbox':
          result[field.id] = (element as HTMLInputElement).checked;
          break;
        case 'number': {
          const input = element as HTMLInputElement;
          result[field.id] = input.value === '' ? null : Number(input.value);
          break;
        }
        case 'range': {
          const input = element as HTMLInputElement;
          result[field.id] = input.value === '' ? null : Number(input.value);
          break;
        }
        case 'select':
          result[field.id] = (element as HTMLSelectElement).value;
          break;
        case 'color-array': {
          const container = element as HTMLDivElement;
          result[field.id] = JSON.parse(container.dataset.colors || '[]');
          break;
        }
        case 'info':
          break;
        default:
          assertUnreachable(field);
      }
    })
  );

  if (typeof result['pngx_lossy_dither_level'] === 'number' && Number.isFinite(result['pngx_lossy_dither_level'] as number)) {
    const value = result['pngx_lossy_dither_level'] as number;
    result['pngx_lossy_dither_level'] = Math.min(100, Math.max(0, Math.round(value)));
  }

  const lossyTypeValue = Number((result['pngx_lossy_type'] as string) ?? 0);
  const ditherAuto = Boolean(result['pngx_lossy_dither_auto']);
  if (lossyTypeValue === 1 && ditherAuto) {
    result['pngx_lossy_dither_level'] = -1;
  }

  return format.normalizeOptions ? format.normalizeOptions(result) : result;
}

function applyConfigToModalFields(format: FormatDefinition, config: FormatOptions): void {
  (format.optionsSchema || []).forEach((section) =>
    section.fields.forEach((field) => {
      if (!field.id) {
        return;
      }

      const element = document.querySelector(`[data-field-id="${field.id}"]`) as HTMLElement | null;
      if (!element) {
        return;
      }

      switch (field.type) {
        case 'checkbox':
          (element as HTMLInputElement).checked = Boolean((config as any)[field.id]);
          break;
        case 'number': {
          const input = element as HTMLInputElement;
          const value = (config as any)[field.id];

          input.value = value == null ? '' : String(value);
          break;
        }
        case 'range': {
          const input = element as HTMLInputElement;
          const rawValue = (config as any)[field.id];
          const numericValue = typeof rawValue === 'number' ? rawValue : Number(rawValue);
          const min = (field as { min?: number }).min ?? 0;
          const max = (field as { max?: number }).max ?? 100;
          let clamped = Number.isFinite(numericValue) ? numericValue : min;

          if (clamped < min) {
            clamped = min;
          }
          if (clamped > max) {
            clamped = max;
          }
          clamped = Math.round(clamped);

          input.value = String(clamped);
          syncRangeValueDisplay(input);

          break;
        }
        case 'select':
          (element as HTMLSelectElement).value = String((config as any)[field.id]);
          break;
        case 'color-array': {
          const container = element as HTMLDivElement;
          const colors = ((config as any)[field.id] as ColorValue[]) || [];
          container.dataset.colors = JSON.stringify(colors);
          renderColorArray(container, colors, field.id);
          break;
        }
        case 'info':
          break;
        default:
          assertUnreachable(field);
      }
    })
  );

  updatePngxLossyMaxColorsState();
  ensureRangeFieldHandlers();
}

function updatePngxLossyMaxColorsState(userInitiated = false): void {
  const typeElement = document.querySelector<HTMLSelectElement>("[data-field-id='pngx_lossy_type']");
  const maxColorsInput = document.querySelector<HTMLInputElement>("[data-field-id='pngx_lossy_max_colors']");
  const ditherInput = document.querySelector<HTMLInputElement>("[data-field-id='pngx_lossy_dither_level']");
  const ditherAutoInput = document.querySelector<HTMLInputElement>("[data-field-id='pngx_lossy_dither_auto']");
  if (!typeElement) {
    return;
  }
  const paletteSelected = typeElement.value === '0';
  const limitedSelected = typeElement.value === '1';
  const reducedSelected = typeElement.value === '2';
  const palette256Section = document.querySelector<HTMLDivElement>(".config-group[data-section='palette256']");
  if (palette256Section) {
    palette256Section.hidden = !paletteSelected;
    palette256Section.style.display = paletteSelected ? '' : 'none';
    palette256Section.setAttribute('aria-hidden', paletteSelected ? 'false' : 'true');
  }

  const palette256OnlyFields = [
    'pngx_palette256_gradient_profile_enable',
    'pngx_palette256_gradient_dither_floor',
    'pngx_palette256_alpha_bleed_enable',
    'pngx_palette256_alpha_bleed_max_distance',
    'pngx_palette256_alpha_bleed_opaque_threshold',
    'pngx_palette256_alpha_bleed_soft_limit',
    'pngx_palette256_profile_opaque_ratio_threshold',
    'pngx_palette256_profile_gradient_mean_max',
    'pngx_palette256_profile_saturation_mean_max',
    'pngx_palette256_tune_opaque_ratio_threshold',
    'pngx_palette256_tune_gradient_mean_max',
    'pngx_palette256_tune_saturation_mean_max',
    'pngx_palette256_tune_speed_max',
    'pngx_palette256_tune_quality_min_floor',
    'pngx_palette256_tune_quality_max_target',
  ];

  palette256OnlyFields.forEach((id) => {
    const input = document.querySelector<HTMLInputElement>(`[data-field-id='${id}']`);

    if (!input) {
      return;
    }
    input.disabled = !paletteSelected;

    if (paletteSelected) {
      input.removeAttribute('aria-disabled');
    } else {
      input.setAttribute('aria-disabled', 'true');
    }

    const wrapper = document.querySelector<HTMLDivElement>(`.config-item[data-wrapper='${id}']`);
    if (!wrapper) {
      return;
    }

    wrapper.hidden = !paletteSelected;
    wrapper.style.display = paletteSelected ? '' : 'none';
    wrapper.classList.toggle('disabled', !paletteSelected);
  });

  if (limitedSelected && userInitiated && ditherAutoInput && !ditherAutoInput.checked) {
    ditherAutoInput.checked = true;
  }
  if (maxColorsInput) {
    const maxColorsActive = paletteSelected || reducedSelected;

    maxColorsInput.disabled = !maxColorsActive;
    if (maxColorsActive) {
      maxColorsInput.removeAttribute('aria-disabled');
    } else {
      maxColorsInput.setAttribute('aria-disabled', 'true');
    }

    const min = reducedSelected ? 1 : 2;
    const max = reducedSelected ? 32768 : 256;
    maxColorsInput.min = String(min);
    maxColorsInput.max = String(max);

    if (maxColorsActive) {
      if (reducedSelected && userInitiated) {
        maxColorsInput.value = '1';
        maxColorsInput.dispatchEvent(new Event('input', { bubbles: true }));
      }

      const currentValue = Number(maxColorsInput.value);
      if (!Number.isFinite(currentValue)) {
        maxColorsInput.value = String(min);
      } else {
        const clamped = Math.min(max, Math.max(min, Math.trunc(currentValue)));
        if (clamped !== currentValue) {
          maxColorsInput.value = String(clamped);
        }
      }
    }

    const wrapper = document.querySelector<HTMLDivElement>(".config-item[data-wrapper='pngx_lossy_max_colors']");
    if (wrapper) {
      wrapper.hidden = limitedSelected;
      wrapper.classList.toggle('disabled', !maxColorsActive);

      const noteElement = wrapper.querySelector<HTMLDivElement>('.field-note');
      if (noteElement) {
        let noteKey = 'settingsModal.notes.pngxMaxColorsBitdepth';
        if (paletteSelected) {
          noteKey = 'settingsModal.notes.pngxMaxColorsPalette';
        } else if (reducedSelected) {
          noteKey = 'settingsModal.notes.pngxMaxColorsReduced';
        }
        noteElement.textContent = t(noteKey);
      }

      const labelElement = wrapper.querySelector<HTMLLabelElement>('label');
      if (labelElement) {
        let labelKey = 'settingsModal.labels.pngxMaxColorsBase';
        if (paletteSelected) {
          labelKey = 'settingsModal.labels.pngxMaxColorsPalette';
        } else if (reducedSelected) {
          labelKey = 'settingsModal.labels.pngxMaxColorsReduced';
        }
        labelElement.textContent = `${t(labelKey)}: `;
      }
    }
  }

  const reducedBitFields = [
    { id: 'pngx_lossy_reduced_bits_rgb', noteKey: 'settingsModal.notes.pngxReducedBitsRgb' },
    { id: 'pngx_lossy_reduced_alpha_bits', noteKey: 'settingsModal.notes.pngxReducedAlphaBits' },
  ];

  reducedBitFields.forEach(({ id, noteKey }) => {
    const input = document.querySelector<HTMLInputElement>(`[data-field-id='${id}']`);
    if (!input) {
      return;
    }
    input.disabled = !reducedSelected;
    if (reducedSelected) {
      input.removeAttribute('aria-disabled');
    } else {
      input.setAttribute('aria-disabled', 'true');
    }
    const wrapper = document.querySelector<HTMLDivElement>(`.config-item[data-wrapper='${id}']`);
    if (wrapper) {
      wrapper.hidden = !reducedSelected;
      wrapper.classList.toggle('disabled', !reducedSelected);
      const noteElement = wrapper.querySelector<HTMLDivElement>('.field-note');
      if (noteElement) {
        noteElement.textContent = t(noteKey);
      }
    }
  });

  const ditherWrapper = document.querySelector<HTMLDivElement>(".config-item[data-wrapper='pngx_lossy_dither_level']");
  const ditherAutoWrapper = document.querySelector<HTMLDivElement>(".config-item[data-wrapper='pngx_lossy_dither_auto']");
  const ditherInlineSlot = document.querySelector<HTMLDivElement>(".range-control[data-range-for='pngx_lossy_dither_level'] .range-inline-slot");
  const autoDitherActive = Boolean(ditherAutoInput?.checked) && limitedSelected;

  if (ditherInlineSlot) {
    ditherInlineSlot.hidden = !limitedSelected;
    ditherInlineSlot.style.display = limitedSelected ? '' : 'none';
  }

  if (ditherAutoWrapper) {
    ditherAutoWrapper.hidden = !limitedSelected;
    ditherAutoWrapper.style.display = limitedSelected ? '' : 'none';
    ditherAutoWrapper.classList.toggle('disabled', !limitedSelected);
  }

  if (ditherWrapper) {
    let noteElement = ditherWrapper.querySelector<HTMLDivElement>('.field-note');
    if (!noteElement) {
      noteElement = document.createElement('div');
      noteElement.classList.add('field-note');
      ditherWrapper.appendChild(noteElement);
    }
    if (limitedSelected) {
      noteElement.textContent = t('settingsModal.notes.pngxDitherLimitedAuto');
      noteElement.classList.add('warning');
    } else if (reducedSelected) {
      noteElement.textContent = t('settingsModal.notes.pngxDitherReducedUnused');
      noteElement.classList.remove('warning');
    } else {
      noteElement.textContent = t('settingsModal.notes.pngxDitherDefault');
      noteElement.classList.remove('warning');
    }
    if (ditherInput) {
      const shouldDisable = autoDitherActive;
      ditherInput.disabled = shouldDisable;
      if (shouldDisable) {
        ditherInput.setAttribute('aria-disabled', 'true');
        if (ditherInput.value !== '0') {
          ditherInput.value = '0';
          ditherInput.dispatchEvent(new Event('input', { bubbles: true }));
        } else {
          syncRangeValueDisplay(ditherInput);
        }
      } else {
        ditherInput.removeAttribute('aria-disabled');
        syncRangeValueDisplay(ditherInput);
      }
    }
  }

  if (!limitedSelected && ditherAutoInput && ditherAutoInput.checked) {
    ditherAutoInput.checked = false;
    if (ditherInput) {
      syncRangeValueDisplay(ditherInput);
    }
  }
}

function validateConfig(format: FormatDefinition, config: FormatOptions): Record<string, string> {
  const errors: Record<string, string> = {};
  const configRecord = config as Record<string, unknown>;
  const ditherAutoEnabled = Boolean(configRecord['pngx_lossy_dither_auto']);
  const hardwareConcurrency = typeof navigator !== 'undefined' ? navigator.hardwareConcurrency : 64;
  (format.optionsSchema || []).forEach((section) =>
    section.fields.forEach((field) => {
      if (!field.id) {
        return;
      }
      const value = (config as any)[field.id];
      if (field.type === 'number' || field.type === 'range') {
        if (value == null || Number.isNaN(Number(value))) {
          errors[field.id] = t('settingsModal.validation.numberRequired');
          return;
        }
        const skipMinValidation = field.id === 'pngx_lossy_dither_level' && ditherAutoEnabled && value === -1;
        if (field.min !== undefined && value < field.min && !skipMinValidation) {
          errors[field.id] = t('settingsModal.validation.min', { min: field.min });
        }
        if (!errors[field.id] && field.type === 'number') {
          const effectiveMax = field.maxIsHardwareConcurrency ? Math.min(field.max ?? hardwareConcurrency, hardwareConcurrency) : field.max;
          if (effectiveMax !== undefined && value > effectiveMax) {
            errors[field.id] = t('settingsModal.validation.max', { max: effectiveMax });
          }
        } else if (!errors[field.id] && field.max !== undefined && value > field.max) {
          errors[field.id] = t('settingsModal.validation.max', { max: field.max });
        }
      }
    })
  );
  return errors;
}

function showValidationErrors(errors: Record<string, string>): void {
  document.querySelectorAll('.config-item.error').forEach((node) => node.classList.remove('error'));
  Object.entries(errors).forEach(([fieldId, message]) => {
    const wrapper = document.querySelector(`.config-item[data-wrapper='${fieldId}']`);
    const errorElement = document.getElementById(`err_${fieldId}`);
    if (wrapper) {
      wrapper.classList.add('error');
    }
    if (errorElement) {
      errorElement.textContent = message;
    }
  });
}

async function loadProfiles(format: FormatDefinition): Promise<ProfileDictionary> {
  const stored = await Storage.getItem<ProfileDictionary | (FormatOptions & { _formatId?: string })>(`profiles_${format.id}`);
  if (!stored) {
    return {};
  }

  if (!('_formatId' in stored)) {
    return stored as ProfileDictionary;
  }

  return {};
}

async function saveProfiles(format: FormatDefinition, profiles: ProfileDictionary): Promise<void> {
  const normalized: ProfileDictionary = {};
  Object.keys(profiles).forEach((name) => {
    const profile = profiles[name] || {};
    normalized[name] = {
      ...profile,
      _schemaVersion: SETTINGS_SCHEMA_VERSION,
    };
  });
  await Storage.setItem(`profiles_${format.id}`, normalized);
}

async function refreshProfileSelect(format: FormatDefinition, currentConfig: FormatOptions | null, selectedName?: string | null, forceSelect = false): Promise<void> {
  const selectElement = document.getElementById('profileSelect') as HTMLSelectElement | null;
  if (!selectElement) {
    return;
  }
  const previousValue = selectElement.value;
  selectElement.innerHTML = `<option value="__current">${t('settingsModal.profileCurrent')}</option>`;
  const profiles = await loadProfiles(format);
  Object.keys(profiles).forEach((name) => {
    const profile = profiles[name];
    if (profile._formatId && profile._formatId !== format.id) {
      return;
    }
    const option = document.createElement('option');
    option.value = name;
    option.textContent = name;
    selectElement.appendChild(option);
  });

  const targetValue = forceSelect ? selectedName : selectedName || previousValue;
  if (targetValue && profiles[targetValue]) {
    selectElement.value = targetValue;
  } else {
    selectElement.value = '__current';
  }

  if (currentConfig) {
    lastProfileSelection.set(format.id, selectElement.value);
  }
}

function areConfigsEqual(config1: FormatOptions, config2: FormatOptions): boolean {
  if (!config1 || !config2) {
    return false;
  }

  const keys1 = Object.keys(config1)
    .filter((key) => key !== '_formatId')
    .sort();
  const keys2 = Object.keys(config2)
    .filter((key) => key !== '_formatId')
    .sort();

  if (keys1.length !== keys2.length) {
    return false;
  }

  if (keys1.some((key, index) => key !== keys2[index])) {
    return false;
  }

  return keys1.every((key) => {
    const value1 = (config1 as any)[key];
    const value2 = (config2 as any)[key];

    if (value1 == null && value2 == null) {
      return true;
    }
    if (value1 == null || value2 == null) {
      return false;
    }
    if (typeof value1 !== typeof value2) {
      return false;
    }
    if (typeof value1 === 'object') {
      return JSON.stringify(value1) === JSON.stringify(value2);
    }

    return value1 === value2;
  });
}

function findDuplicateProfile(config: FormatOptions, profiles: ProfileDictionary): string | null {
  for (const [name, profile] of Object.entries(profiles)) {
    if (areConfigsEqual(config, profile)) {
      return name;
    }
  }

  return null;
}

function getExcludedFieldIds(formatId: string): Set<string> {
  const format = getFormat(formatId);
  const excluded = new Set<string>();

  if (!format) {
    return new Set();
  }

  (format.optionsSchema || []).forEach((section) => {
    section.fields.forEach((field) => {
      if (field.excludeFromExport) {
        excluded.add(field.id);
      }
    });
  });

  return excluded;
}

function stripExcludedFields(data: FormatOptions, excludedIds: Set<string>): FormatOptions {
  const result: FormatOptions = {};

  if (excludedIds.size === 0) {
    return data;
  }

  Object.keys(data).forEach((key) => {
    if (!excludedIds.has(key)) {
      result[key] = data[key];
    }
  });

  return result;
}

async function exportProfileFile(name: string, data: FormatOptions & { _formatId?: string }): Promise<void> {
  const excludedIds = data._formatId ? getExcludedFieldIds(data._formatId) : new Set<string>();
  const exportData = stripExcludedFields(data, excludedIds);
  const jsonContent = JSON.stringify({ version: SETTINGS_SCHEMA_VERSION, format: data._formatId, config: exportData }, null, 2);
  const defaultFileName = `colopresso-${data._formatId}-profile-${name}.json`;

  const electron = window.electronAPI;
  if (electron?.saveJsonDialog && electron.writeFile) {
    try {
      const saveResult = await electron.saveJsonDialog(defaultFileName);
      if (saveResult.canceled) {
        const cancelError = new Error('Export canceled by user');
        (cancelError as Error & { canceled?: boolean }).canceled = true;
        throw cancelError;
      }
      if (!saveResult.success) {
        throw new Error(saveResult.error || 'Failed to show save dialog');
      }
      if (!saveResult.filePath) {
        throw new Error('Save dialog did not provide a file path');
      }
      const encoder = new TextEncoder();
      const writeResult = await electron.writeFile(saveResult.filePath, encoder.encode(jsonContent));
      if (!writeResult.success) {
        throw new Error('Failed to write file: ' + (writeResult.error ?? 'unknown'));
      }
    } catch (error) {
      throw error;
    }
  } else {
    const blob = new Blob([jsonContent], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement('a');
    anchor.href = url;
    anchor.download = defaultFileName;
    anchor.click();
    URL.revokeObjectURL(url);
  }
}

function openModalWithFocusTrap(modal: HTMLElement): void {
  modal.classList.add('active');
  const content = modal.querySelector<HTMLElement>('.modal-content');
  if (!content) {
    return;
  }
  const focusables = getFocusable(content);
  const previouslyFocused = document.activeElement as HTMLElement | null;
  content.dataset.prevFocus = previouslyFocused ? previouslyFocused.id || '___prevfocus___' : '___none___';
  if (focusables.length > 0) {
    focusables[0].focus();
  } else {
    content.setAttribute('tabindex', '-1');
    content.focus();
  }

  function handleKey(event: KeyboardEvent): void {
    if (event.key === 'Escape') {
      if (document.getElementById('namePromptOverlay')) {
        return;
      }
      closeModal(modal);
    }
    if (event.key === 'Enter') {
      const target = event.target as HTMLElement | null;
      const tagName = target?.tagName?.toLowerCase();
      if (tagName === 'button' || tagName === 'select') {
        return;
      }
      if (document.getElementById('namePromptOverlay')) {
        return;
      }
      const btnApply = document.getElementById('advancedApplyBtn') as HTMLButtonElement | null;
      if (btnApply) {
        event.preventDefault();
        btnApply.click();
      }
    }
    if (event.key === 'Tab') {
      if (focusables.length === 0) {
        event.preventDefault();
        return;
      }
      const index = focusables.indexOf(document.activeElement as HTMLElement);
      if (event.shiftKey) {
        const nextIndex = index <= 0 ? focusables.length - 1 : index - 1;
        event.preventDefault();
        focusables[nextIndex].focus();
      } else {
        const nextIndex = index === focusables.length - 1 ? 0 : index + 1;
        event.preventDefault();
        focusables[nextIndex].focus();
      }
    }
  }

  (modal as any)._trapHandler = handleKey;
  document.addEventListener('keydown', handleKey, true);
}

function closeModal(modal: HTMLElement): void {
  modal.classList.remove('active');
  const handler = (modal as any)._trapHandler as ((event: KeyboardEvent) => void) | undefined;
  if (handler) {
    document.removeEventListener('keydown', handler, true);
    delete (modal as any)._trapHandler;
  }
  const content = modal.querySelector<HTMLElement>('.modal-content');
  const previousId = content?.dataset.prevFocus;
  if (previousId && previousId !== '___none___') {
    const previousElement = document.getElementById(previousId);
    previousElement?.focus();
  } else {
    const openButton = document.getElementById('advancedSettingsBtn');
    openButton?.focus();
  }
}

function getFocusable(root: HTMLElement): HTMLElement[] {
  return Array.from(root.querySelectorAll<HTMLElement>('button, [href], input, select, textarea, [tabindex]:not([tabindex="-1"])')).filter(
    (element) => !element.hasAttribute('disabled') && element.offsetParent !== null
  );
}

async function showNamePrompt(title: string, defaultValue = ''): Promise<string | null> {
  return new Promise((resolve) => {
    const existing = document.getElementById('namePromptOverlay');
    if (existing) {
      existing.remove();
    }
    const overlay = document.createElement('div');
    overlay.id = 'namePromptOverlay';
    overlay.className = 'name-prompt-overlay';
    const cancelLabel = t('settingsModal.prompts.cancel');
    const okLabel = t('settingsModal.prompts.ok');
    const safeTitle = title || '';
    const safeValue = (defaultValue || '').replace(/"/g, '&quot;');
    overlay.innerHTML = `<div role="dialog" aria-modal="true" class="name-prompt-dialog">
      <h3 class="name-prompt-title">${safeTitle}</h3>
      <input type="text" id="namePromptInput" class="name-prompt-input" value="${safeValue}">
      <div class="name-prompt-actions">
        <button type="button" id="namePromptCancel" class="btn-secondary">${cancelLabel}</button>
        <button type="button" id="namePromptOk" class="btn-primary">${okLabel}</button>
      </div>
    </div>`;
    document.body.appendChild(overlay);

    const input = overlay.querySelector<HTMLInputElement>('#namePromptInput');
    const okButton = overlay.querySelector<HTMLButtonElement>('#namePromptOk');
    const cancelButton = overlay.querySelector<HTMLButtonElement>('#namePromptCancel');

    input?.focus();
    input?.select();

    function cleanup(result: string | null): void {
      document.removeEventListener('keydown', keyHandler, true);
      overlay.remove();
      resolve(result);
    }

    function keyHandler(event: KeyboardEvent): void {
      if (event.key === 'Escape') {
        event.preventDefault();
        cleanup(null);
        return;
      }
      if (event.key === 'Enter') {
        event.preventDefault();
        submit();
        return;
      }
      if (event.key === 'Tab') {
        const focusables = [input, cancelButton, okButton].filter((element): element is HTMLInputElement | HTMLButtonElement => Boolean(element));
        if (focusables.length === 0) {
          return;
        }
        let index = focusables.indexOf(document.activeElement as HTMLInputElement | HTMLButtonElement);
        if (index < 0) {
          index = 0;
        } else if (event.shiftKey) {
          index = index <= 0 ? focusables.length - 1 : index - 1;
        } else {
          index = (index + 1) % focusables.length;
        }
        event.preventDefault();
        focusables[index].focus();
      }
    }

    function submit(): void {
      const value = (input?.value || '').trim();
      cleanup(value || null);
    }

    okButton?.addEventListener('click', submit);
    cancelButton?.addEventListener('click', () => cleanup(null));
    document.addEventListener('keydown', keyHandler, true);
    overlay.addEventListener('click', (event) => {
      if (event.target === overlay) {
        cleanup(null);
      }
    });
  });
}

function renderColorArray(container: HTMLDivElement, colors: ColorValue[], fieldId: string): void {
  container.innerHTML = '';
  colors.forEach((color, index) => {
    const colorItem = document.createElement('div');
    colorItem.className = 'color-item';
    const hexValue = rgbToHex(color.r, color.g, color.b).substring(1);
    const fullHex = rgbToHex(color.r, color.g, color.b);
    const textColor = getContrastTextColor(color.r, color.g, color.b);

    colorItem.innerHTML = `
      <div class="color-preview" style="background-color: ${fullHex}; color: ${textColor};" data-index="${index}">
        ${fullHex}
      </div>
      <span class="hex-prefix">#</span>
      <input type="text" value="${hexValue}" data-index="${index}" class="hex-input" maxlength="6" placeholder="RRGGBB">
      <input type="range" min="0" max="255" value="${color.a}" data-index="${index}" class="alpha-slider">
      <span class="alpha-value">${color.a}</span>
      <button type="button" class="btn-remove-color" data-index="${index}">×</button>
    `;
    container.appendChild(colorItem);

    const hexInput = colorItem.querySelector<HTMLInputElement>('.hex-input');
    const alphaSlider = colorItem.querySelector<HTMLInputElement>('.alpha-slider');
    const alphaValue = colorItem.querySelector<HTMLSpanElement>('.alpha-value');
    const removeButton = colorItem.querySelector<HTMLButtonElement>('.btn-remove-color');

    const updatePreview = () => {
      if (!hexInput) {
        return;
      }
      const currentHex = hexInput.value.trim();
      const fullCurrentHex = currentHex.startsWith('#') ? currentHex : `#${currentHex}`;
      if (currentHex.length === 6) {
        const rgb = hexToRgb(fullCurrentHex);
        const preview = colorItem.querySelector<HTMLDivElement>('.color-preview');
        if (preview) {
          const newTextColor = getContrastTextColor(rgb.r, rgb.g, rgb.b);
          preview.style.backgroundColor = fullCurrentHex;
          preview.style.color = newTextColor;
          preview.textContent = fullCurrentHex;
        }
      }
    };

    hexInput?.addEventListener('input', (event) => {
      const inputElement = event.target as HTMLInputElement;
      inputElement.value = inputElement.value.replace(/[^0-9A-Fa-f]/g, '').toUpperCase();
      if (inputElement.value.length === 6) {
        updatePreview();
        updateColorArray(container, fieldId);
      }
    });

    hexInput?.addEventListener('blur', (event) => {
      const inputElement = event.target as HTMLInputElement;
      if (inputElement.value.length !== 6) {
        const original = colors[index];
        inputElement.value = rgbToHex(original.r, original.g, original.b).substring(1);
        updatePreview();
      } else {
        updateColorArray(container, fieldId);
      }
    });

    alphaSlider?.addEventListener('input', (event) => {
      const slider = event.target as HTMLInputElement;
      if (alphaValue) {
        alphaValue.textContent = slider.value;
      }
      updateColorArray(container, fieldId);
    });

    removeButton?.addEventListener('click', () => {
      colors.splice(index, 1);
      container.dataset.colors = JSON.stringify(colors);
      renderColorArray(container, colors, fieldId);
    });
  });
}

function syncRangeValueDisplay(input: HTMLInputElement): void {
  const fieldId = input.dataset.fieldId;
  if (!fieldId) {
    return;
  }
  const display = document.querySelector<HTMLSpanElement>(`.range-value[data-range-value-for='${fieldId}']`);
  if (!display) {
    return;
  }
  if (fieldId === 'pngx_lossy_dither_level') {
    const typeElement = document.querySelector<HTMLSelectElement>("[data-field-id='pngx_lossy_type']");
    const limitedSelected = typeElement?.value === '1';
    const autoInput = document.querySelector<HTMLInputElement>("[data-field-id='pngx_lossy_dither_auto']");
    const autoActive = Boolean(autoInput?.checked) && limitedSelected;
    display.textContent = autoActive ? '-1' : `${Math.round(Number(input.value) || 0)}%`;
    return;
  }
  display.textContent = input.value;
}

function mountRangeInlineControls(): void {
  document.querySelectorAll<HTMLElement>('.config-item[data-inline-for]').forEach((inlineItem) => {
    const targetId = inlineItem.dataset.inlineFor;
    if (!targetId) {
      return;
    }
    const slot = document.querySelector<HTMLDivElement>(`.range-control[data-range-for='${targetId}'] .range-inline-slot`);
    if (!slot) {
      return;
    }
    if (inlineItem.parentElement !== slot) {
      slot.appendChild(inlineItem);
    }
  });
}

function ensureRangeFieldHandlers(): void {
  mountRangeInlineControls();
  document.querySelectorAll<HTMLInputElement>("input[data-field-type='range']").forEach((input) => {
    if (!input.dataset.rangeBound) {
      input.addEventListener('input', () => syncRangeValueDisplay(input));
      input.dataset.rangeBound = 'true';
    }
    syncRangeValueDisplay(input);
  });
}

function updateColorArray(container: HTMLDivElement, fieldId: string): void {
  const colors: ColorValue[] = [];
  container.querySelectorAll<HTMLDivElement>('.color-item').forEach((item) => {
    const hexInput = item.querySelector<HTMLInputElement>('.hex-input');
    const alphaSlider = item.querySelector<HTMLInputElement>('.alpha-slider');
    if (!hexInput || !alphaSlider) {
      return;
    }
    const hexValue = hexInput.value.trim();
    const fullHex = hexValue.startsWith('#') ? hexValue : `#${hexValue}`;
    const rgb = hexToRgb(fullHex);
    colors.push({ r: rgb.r, g: rgb.g, b: rgb.b, a: parseInt(alphaSlider.value, 10) });
  });
  container.dataset.colors = JSON.stringify(colors);
}

function rgbToHex(r: number, g: number, b: number): string {
  return (
    '#' +
    [r, g, b]
      .map((component) => {
        const hex = component.toString(16);
        return hex.length === 1 ? `0${hex}` : hex;
      })
      .join('')
  );
}

function hexToRgb(hex: string): { r: number; g: number; b: number } {
  const result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
  if (!result) {
    return { r: 0, g: 0, b: 0 };
  }
  return {
    r: parseInt(result[1], 16),
    g: parseInt(result[2], 16),
    b: parseInt(result[3], 16),
  };
}

function getContrastTextColor(r: number, g: number, b: number): string {
  const luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255;
  return luminance > 0.5 ? '#000000' : '#FFFFFF';
}

function assertUnreachable(_value: never): never {
  throw new Error('Unsupported field type encountered in advanced settings modal');
}
