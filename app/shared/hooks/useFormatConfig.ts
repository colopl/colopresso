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

import { useCallback, useMemo, useRef, useState } from 'react';
import { useAppDispatch, useAppState } from '../state/appState';
import { getFormat, getDefaultFormat, activateFormat } from '../formats';
import { loadFormatConfig, saveFormatConfig, saveSelectedFormatId } from '../core/configStore';
import { AdvancedSettingsController, setupFormatAwareAdvancedSettings } from '../core/advancedSettingsModal';
import { FormatDefinition, FormatOptions } from '../types';

export interface UseFormatConfigResult {
  formats: FormatDefinition[];
  currentFormat: FormatDefinition;
  storedFormatConfig: FormatOptions | undefined;
  currentConfig: FormatOptions;
  formatConfigs: Record<string, FormatOptions>;
  setFormatConfigs: React.Dispatch<React.SetStateAction<Record<string, FormatOptions>>>;
  handleFormatChange: (formatId: string) => Promise<void>;
  persistFormatConfig: (format: FormatDefinition, config: FormatOptions) => Promise<void>;
  initializeAdvancedSettings: (format: FormatDefinition, config: FormatOptions) => Promise<AdvancedSettingsController | null>;
  handleAdvancedSettingsClick: () => Promise<void>;
  advancedSettingsControllerRef: React.MutableRefObject<AdvancedSettingsController | null>;
  threadsEnabledRef: React.MutableRefObject<boolean>;
}

export function useFormatConfig(): UseFormatConfigResult {
  const state = useAppState();
  const dispatch = useAppDispatch();

  const [formats] = useState<FormatDefinition[]>(() => {
    const { getFormats } = require('../formats');
    const order = ['webp', 'avif', 'pngx'];
    return getFormats()
      .slice()
      .sort((a: FormatDefinition, b: FormatDefinition) => order.indexOf(a.id) - order.indexOf(b.id));
  });

  const [formatConfigs, setFormatConfigs] = useState<Record<string, FormatOptions>>({});

  const advancedSettingsControllerRef = useRef<AdvancedSettingsController | null>(null);
  const latestAdvancedFormatRef = useRef<string | null>(null);
  const latestAdvancedConfigRef = useRef<FormatOptions | null>(null);
  const threadsEnabledRef = useRef<boolean>(true);

  const currentFormat = useMemo(() => getFormat(state.activeFormatId) ?? getDefaultFormat(), [state.activeFormatId]);

  const storedFormatConfig = formatConfigs[currentFormat.id];
  const currentConfig = useMemo(() => storedFormatConfig ?? currentFormat.getDefaultOptions(), [currentFormat, storedFormatConfig]);

  const handleFormatChange = useCallback(
    async (formatId: string) => {
      const format = getFormat(formatId);
      if (!format) {
        return;
      }

      activateFormat(formatId);
      dispatch({ type: 'setActiveFormat', id: formatId });
      await saveSelectedFormatId(formatId);
      if (!formatConfigs[formatId]) {
        const config = await loadFormatConfig(format);
        setFormatConfigs((prev) => ({ ...prev, [formatId]: config }));
      }
    },
    [dispatch, formatConfigs]
  );

  const persistFormatConfig = useCallback(async (format: FormatDefinition, config: FormatOptions) => {
    setFormatConfigs((prev) => ({ ...prev, [format.id]: config }));
    try {
      await saveFormatConfig(format, config);
    } catch {
      /* NOP */
    }
  }, []);

  const initializeAdvancedSettings = useCallback(
    async (format: FormatDefinition, config: FormatOptions): Promise<AdvancedSettingsController | null> => {
      if (latestAdvancedFormatRef.current === format.id && latestAdvancedConfigRef.current === config && advancedSettingsControllerRef.current) {
        return advancedSettingsControllerRef.current;
      }
      try {
        const controller = await setupFormatAwareAdvancedSettings(
          format,
          config,
          () => {
            /* NOP */
          },
          {
            getDefaultConfigForFormat: (target) => target.getDefaultOptions(),
            saveCurrentConfigForFormat: async (target, nextConfig) => {
              await persistFormatConfig(target, nextConfig);
            },
            isThreadsEnabled: threadsEnabledRef.current,
          }
        );
        advancedSettingsControllerRef.current = controller;
        latestAdvancedFormatRef.current = format.id;
        latestAdvancedConfigRef.current = config;
        return controller;
      } catch (_error) {
        return null;
      }
    },
    [persistFormatConfig]
  );

  const handleAdvancedSettingsClick = useCallback(async () => {
    if (state.isProcessing) {
      return;
    }
    const configForModal = storedFormatConfig ?? currentFormat.getDefaultOptions();
    const controller = await initializeAdvancedSettings(currentFormat, configForModal);
    if (controller && typeof controller.open === 'function') {
      await controller.open();
    }
  }, [currentFormat, initializeAdvancedSettings, state.isProcessing, storedFormatConfig]);

  return {
    formats,
    currentFormat,
    storedFormatConfig,
    currentConfig,
    formatConfigs,
    setFormatConfigs,
    handleFormatChange,
    persistFormatConfig,
    initializeAdvancedSettings,
    handleAdvancedSettingsClick,
    advancedSettingsControllerRef,
    threadsEnabledRef,
  };
}
