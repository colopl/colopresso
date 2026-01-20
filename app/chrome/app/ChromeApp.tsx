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

import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import styles from '../../shared/styles/App.module.css';
import { AppStateProvider, useAppDispatch, useAppState } from '../../shared/state/appState';
import { I18nProvider, useI18n } from '../../shared/i18n';
import { Theme } from '../../shared/core/theme';
import ThemeProvider, { useTheme } from '../../shared/core/ThemeProvider';
import { ColoZip } from '../../shared/core/zip';
import { readFileAsArrayBuffer, downloadBlob } from '../../shared/core/files';
import { formatBytes } from '../../shared/core/formatting';
import BuildInfoPanel from '../../shared/components/BuildInfoPanel';
import { AppHeader, DropZone, FileList, ProgressBar, SettingsPanel, StatusMessage as StatusMessageComponent, SettingItemConfig } from '../../shared/components';
import { getFormats, activateFormat, getFormat, getDefaultFormat, normalizeFormatOptions } from '../../shared/formats';
import { saveSelectedFormatId, loadSelectedFormatId, loadFormatConfig, saveFormatConfig, resetAllStoredData } from '../../shared/core/configStore';
import { AdvancedSettingsController, setupFormatAwareAdvancedSettings } from '../../shared/core/advancedSettingsModal';
import Storage from '../../shared/core/storage';
import { BuildInfoPayload, FileEntry, FormatDefinition, FormatOptions, SettingsState, StatusMessage, SETTINGS_SCHEMA_VERSION, SETTINGS_SCHEMA_VERSION_STORAGE_KEY } from '../../shared/types';
import { clearOutputDirectoryHandle, loadOutputDirectoryHandle, saveOutputDirectoryHandle } from './outputDirectoryStore';

interface FileSystemEntry {
  isFile: boolean;
  isDirectory: boolean;
  name: string;
}

interface FileSystemFileEntry extends FileSystemEntry {
  file(successCallback: (file: File) => void, errorCallback?: (error: DOMException) => void): void;
}

interface FileSystemDirectoryReader {
  readEntries(successCallback: (entries: FileSystemEntry[]) => void, errorCallback?: (error: DOMException) => void): void;
}

interface FileSystemDirectoryEntry extends FileSystemEntry {
  createReader(): FileSystemDirectoryReader;
}

interface ChromeFileCandidate {
  file: File;
  path: string;
}

const SETTINGS_STORAGE_KEY = 'settings';

type ChromeSettingKey = 'processFolder' | 'createZip';

const defaultChromeSettings: SettingsState = {
  useOriginalOutput: false,
};

interface DirectoryPickerOptions {
  id?: string;
  mode?: 'read' | 'readwrite';
}

type DirectoryPickerFunction = (options?: DirectoryPickerOptions) => Promise<FileSystemDirectoryHandle>;

const applyChromeSettingDefaults = (settings?: SettingsState | null): SettingsState => ({
  ...defaultChromeSettings,
  ...(settings ?? {}),
});

const sanitizeRelativeOutputPath = (input: string): string => {
  if (typeof input !== 'string') {
    return '';
  }
  const rawSegments = input
    .replace(/\\/g, '/')
    .split('/')
    .map((segment) => segment.trim())
    .filter((segment) => segment.length > 0);
  if (rawSegments.length === 0) {
    return '';
  }
  const sanitized: string[] = [];
  for (const segment of rawSegments) {
    if (segment === '.' || segment === '..') {
      return '';
    }
    sanitized.push(segment);
  }
  return sanitized.join('/');
};

const OFFSCREEN_MAX_ATTEMPTS = 8;
const OFFSCREEN_BASE_DELAY_MS = 80;

async function traverseFileTree(entry: FileSystemEntry, parentPath = ''): Promise<ChromeFileCandidate[]> {
  if (entry.isFile) {
    const fileEntry = entry as FileSystemFileEntry;
    const file = await new Promise<File>((resolve, reject) => {
      fileEntry.file(resolve, (error) => {
        reject(error);
      });
    });
    const path = parentPath ? `${parentPath}/${file.name}` : file.name;
    if (file.name.toLowerCase().endsWith('.png') || file.type === 'image/png') {
      return [{ file, path }];
    }
    return [];
  }

  if (entry.isDirectory) {
    const directory = entry as FileSystemDirectoryEntry;
    const reader = directory.createReader();
    const readEntries = () =>
      new Promise<FileSystemEntry[]>((resolve, reject) => {
        reader.readEntries(resolve, reject);
      });

    const results: ChromeFileCandidate[] = [];
    let batch: FileSystemEntry[];

    do {
      batch = await readEntries();
      for (let index = 0; index < batch.length; index += 1) {
        const child = batch[index];
        const nextParent = parentPath ? `${parentPath}/${child.name}` : child.name;
        const nested = await traverseFileTree(child, nextParent);
        nested.forEach((candidate) => results.push(candidate));
      }
    } while (batch.length > 0);

    return results;
  }

  return [];
}

function sendChromeMessage<TResponse = unknown>(message: unknown): Promise<TResponse> {
  if (typeof chrome === 'undefined' || !chrome.runtime || !chrome.runtime.sendMessage) {
    return Promise.reject(new Error('Chrome runtime is unavailable'));
  }
  return new Promise((resolve, reject) => {
    try {
      chrome.runtime.sendMessage(message, (response: TResponse) => {
        if (chrome.runtime.lastError) {
          reject(new Error(chrome.runtime.lastError.message));
          return;
        }
        resolve(response);
      });
    } catch (error) {
      reject(error);
    }
  });
}

async function fetchBuildInfo(): Promise<{ buildInfo: BuildInfoPayload | null; isThreadsEnabled: boolean }> {
  try {
    const response = await sendChromeMessage<
      | {
          success: true;
          version?: number;
          libwebpVersion?: number;
          libpngVersion?: number;
          libavifVersion?: number;
          pngxOxipngVersion?: number;
          pngxLibimagequantVersion?: number;
          compilerVersionString?: string;
          rustVersionString?: string;
          buildtime?: number;
          isThreadsEnabled?: boolean;
        }
      | { success: false }
    >({
      type: 'GET_VERSION_INFO',
      target: 'offscreen',
    });
    if (response && response.success) {
      return {
        buildInfo: {
          version: response.version,
          libwebpVersion: response.libwebpVersion,
          libpngVersion: response.libpngVersion,
          libavifVersion: response.libavifVersion,
          pngxOxipngVersion: response.pngxOxipngVersion,
          pngxLibimagequantVersion: response.pngxLibimagequantVersion,
          compilerVersionString: response.compilerVersionString,
          rustVersionString: response.rustVersionString,
          buildtime: response.buildtime,
        },
        isThreadsEnabled: response.isThreadsEnabled ?? true,
      };
    }
    return { buildInfo: null, isThreadsEnabled: true };
  } catch (_error) {
    return { buildInfo: null, isThreadsEnabled: true };
  }
}

const ChromeAppInner: React.FC = () => {
  const { t, setLanguage, availableLanguages, language } = useI18n();
  const state = useAppState();
  const dispatch = useAppDispatch();
  const { setTheme } = useTheme();
  const fileInputRef = useRef<HTMLInputElement | null>(null);
  const statusTimerRef = useRef<number | null>(null);
  const advancedSettingsControllerRef = useRef<AdvancedSettingsController | null>(null);
  const latestAdvancedFormatRef = useRef<string | null>(null);
  const latestAdvancedConfigRef = useRef<FormatOptions | null>(null);
  const threadsEnabledRef = useRef<boolean>(true);
  const cancelRequestedRef = useRef<boolean>(false);
  const [isCancelling, setIsCancelling] = useState(false);
  const [isDragOver, setIsDragOver] = useState(false);
  const [formats] = useState<FormatDefinition[]>(() => {
    const order = ['webp', 'avif', 'pngx'];
    const sorted = getFormats()
      .slice()
      .sort((a, b) => order.indexOf(a.id) - order.indexOf(b.id));
    return sorted;
  });
  const [formatConfigs, setFormatConfigs] = useState<Record<string, FormatOptions>>({});
  const [initializing, setInitializing] = useState(true);

  const currentFormat = useMemo(() => getFormat(state.activeFormatId) ?? getDefaultFormat(), [state.activeFormatId]);
  const storedFormatConfig = formatConfigs[currentFormat.id];
  const currentConfig = useMemo(() => storedFormatConfig ?? currentFormat.getDefaultOptions(), [currentFormat, storedFormatConfig]);
  const normalizedOutputDirectoryPath = '';
  const isFixedOutputActive = false;

  const applyStatus = useCallback(
    (status: StatusMessage | null) => {
      if (statusTimerRef.current) {
        clearTimeout(statusTimerRef.current);
        statusTimerRef.current = null;
      }
      dispatch({ type: 'setStatus', status });
      if (status && status.durationMs && status.durationMs > 0) {
        statusTimerRef.current = window.setTimeout(() => {
          dispatch({ type: 'setStatus', status: null });
        }, status.durationMs);
      }
    },
    [dispatch]
  );

  const applyStatusRef = useRef(applyStatus);
  useEffect(() => {
    applyStatusRef.current = applyStatus;
  }, [applyStatus]);

  const resolveConversionError = useCallback(
    (error: unknown, format: FormatDefinition, originalSize?: number) => {
      const fallbackMessage = (error as Error | undefined)?.message ?? t('common.unknownError');
      const baseInputSize = typeof originalSize === 'number' && Number.isFinite(originalSize) ? originalSize : undefined;
      const base = {
        errorMessageKey: 'common.errorPrefix',
        errorParams: { message: fallbackMessage },
        errorRawMessage: fallbackMessage,
        inputSize: baseInputSize,
        outputSize: undefined as number | undefined,
      };

      if (!error || typeof error !== 'object') {
        return base;
      }

      const typed = error as Error & {
        code?: string;
        details?: { inputSize?: number; outputSize?: number };
        inputSize?: number;
        outputSize?: number;
      };

      const rawDetails = typed.details ?? {
        inputSize: typed.inputSize,
        outputSize: typed.outputSize,
      };

      const extractSize = (value: unknown): number | undefined => {
        return typeof value === 'number' && Number.isFinite(value) ? value : undefined;
      };

      const detailedInputSize = extractSize(rawDetails?.inputSize);
      const detailedOutputSize = extractSize(rawDetails?.outputSize);
      const inputSize = detailedInputSize ?? baseInputSize;
      const outputSize = detailedOutputSize;

      const describeSize = (size?: number) => {
        if (typeof size !== 'number' || Number.isNaN(size)) {
          return t('errors.sizeUnknown');
        }
        return `${formatBytes(size)} (${size} B)`;
      };

      if (typed.code === 'output_larger_than_input') {
        return {
          errorMessageKey: 'errors.outputLarger',
          errorParams: {
            format: t(format.labelKey),
            inputSize: describeSize(inputSize),
            outputSize: describeSize(outputSize),
          },
          errorRawMessage: fallbackMessage,
          inputSize,
          outputSize,
        };
      }

      return {
        ...base,
        inputSize,
        outputSize,
      };
    },
    [t]
  );

  const getErrorMessageForEntry = useCallback(
    (entry: FileEntry) => {
      if (entry.errorMessageKey) {
        return t(entry.errorMessageKey, entry.errorParams);
      }
      if (entry.errorParams?.message) {
        return String(entry.errorParams.message);
      }
      if (entry.errorMessageRaw) {
        return entry.errorMessageRaw;
      }
      return t('common.unknownError');
    },
    [t]
  );

  const handleCopyErrorMessage = useCallback(
    async (entry: FileEntry) => {
      const message = getErrorMessageForEntry(entry);
      if (!message) {
        applyStatus({ type: 'error', messageKey: 'errors.copyFailed', durationMs: 4000 });
        return;
      }
      const copyViaFallback = () => {
        if (typeof document === 'undefined') {
          throw new Error('document is unavailable');
        }
        const textarea = document.createElement('textarea');
        textarea.value = message;
        textarea.style.position = 'fixed';
        textarea.style.opacity = '0';
        textarea.style.pointerEvents = 'none';
        document.body.appendChild(textarea);
        textarea.focus();
        textarea.select();
        const succeeded = document.execCommand('copy');
        document.body.removeChild(textarea);
        if (!succeeded) {
          throw new Error('clipboard copy command failed');
        }
      };

      try {
        if (typeof navigator !== 'undefined' && navigator.clipboard && navigator.clipboard.writeText) {
          await navigator.clipboard.writeText(message);
        } else {
          copyViaFallback();
        }
        applyStatus({ type: 'success', messageKey: 'errors.copySuccess', durationMs: 2000 });
      } catch (_copyError) {
        applyStatus({ type: 'error', messageKey: 'errors.copyFailed', durationMs: 4000 });
      }
    },
    [applyStatus, getErrorMessageForEntry]
  );

  const persistFormatConfig = useCallback(
    async (format: FormatDefinition, config: FormatOptions) => {
      setFormatConfigs((prev) => ({ ...prev, [format.id]: config }));
      try {
        await saveFormatConfig(format, config);
      } catch {
        /* NOP */
      }
    },
    [setFormatConfigs]
  );

  const initializeAdvancedSettings = useCallback(
    async (format: FormatDefinition, config: FormatOptions) => {
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
        applyStatusRef.current({ type: 'error', messageKey: 'common.unknownError', durationMs: 4000 });
        return null;
      }
    },
    [persistFormatConfig]
  );

  useEffect(() => {
    if (!storedFormatConfig) {
      return;
    }
    if (latestAdvancedFormatRef.current === currentFormat.id && latestAdvancedConfigRef.current === storedFormatConfig) {
      return;
    }
    void initializeAdvancedSettings(currentFormat, storedFormatConfig);
  }, [currentFormat, initializeAdvancedSettings, storedFormatConfig]);

  useEffect(() => {
    return () => {
      if (statusTimerRef.current) {
        clearTimeout(statusTimerRef.current);
      }
    };
  }, []);

  useEffect(() => {
    return () => {
      if (advancedSettingsControllerRef.current) {
        advancedSettingsControllerRef.current.dispose();
        advancedSettingsControllerRef.current = null;
      }
    };
  }, []);

  useEffect(() => {
    let cancelled = false;

    const run = async () => {
      dispatch({ type: 'setBuildInfo', buildInfo: { status: 'loading' } });

      try {
        const storedSchemaVersion = await Storage.getItem<number>(SETTINGS_SCHEMA_VERSION_STORAGE_KEY);
        const shouldResetAll = typeof storedSchemaVersion === 'number' && storedSchemaVersion !== SETTINGS_SCHEMA_VERSION;
        if (shouldResetAll) {
          await resetAllStoredData();
          await Storage.setItem(SETTINGS_SCHEMA_VERSION_STORAGE_KEY, SETTINGS_SCHEMA_VERSION);
          window.alert(t('settingsMenu.schemaResetNotice'));
        } else if (storedSchemaVersion == null) {
          await Storage.setItem(SETTINGS_SCHEMA_VERSION_STORAGE_KEY, SETTINGS_SCHEMA_VERSION);
        }

        const formats = getFormats();
        const fallbackFormat = formats[0] ?? getDefaultFormat();
        const savedFormatId = await loadSelectedFormatId();
        const initialFormat = getFormat(savedFormatId ?? '') ?? fallbackFormat;

        activateFormat(initialFormat.id);
        dispatch({ type: 'setActiveFormat', id: initialFormat.id });

        const config = await loadFormatConfig(initialFormat);
        if (!cancelled) {
          setFormatConfigs((prev) => ({ ...prev, [initialFormat.id]: config }));
        }

        const storedSettings = await Storage.getJSON<SettingsState>(SETTINGS_STORAGE_KEY, {});
        if (!cancelled) {
          dispatch({ type: 'setSettings', settings: applyChromeSettingDefaults(storedSettings) });
        }

        const { buildInfo, isThreadsEnabled } = await fetchBuildInfo();
        if (!cancelled) {
          threadsEnabledRef.current = isThreadsEnabled;
          dispatch({ type: 'setBuildInfo', buildInfo: buildInfo ? { status: 'ready', payload: buildInfo } : { status: 'error' } });
          applyStatusRef.current({ type: 'info', messageKey: 'common.dropPrompt', durationMs: 4000 });
          setInitializing(false);
        }
      } catch (_error) {
        applyStatusRef.current({ type: 'error', messageKey: 'common.unknownError', durationMs: 4000 });
        if (!cancelled) {
          dispatch({ type: 'setBuildInfo', buildInfo: { status: 'error' } });
          setInitializing(false);
        }
      }
    };

    run();

    return () => {
      cancelled = true;
    };
  }, [dispatch]);

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape' && state.isProcessing) {
        cancelRequestedRef.current = true;
        setIsCancelling(true);

        sendChromeMessage({ type: 'CANCEL_PROCESSING', target: 'background' }).catch(() => {
          /* NOP */
        });
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => {
      window.removeEventListener('keydown', handleKeyDown);
    };
  }, [state.isProcessing]);

  const handleThemeToggle = useCallback(async () => {
    const nextTheme: Theme = state.theme === 'dark' ? 'light' : 'dark';
    await setTheme(nextTheme);
  }, [setTheme, state.theme]);

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

  const persistSettings = useCallback(
    async (patch: Partial<SettingsState>) => {
      const merged: SettingsState = { ...state.settings, ...patch };
      dispatch({ type: 'mergeSettings', settings: patch });
      await Storage.setItem(SETTINGS_STORAGE_KEY, merged);
    },
    [dispatch, state.settings]
  );

  const handleSettingToggle = useCallback(
    async (key: ChromeSettingKey, value: boolean) => {
      await persistSettings({ [key]: value } as Partial<SettingsState>);
    },
    [persistSettings]
  );

  const ensureHandlePermission = useCallback(async (_handle: FileSystemDirectoryHandle | null) => {
    return false;
  }, []);

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

  const handleResetAllSettings = useCallback(async () => {
    if (!window.confirm(t('settingsMenu.resetConfirm'))) {
      return;
    }
    try {
      await resetAllStoredData();
      window.alert(t('settingsMenu.resetSuccess'));
      window.location.reload();
    } catch (error) {
      window.alert(t('settingsMenu.resetFailed', { error: (error as Error).message ?? '' }));
    }
  }, [dispatch, t]);

  const resetProcessingState = useCallback(() => {
    cancelRequestedRef.current = false;
    setIsCancelling(false);
    dispatch({ type: 'setProcessing', processing: true });
    dispatch({ type: 'setProgress', progress: { current: 0, total: 0, state: 'processing' } });
    dispatch({ type: 'setFileEntries', entries: [] });
  }, [dispatch]);

  const finalizeProcessingState = useCallback(() => {
    cancelRequestedRef.current = false;
    setIsCancelling(false);
    dispatch({ type: 'setProcessing', processing: false });
  }, [dispatch]);

  const handleCancelProcessing = useCallback(() => {
    cancelRequestedRef.current = true;
    setIsCancelling(true);

    sendChromeMessage({ type: 'CANCEL_PROCESSING', target: 'background' }).catch(() => {
      /* NOP */
    });
    offscreenReadyPromiseRef.current = null;
  }, []);

  const updateProgress = useCallback(
    (current: number, total: number, done = false) => {
      dispatch({
        type: 'setProgress',
        progress: { current, total, state: done ? 'done' : 'processing' },
      });
    },
    [dispatch]
  );

  const registerFileEntries = useCallback(
    (files: ChromeFileCandidate[]) => {
      const entries: FileEntry[] = files.map((candidate, index) => ({
        id: `${Date.now()}_${index}`,
        name: candidate.path || candidate.file.name,
        status: 'pending',
      }));
      dispatch({ type: 'setFileEntries', entries });
      return entries;
    },
    [dispatch]
  );

  const patchFileEntry = useCallback(
    (id: string, patch: Partial<FileEntry>) => {
      dispatch({ type: 'patchFileEntry', id, patch });
    },
    [dispatch]
  );

  const offscreenReadyRef = useRef<boolean>(false);
  const offscreenReadyPromiseRef = useRef<Promise<void> | null>(null);

  const ensureOffscreenReady = useCallback(async () => {
    if (offscreenReadyRef.current) {
      return;
    }

    if (!offscreenReadyPromiseRef.current) {
      const attemptEnsure = async () => {
        await sendChromeMessage({ type: 'ENSURE_OFFSCREEN', target: 'background' });

        for (let attempt = 0; attempt < OFFSCREEN_MAX_ATTEMPTS; attempt += 1) {
          try {
            const response = await sendChromeMessage<{ success?: boolean }>({ type: 'OFFSCREEN_PING', target: 'offscreen' });
            if (!response || response.success !== false) {
              offscreenReadyRef.current = true;
              return;
            }
          } catch (error) {
            if (attempt === OFFSCREEN_MAX_ATTEMPTS - 1) {
              throw error;
            }
          }

          await new Promise<void>((resolve) => {
            window.setTimeout(resolve, OFFSCREEN_BASE_DELAY_MS * (attempt + 1));
          });
        }

        throw new Error('Offscreen worker did not respond to ping');
      };

      offscreenReadyPromiseRef.current = attemptEnsure()
        .catch((error) => {
          offscreenReadyRef.current = false;
          throw error;
        })
        .finally(() => {
          offscreenReadyPromiseRef.current = null;
        });
    }

    await offscreenReadyPromiseRef.current;
  }, []);

  const convertFileWithOffscreen = useCallback(
    async (candidate: ChromeFileCandidate, options: FormatOptions) => {
      const arrayBuffer = await readFileAsArrayBuffer(candidate.file);
      const pngData = new Uint8Array(arrayBuffer);
      const serializedInput = Array.from(pngData);
      const format = getFormat(state.activeFormatId) ?? getDefaultFormat();
      const response = await sendChromeMessage<{
        success: boolean;
        outputData?: unknown;
        outputEncoding?: string;
        error?: string;
        errorCode?: string;
        errorDetails?: { inputSize?: number; outputSize?: number; formatLabel?: string };
        formatId?: string;
        conversionTimeMs?: number;
      }>({
        type: 'PROCESS_FILE',
        target: 'offscreen',
        config: options,
        formatId: format.id,
        fileData: {
          name: candidate.file.name,
          data: serializedInput,
          dataEncoding: 'uint8-array',
          outputName: (candidate.path || candidate.file.name).replace(/\.png$/i, `.${format.outputExtension}`),
        },
      });
      if (!response || !response.success || response.outputData === undefined || response.outputData === null) {
        const errorMessage = response?.error || t('common.conversionFailedUnknown');
        const failure = new Error(errorMessage);
        (failure as Error & { code?: string; details?: unknown }).code = response?.errorCode;
        (failure as Error & { code?: string; details?: unknown }).details = response?.errorDetails;
        if (response?.errorDetails) {
          (failure as Error & { inputSize?: number; outputSize?: number }).inputSize = response.errorDetails.inputSize;
          (failure as Error & { inputSize?: number; outputSize?: number }).outputSize = response.errorDetails.outputSize;
        }
        throw failure;
      }

      let outputData: Uint8Array;
      if (response.outputEncoding === 'uint8-array' && Array.isArray(response.outputData)) {
        outputData = Uint8Array.from(response.outputData as unknown as number[]);
      } else if (response.outputData instanceof ArrayBuffer) {
        outputData = new Uint8Array(response.outputData);
      } else if (Array.isArray(response.outputData)) {
        outputData = Uint8Array.from(response.outputData as unknown as number[]);
      } else {
        throw new Error('Received unsupported output data from offscreen worker');
      }
      return { outputData, pngData, formatId: response.formatId ?? format.id, conversionTimeMs: response.conversionTimeMs } as const;
    },
    [state.activeFormatId, t]
  );

  const processFiles = useCallback(
    async (files: ChromeFileCandidate[]) => {
      if (files.length === 0) {
        applyStatus({ type: 'error', messageKey: 'common.noPngFiles', durationMs: 4000 });
        return;
      }
      if (state.isProcessing) {
        applyStatus({ type: 'info', messageKey: 'common.processingWait', durationMs: 4000 });
        return;
      }

      await ensureOffscreenReady();
      resetProcessingState();
      const entries = registerFileEntries(files);
      updateProgress(0, files.length);

      const shouldCreateZip = Boolean(state.settings.createZip) && files.length > 1;
      applyStatus({ type: 'info', messageKey: shouldCreateZip ? 'common.createZipInProgress' : 'common.conversionRunning', durationMs: 0 });

      const zipCandidates: Array<{ name: string; data: Uint8Array; originalSize: number; convertedSize: number }> = [];
      let successCount = 0;
      const format = getFormat(state.activeFormatId) ?? getDefaultFormat();
      const options = normalizeFormatOptions(format, formatConfigs[format.id]);

      for (let index = 0; index < files.length; index += 1) {
        if (cancelRequestedRef.current) {
          dispatch({ type: 'setFileEntries', entries: [] });
          dispatch({ type: 'setProgress', progress: { current: 0, total: 0, state: 'idle' } });
          applyStatus({ type: 'info', messageKey: 'common.conversionCanceled', durationMs: 4000 });
          finalizeProcessingState();
          return;
        }

        const candidate = files[index];
        const entry = entries[index];
        patchFileEntry(entry.id, { status: 'processing' });

        let originalSize = 0;

        try {
          const { outputData, pngData, conversionTimeMs } = await convertFileWithOffscreen(candidate, options);
          originalSize = pngData.length;
          const sourceName = candidate.path || candidate.file.name;
          const baseName = sourceName.split(/[\\/]/).pop() ?? sourceName;
          const sanitizedRelative = sanitizeRelativeOutputPath(sourceName);
          const outputName = baseName.replace(/\.png$/i, `.${format.outputExtension}`);
          const targetRelativePath = (sanitizedRelative && sanitizedRelative.replace(/\.png$/i, `.${format.outputExtension}`)) || outputName;

          if (shouldCreateZip) {
            zipCandidates.push({
              name: outputName,
              data: outputData,
              originalSize: pngData.length,
              convertedSize: outputData.length,
            });
            successCount += 1;
          } else {
            downloadBlob(outputData, outputName);
            successCount += 1;
          }

          patchFileEntry(entry.id, {
            status: 'success',
            originalSize: pngData.length,
            convertedSize: outputData.length,
            conversionTimeMs,
          });
        } catch (error) {
          const failure = error as Error & { code?: string };
          const errorInfo = resolveConversionError(failure, format, originalSize);
          const isOutputLarger = failure?.code === 'output_larger_than_input';
          const entryOriginalSize = typeof errorInfo.inputSize === 'number' ? errorInfo.inputSize : typeof originalSize === 'number' && Number.isFinite(originalSize) ? originalSize : undefined;
          const entryConvertedSize = typeof errorInfo.outputSize === 'number' ? errorInfo.outputSize : undefined;
          patchFileEntry(entry.id, {
            status: 'error',
            errorMessageKey: errorInfo.errorMessageKey,
            errorParams: errorInfo.errorParams,
            errorMessageRaw: errorInfo.errorRawMessage,
            errorCode: failure.code,
            originalSize: entryOriginalSize,
            convertedSize: entryConvertedSize,
          });
          applyStatus({ type: 'error', messageKey: errorInfo.errorMessageKey, params: errorInfo.errorParams, durationMs: 5000 });
        }

        updateProgress(index + 1, files.length, index + 1 === files.length);
      }

      if (shouldCreateZip && zipCandidates.length > 0) {
        const zip = new ColoZip();
        zipCandidates.forEach((file) => {
          zip.addFile(file.name, file.data);
        });
        const zipData = zip.generate();
        const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
        downloadBlob(zipData, `colopresso-${timestamp}.zip`);
        applyStatus({ type: 'success', messageKey: 'common.zipCreated', durationMs: 4000 });
      } else if (!shouldCreateZip && successCount > 0) {
        const messageKey = successCount === 1 ? 'common.filesConvertedOne' : 'common.filesConvertedMany';
        applyStatus({ type: 'success', messageKey, params: successCount === 1 ? {} : { count: successCount }, durationMs: 4000 });
      }

      finalizeProcessingState();
    },
    [
      applyStatus,
      convertFileWithOffscreen,
      ensureOffscreenReady,
      finalizeProcessingState,
      formatConfigs,
      patchFileEntry,
      registerFileEntries,
      resetProcessingState,
      resolveConversionError,
      state.isProcessing,
      state.settings.createZip,
      state.activeFormatId,
      updateProgress,
    ]
  );

  const hasDirectoryEntry = useCallback((items: DataTransferItemList) => {
    for (let index = 0; index < items.length; index += 1) {
      const entry = items[index];
      const fsEntry = entry.webkitGetAsEntry?.();
      if (fsEntry?.isDirectory) {
        return true;
      }
    }
    return false;
  }, []);

  const collectFilesFromDataTransfer = useCallback(
    async (dataTransfer: DataTransfer): Promise<ChromeFileCandidate[]> => {
      const items = dataTransfer.items;
      const files: ChromeFileCandidate[] = [];
      const directories = hasDirectoryEntry(items);

      if (directories && !state.settings.processFolder) {
        applyStatus({ type: 'info', messageKey: 'common.enableFolderProcessing', durationMs: 5000 });
        return [];
      }

      if (directories) {
        const tasks: Array<Promise<ChromeFileCandidate[]>> = [];
        for (let index = 0; index < items.length; index += 1) {
          const entry = items[index];
          const fsEntry = entry.webkitGetAsEntry?.();
          if (fsEntry) {
            tasks.push(traverseFileTree(fsEntry));
          }
        }
        const nested = await Promise.all(tasks);
        nested.flat().forEach((candidate) => files.push(candidate));
        return files;
      }

      for (let index = 0; index < dataTransfer.files.length; index += 1) {
        const file = dataTransfer.files[index];
        if (file.name.toLowerCase().endsWith('.png') || file.type === 'image/png') {
          files.push({ file, path: file.name });
        }
      }

      return files;
    },
    [applyStatus, hasDirectoryEntry, state.settings.processFolder, traverseFileTree]
  );

  const handleDrop = useCallback(
    async (event: React.DragEvent<HTMLDivElement>) => {
      event.preventDefault();
      event.stopPropagation();
      setIsDragOver(false);
      const files = await collectFilesFromDataTransfer(event.dataTransfer);
      await processFiles(files);
    },
    [collectFilesFromDataTransfer, processFiles]
  );

  const handleBrowseClick = useCallback(() => {
    if (state.isProcessing) {
      applyStatus({ type: 'info', messageKey: 'common.processingWait', durationMs: 3000 });
      return;
    }
    fileInputRef.current?.click();
  }, [applyStatus, state.isProcessing]);

  const handleFileInputChange = useCallback(
    async (event: React.ChangeEvent<HTMLInputElement>) => {
      const fileList = event.target.files;
      if (!fileList) {
        return;
      }
      const files: ChromeFileCandidate[] = [];
      for (let index = 0; index < fileList.length; index += 1) {
        const file = fileList.item(index);
        if (!file) {
          continue;
        }
        if (file.name.toLowerCase().endsWith('.png') || file.type === 'image/png') {
          files.push({ file, path: file.name });
        }
      }
      await processFiles(files);
      event.target.value = '';
    },
    [processFiles]
  );

  const handleLanguageChange = useCallback(
    async (event: React.ChangeEvent<HTMLSelectElement>) => {
      const next = event.target.value;
      await setLanguage(next as typeof language);
    },
    [setLanguage]
  );

  const formatStatusText = useCallback(
    (entry: FileEntry) => {
      switch (entry.status) {
        case 'pending':
          return t('utils.status.pending');
        case 'processing':
          return t('utils.status.processing');
        case 'success':
          return t('utils.status.done');
        case 'error':
          return t('utils.status.error');
        default:
          return entry.status;
      }
    },
    [t]
  );

  const statusMessage = useMemo(() => {
    if (!state.status) return null;
    return t(state.status.messageKey, state.status.params);
  }, [state.status, t]);

  const progressText = useMemo(() => {
    if (state.progress.total === 0) return '';
    return state.progress.state === 'done' ? t('utils.progressDone') : t('utils.progressProcessing', { current: state.progress.current, total: state.progress.total });
  }, [state.progress.current, state.progress.total, state.progress.state, t]);

  const settingsItems: SettingItemConfig[] = useMemo(
    () => [
      {
        id: 'processFolderCheckbox',
        type: 'checkbox',
        labelKey: 'chrome.settings.processFolderLabel',
        descriptionKey: 'chrome.settings.processFolderDescription',
        value: Boolean(state.settings.processFolder),
        onChange: (value) => handleSettingToggle('processFolder', value as boolean),
      },
      {
        id: 'createZipCheckbox',
        type: 'checkbox',
        labelKey: 'chrome.settings.createZipLabel',
        descriptionKey: 'chrome.settings.createZipDescription',
        value: Boolean(state.settings.createZip) && !isFixedOutputActive,
        disabled: isFixedOutputActive,
        locked: isFixedOutputActive,
        lockedNoteKey: 'common.outputDirectory.lockedOptionNote',
        onChange: (value) => handleSettingToggle('createZip', value as boolean),
      },
    ],
    [state.settings.processFolder, state.settings.createZip, isFixedOutputActive, handleSettingToggle]
  );

  const renderResetButton = () => {
    return (
      <div className={styles.resetSection}>
        <button type="button" id="resetAllSettingsBtn" className={styles.dangerResetButton} onClick={handleResetAllSettings}>
          {t('settingsMenu.resetAll')}
        </button>
      </div>
    );
  };

  const renderBuildInfo = () => {
    const manifest = typeof chrome !== 'undefined' && chrome.runtime && typeof chrome.runtime.getManifest === 'function' ? chrome.runtime.getManifest() : { version: '0.0.0' };
    const headerTitle = `${t('chrome.title')} for Chrome v${manifest.version}`;
    return <BuildInfoPanel title={headerTitle} buildInfo={state.buildInfo} t={t} />;
  };

  if (initializing) {
    return <div className={styles.container} />;
  }

  const activeFormatId = currentFormat.id;

  return (
    <div className={styles.container}>
      <AppHeader
        title={t('chrome.title')}
        formats={formats}
        activeFormatId={activeFormatId}
        language={language}
        availableLanguages={availableLanguages}
        theme={state.theme}
        isProcessing={state.isProcessing}
        onFormatChange={handleFormatChange}
        onLanguageChange={handleLanguageChange}
        onThemeToggle={handleThemeToggle}
        onAdvancedSettingsClick={handleAdvancedSettingsClick}
        t={t}
      />

      <DropZone
        iconEmoji="🖼️"
        text={t('chrome.dropzone.text')}
        hint={t('chrome.dropzone.hint')}
        isProcessing={state.isProcessing}
        isDragOver={isDragOver}
        onDragOver={(event) => {
          event.preventDefault();
          event.stopPropagation();
          if (!state.isProcessing) {
            setIsDragOver(true);
          }
        }}
        onDragEnter={(event) => {
          event.preventDefault();
          event.stopPropagation();
          if (!state.isProcessing) {
            setIsDragOver(true);
          }
        }}
        onDragLeave={(event) => {
          event.preventDefault();
          event.stopPropagation();
          setIsDragOver(false);
        }}
        onDragEnd={() => setIsDragOver(false)}
        onDrop={handleDrop}
        onClick={handleBrowseClick}
        onCancel={handleCancelProcessing}
        cancelButtonLabel={t('common.cancelButton')}
        isCancelling={isCancelling}
        cancellingButtonLabel={t('common.cancellingButton')}
      />

      <SettingsPanel items={settingsItems} isProcessing={state.isProcessing} t={t} />

      <input ref={fileInputRef} type="file" accept="image/png,.png" multiple hidden onChange={handleFileInputChange} />

      <ProgressBar current={state.progress.current} total={state.progress.total} isVisible={state.progress.total > 0} progressText={progressText} />
      <FileList entries={state.fileEntries} formatStatusText={formatStatusText} getErrorMessage={getErrorMessageForEntry} onCopyError={handleCopyErrorMessage} formatBytes={formatBytes} t={t} />
      <StatusMessageComponent message={statusMessage} type={state.status?.type ?? 'info'} />
      {renderBuildInfo()}
      {renderResetButton()}
    </div>
  );
};

const ChromeApp: React.FC = () => {
  return (
    <I18nProvider>
      <AppStateProvider>
        <ThemeProvider>
          <ChromeAppInner />
        </ThemeProvider>
      </AppStateProvider>
    </I18nProvider>
  );
};

export default ChromeApp;
