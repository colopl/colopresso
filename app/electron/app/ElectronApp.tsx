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

import React, { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import styles from '../../shared/styles/App.module.css';
import { AppStateProvider, useAppDispatch, useAppState } from '../../shared/state/appState';
import { I18nProvider, useI18n } from '../../shared/i18n';
import { Theme } from '../../shared/core/theme';
import ThemeProvider, { useTheme } from '../../shared/core/ThemeProvider';
import { ColoZip } from '../../shared/core/zip';
import { formatBytes } from '../../shared/core/formatting';
import { getFormats, activateFormat, getFormat, getDefaultFormat, normalizeFormatOptions } from '../../shared/formats';
import { loadFormatConfig, saveFormatConfig, saveSelectedFormatId, loadSelectedFormatId, resetAllStoredData } from '../../shared/core/configStore';
import { AdvancedSettingsController, setupFormatAwareAdvancedSettings } from '../../shared/core/advancedSettingsModal';
import Storage from '../../shared/core/storage';
import { initializeModule, getVersionInfo, ModuleWithHelpers } from '../../shared/core/converter';
import { readFileAsArrayBuffer } from '../../shared/core/files';
import { FileEntry, FormatDefinition, FormatOptions, SettingsState, StatusMessage, BuildInfoPayload, SETTINGS_SCHEMA_VERSION, SETTINGS_SCHEMA_VERSION_STORAGE_KEY } from '../../shared/types';
import BuildInfoPanel from '../../shared/components/BuildInfoPanel';

const ELECTRON_SETTINGS_STORAGE_KEY = 'settings';
const COLOPRESSO_MODULE_URL = new URL(/* @vite-ignore */ './colopresso.js', import.meta.url);

type ColopressoModuleFactoryType = (moduleConfig?: Record<string, unknown>) => Promise<ModuleWithHelpers>;

let moduleFactoryPromise: Promise<ColopressoModuleFactoryType> | null = null;

const loadColopressoModuleFactory = async (): Promise<ColopressoModuleFactoryType> => {
  if (!moduleFactoryPromise) {
    moduleFactoryPromise = import(/* @vite-ignore */ COLOPRESSO_MODULE_URL.href).then((mod) => mod.default as ColopressoModuleFactoryType);
  }
  return moduleFactoryPromise;
};

const defaultElectronSettings: SettingsState = {
  useOriginalOutput: false,
};

const applyElectronSettingDefaults = (settings?: SettingsState | null): SettingsState => ({
  ...defaultElectronSettings,
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

type ElectronSettingKey = 'deletePng' | 'createZip';

interface FolderFileEntry {
  name: string;
  path: string;
  data: Uint8Array;
}

interface ConversionOutcome {
  filename: string;
  data: Uint8Array;
  originalSize: number;
  convertedSize: number;
}

const ElectronAppInner: React.FC = () => {
  const { t, setLanguage, availableLanguages, language } = useI18n();
  const state = useAppState();
  const dispatch = useAppDispatch();
  const { setTheme } = useTheme();
  const [formats] = useState<FormatDefinition[]>(() => {
    const order = ['webp', 'avif', 'pngx'];
    return getFormats()
      .slice()
      .sort((a, b) => order.indexOf(a.id) - order.indexOf(b.id));
  });
  const [formatConfigs, setFormatConfigs] = useState<Record<string, FormatOptions>>({});
  const [initializing, setInitializing] = useState(true);
  const [isSelectingOutputDirectory, setIsSelectingOutputDirectory] = useState(false);
  const [isDragOver, setIsDragOver] = useState(false);
  const moduleRef = useRef<ModuleWithHelpers | null>(null);
  const modulePromiseRef = useRef<Promise<ModuleWithHelpers> | null>(null);
  const statusTimerRef = useRef<number | null>(null);
  const advancedSettingsControllerRef = useRef<AdvancedSettingsController | null>(null);
  const latestAdvancedFormatRef = useRef<string | null>(null);
  const latestAdvancedConfigRef = useRef<FormatOptions | null>(null);

  const currentFormat = useMemo(() => getFormat(state.activeFormatId) ?? getDefaultFormat(), [state.activeFormatId]);
  const storedFormatConfig = formatConfigs[currentFormat.id];
  const currentConfig = useMemo(() => storedFormatConfig ?? currentFormat.getDefaultOptions(), [currentFormat, storedFormatConfig]);
  const normalizedOutputDirectoryPath = typeof state.settings.outputDirectoryPath === 'string' ? state.settings.outputDirectoryPath.trim() : '';
  const useOriginalOutput = state.settings.useOriginalOutput === true;
  const isFixedOutputActive = useOriginalOutput && normalizedOutputDirectoryPath.length > 0;

  const ensureModule = useCallback(async (): Promise<ModuleWithHelpers> => {
    if (moduleRef.current) {
      return moduleRef.current;
    }
    if (modulePromiseRef.current) {
      return modulePromiseRef.current;
    }
    const locateFile = (resource: string) => {
      try {
        return new URL(resource, COLOPRESSO_MODULE_URL).href;
      } catch (_error) {
        return resource;
      }
    };
    const creation = loadColopressoModuleFactory()
      .then((factory) => initializeModule(factory, { locateFile }))
      .then((Module) => {
        moduleRef.current = Module;
        return Module;
      })
      .finally(() => {
        modulePromiseRef.current = null;
      });
    modulePromiseRef.current = creation;
    return creation;
  }, []);

  const loadBuildInfoFromModule = useCallback(async (): Promise<BuildInfoPayload | null> => {
    try {
      const Module = await ensureModule();
      return getVersionInfo(Module);
    } catch (_error) {
      return null;
    }
  }, [ensureModule]);

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
        inputSize?: number;
        outputSize?: number;
        details?: { inputSize?: number; outputSize?: number };
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
      } catch (_) {
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
    return () => {
      moduleRef.current = null;
      modulePromiseRef.current = null;
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

        const storedSettings = await Storage.getJSON<SettingsState>(ELECTRON_SETTINGS_STORAGE_KEY, {});
        if (!cancelled) {
          dispatch({ type: 'setSettings', settings: applyElectronSettingDefaults(storedSettings) });
        }

        const buildInfo = await loadBuildInfoFromModule();
        if (!cancelled) {
          dispatch({ type: 'setBuildInfo', buildInfo: buildInfo ? { status: 'ready', payload: buildInfo } : { status: 'error' } });
          applyStatusRef.current({ type: 'info', messageKey: 'common.dropPrompt', durationMs: 4000 });
          setInitializing(false);
        }
      } catch (_error) {
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
  }, [dispatch, loadBuildInfoFromModule]);

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
      await Storage.setItem(ELECTRON_SETTINGS_STORAGE_KEY, merged);
    },
    [dispatch, state.settings]
  );

  const handleSettingToggle = useCallback(
    async (key: ElectronSettingKey, value: boolean) => {
      await persistSettings({ [key]: value } as Partial<SettingsState>);
    },
    [persistSettings]
  );

  const handleAdvancedSettingsClick = useCallback(async () => {
    const configForModal = storedFormatConfig ?? currentFormat.getDefaultOptions();
    const controller = await initializeAdvancedSettings(currentFormat, configForModal);
    if (controller && typeof controller.open === 'function') {
      await controller.open();
    }
  }, [currentFormat, initializeAdvancedSettings, storedFormatConfig]);

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

  const registerFileEntries = useCallback(
    (names: string[]) => {
      const entries: FileEntry[] = names.map((name, index) => ({
        id: `${Date.now()}_${index}`,
        name,
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

  const resetProcessingState = useCallback(
    (total: number) => {
      dispatch({ type: 'setProcessing', processing: true });
      dispatch({ type: 'setProgress', progress: { current: 0, total, state: total === 0 ? 'idle' : 'processing' } });
    },
    [dispatch]
  );

  const updateProgress = useCallback(
    (current: number, total: number, done = false) => {
      dispatch({ type: 'setProgress', progress: { current, total, state: done ? 'done' : 'processing' } });
    },
    [dispatch]
  );

  const finalizeProcessingState = useCallback(() => {
    dispatch({ type: 'setProcessing', processing: false });
  }, [dispatch]);

  const ensureElectronAPI = (): NonNullable<Window['electronAPI']> => {
    if (!window.electronAPI) {
      applyStatus({ type: 'error', messageKey: 'common.processingUnknownError', durationMs: 4000 });
      throw new Error('electronAPI is unavailable');
    }
    return window.electronAPI;
  };

  const selectOutputDirectory = useCallback(async (): Promise<boolean> => {
    setIsSelectingOutputDirectory(true);
    try {
      const electron = ensureElectronAPI();
      const result = await electron.selectFolder();
      if (!result.success || !result.folderPath) {
        if (result?.canceled) {
          return false;
        }
        throw new Error(result.error || 'Failed to select folder');
      }
      await persistSettings({
        useOriginalOutput: true,
        outputDirectoryPath: result.folderPath,
        createZip: false,
        deletePng: false,
      });
      applyStatus({ type: 'success', messageKey: 'common.outputDirectoryReady', durationMs: 4000 });
      return true;
    } catch (error) {
      applyStatus({ type: 'error', messageKey: 'common.outputDirectorySelectFailed', durationMs: 5000, params: { message: (error as Error).message ?? t('common.unknownError') } });
      return false;
    } finally {
      setIsSelectingOutputDirectory(false);
    }
  }, [applyStatus, ensureElectronAPI, persistSettings, t]);

  const handleOutputModeToggle = useCallback(
    async (nextValue: boolean) => {
      if (nextValue) {
        await selectOutputDirectory();
        return;
      }
      await persistSettings({ useOriginalOutput: false, outputDirectoryPath: null });
      applyStatus({ type: 'info', messageKey: 'common.outputDirectoryCleared', durationMs: 3000 });
    },
    [applyStatus, persistSettings, selectOutputDirectory]
  );

  const handleOutputDirectoryReselect = useCallback(async () => {
    await selectOutputDirectory();
  }, [selectOutputDirectory]);

  const writeToFixedDirectory = useCallback(
    async (relativePath: string, data: Uint8Array) => {
      if (!isFixedOutputActive || !normalizedOutputDirectoryPath) {
        throw new Error('Output directory is not configured');
      }
      const safeRelative = sanitizeRelativeOutputPath(relativePath) || sanitizeRelativeOutputPath(relativePath.split(/[\\/]/).pop() ?? relativePath);
      if (!safeRelative) {
        throw new Error('Invalid output filename');
      }
      const electron = ensureElectronAPI();
      if (!electron.writeFileInDirectory) {
        throw new Error('writeFileInDirectory is unavailable');
      }
      const writeResult = await electron.writeFileInDirectory(normalizedOutputDirectoryPath, safeRelative, data);
      if (!writeResult.success) {
        applyStatus({ type: 'error', messageKey: 'common.outputDirectoryWriteFailed', durationMs: 5000, params: { path: normalizedOutputDirectoryPath } });
        throw new Error(writeResult.error || 'Failed to write output file');
      }
    },
    [applyStatus, ensureElectronAPI, isFixedOutputActive, normalizedOutputDirectoryPath]
  );

  const convertPngBytes = useCallback(
    async (pngData: Uint8Array): Promise<Uint8Array> => {
      const Module = await ensureModule();
      const format = getFormat(state.activeFormatId) ?? getDefaultFormat();
      const options = normalizeFormatOptions(format, formatConfigs[format.id]);
      return format.convert({ Module, inputBytes: pngData, options });
    },
    [ensureModule, formatConfigs, state.activeFormatId]
  );

  const downloadZipFile = useCallback(
    async (files: ConversionOutcome[]) => {
      const electron = ensureElectronAPI();
      const zipDialog = await electron.saveZipDialog?.(`colopresso-${new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19)}.zip`);
      if (!zipDialog || !zipDialog.success || !zipDialog.filePath) {
        if (zipDialog?.canceled) {
          applyStatus({ type: 'info', messageKey: 'common.saveDialogCanceled', durationMs: 3000 });
        }
        return;
      }

      const zip = new ColoZip();
      files.forEach((file) => {
        zip.addFile(file.filename, file.data);
      });
      const zipData = zip.generate();
      const writeResult = await electron.writeFile(zipDialog.filePath, zipData);
      if (!writeResult.success) {
        applyStatus({ type: 'error', messageKey: 'common.zipWriteFailed', durationMs: 5000, params: { path: zipDialog.filePath } });
        throw new Error(writeResult.error || 'Failed to write ZIP');
      }
      applyStatus({ type: 'success', messageKey: 'common.zipCreated', durationMs: 4000 });
    },
    [applyStatus]
  );

  const saveSingleFile = useCallback(
    async (defaultFileName: string, data: Uint8Array, relativePath?: string) => {
      if (isFixedOutputActive) {
        const fallback = relativePath ?? defaultFileName;
        await writeToFixedDirectory(fallback, data);
        return { success: true as const };
      }

      const electron = ensureElectronAPI();
      const dialogResult = await electron.saveFileDialog(defaultFileName);
      if (!dialogResult || !dialogResult.success || !dialogResult.filePath) {
        if (dialogResult?.canceled) {
          applyStatus({ type: 'info', messageKey: 'common.saveDialogCanceled', durationMs: 3000 });
        }
        return { success: false as const };
      }
      const writeResult = await electron.writeFile(dialogResult.filePath, data);
      if (!writeResult.success) {
        applyStatus({ type: 'error', messageKey: 'common.fileWriteFailed', durationMs: 5000, params: { path: dialogResult.filePath } });
        throw new Error(writeResult.error || 'Failed to write file');
      }
      return { success: true as const };
    },
    [applyStatus, ensureElectronAPI, isFixedOutputActive, writeToFixedDirectory]
  );

  const processDroppedFiles = useCallback(
    async (fileList: FileList) => {
      if (fileList.length === 0) {
        applyStatus({ type: 'error', messageKey: 'common.noPngFiles', durationMs: 4000 });
        return;
      }
      if (state.isProcessing) {
        applyStatus({ type: 'info', messageKey: 'common.processingWait', durationMs: 4000 });
        return;
      }

      const electron = ensureElectronAPI();
      const pngFiles: File[] = [];
      let directoryPath: string | null = null;
      let pathLookupFailed = false;

      for (let index = 0; index < fileList.length; index += 1) {
        const file = fileList.item(index);
        if (!file) {
          continue;
        }
        let filePath: string | undefined;
        try {
          filePath = electron.getPathForFile?.(file);
        } catch (_error) {
          pathLookupFailed = true;
        }
        if (!filePath) {
          const candidate = file as File & { path?: string };
          if (typeof candidate.path === 'string' && candidate.path.length > 0) {
            filePath = candidate.path;
          }
        }
        if (!filePath && (!file.type || file.type.length === 0)) {
          pathLookupFailed = true;
        }
        if (filePath) {
          const typeInfo = await electron.checkPathType(filePath);
          if (typeInfo.success && typeInfo.isDirectory) {
            directoryPath = filePath;
            break;
          }
        }
        if (file.type === 'image/png' || file.name.toLowerCase().endsWith('.png')) {
          pngFiles.push(file);
        }
      }

      if (directoryPath) {
        await processFolder(directoryPath);
        return;
      }

      if (pngFiles.length === 0) {
        if (pathLookupFailed) {
          applyStatus({ type: 'error', messageKey: 'common.pathRetrievalFailed', durationMs: 4000 });
          return;
        }
        applyStatus({ type: 'error', messageKey: 'common.noPngFiles', durationMs: 4000 });
        return;
      }

      const entries = registerFileEntries(pngFiles.map((file) => file.name));
      resetProcessingState(pngFiles.length);
      applyStatus({ type: 'info', messageKey: 'common.conversionRunning', durationMs: 0 });

      const shouldCreateZip = !isFixedOutputActive && Boolean(state.settings.createZip) && pngFiles.length > 1;
      const zipCandidates: ConversionOutcome[] = [];
      let successCount = 0;
      const format = getFormat(state.activeFormatId) ?? getDefaultFormat();

      for (let index = 0; index < pngFiles.length; index += 1) {
        const file = pngFiles[index];
        const entry = entries[index];
        patchFileEntry(entry.id, { status: 'processing' });

        let originalSize = 0;

        try {
          const arrayBuffer = await readFileAsArrayBuffer(file);
          const pngData = new Uint8Array(arrayBuffer);
          originalSize = pngData.length;
          const result = await convertPngBytes(pngData);
          const outputName = file.name.replace(/\.png$/i, `.${format.outputExtension}`);
          const relativeCandidate = sanitizeRelativeOutputPath(file.name);
          const targetRelativePath = (relativeCandidate && relativeCandidate.replace(/\.png$/i, `.${format.outputExtension}`)) || outputName;

          if (shouldCreateZip) {
            zipCandidates.push({ filename: outputName, data: result, originalSize: pngData.length, convertedSize: result.length });
            successCount += 1;
          } else {
            const saveResult = await saveSingleFile(outputName, result, targetRelativePath);
            if (saveResult.success) {
              successCount += 1;
            }
          }

          patchFileEntry(entry.id, {
            status: 'success',
            originalSize: pngData.length,
            convertedSize: result.length,
          });
        } catch (error) {
          const failure = error as Error & { code?: string };
          const errorInfo = resolveConversionError(failure, format, originalSize);
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
          applyStatus({ type: 'error', messageKey: errorInfo.errorMessageKey, durationMs: 5000, params: errorInfo.errorParams });
        }

        updateProgress(index + 1, pngFiles.length, index + 1 === pngFiles.length);
      }

      if (shouldCreateZip && zipCandidates.length > 0) {
        await downloadZipFile(zipCandidates);
      } else if (!shouldCreateZip && successCount > 0) {
        const messageKey = successCount === 1 ? 'common.filesConvertedOne' : 'common.filesConvertedMany';
        applyStatus({ type: 'success', messageKey, durationMs: 4000, params: successCount === 1 ? {} : { count: successCount } });
      }

      finalizeProcessingState();
    },
    [
      applyStatus,
      convertPngBytes,
      downloadZipFile,
      finalizeProcessingState,
      patchFileEntry,
      registerFileEntries,
      resetProcessingState,
      saveSingleFile,
      state.activeFormatId,
      state.isProcessing,
      state.settings.createZip,
      isFixedOutputActive,
      updateProgress,
    ]
  );

  const processFolder = useCallback(
    async (folderPath: string) => {
      if (state.isProcessing) {
        applyStatus({ type: 'info', messageKey: 'common.processingWait', durationMs: 4000 });
        return;
      }

      const electron = ensureElectronAPI();
      resetProcessingState(0);
      applyStatus({ type: 'info', messageKey: 'common.folderLoading', durationMs: 0 });

      const format = getFormat(state.activeFormatId) ?? getDefaultFormat();

      try {
        const result = await electron.readDirectory(folderPath);
        if (!result.success || !result.files) {
          throw new Error(result.error || 'Failed to read directory');
        }

        const files: FolderFileEntry[] = result.files.map((file: ElectronDirectoryEntry) => ({
          name: file.name,
          path: file.path,
          data: new Uint8Array(file.data),
        }));

        if (files.length === 0) {
          applyStatus({ type: 'error', messageKey: 'common.noPngFiles', durationMs: 4000 });
          finalizeProcessingState();
          return;
        }

        const entries = registerFileEntries(files.map((file) => file.name));
        dispatch({ type: 'setProcessing', processing: true });
        updateProgress(0, files.length);
        applyStatus({ type: 'info', messageKey: 'common.conversionRunning', durationMs: 0 });

        const deleteOriginal = isFixedOutputActive ? false : Boolean(state.settings.deletePng) && format.id !== 'pngx';

        for (let index = 0; index < files.length; index += 1) {
          const file = files[index];
          const entry = entries[index];
          patchFileEntry(entry.id, { status: 'processing' });

          const originalSize = file.data.length;

          try {
            const resultBytes = await convertPngBytes(file.data);
            if (isFixedOutputActive) {
              const relativeCandidate = sanitizeRelativeOutputPath(file.name);
              const relativePath = (relativeCandidate && relativeCandidate.replace(/\.png$/i, `.${format.outputExtension}`)) || file.name.replace(/\.png$/i, `.${format.outputExtension}`);
              await writeToFixedDirectory(relativePath, resultBytes);
            } else {
              let outputPath: string;
              if (format.id === 'pngx' && !deleteOriginal) {
                outputPath = file.path.replace(/\.png$/i, '_optimized.png');
              } else {
                outputPath = file.path.replace(/\.png$/i, `.${format.outputExtension}`);
              }

              const writeResult = await electron.writeFile(outputPath, resultBytes);
              if (!writeResult.success) {
                applyStatus({ type: 'error', messageKey: 'common.fileWriteFailed', durationMs: 5000, params: { path: outputPath } });
                throw new Error(writeResult.error || 'Failed to write file');
              }
            }

            if (deleteOriginal) {
              const deleteResult = await electron.deleteFile(file.path);
              if (!deleteResult.success) {
                applyStatus({ type: 'error', messageKey: 'common.deletePngFailed', durationMs: 5000, params: { path: file.path } });
              }
            }

            patchFileEntry(entry.id, {
              status: 'success',
              originalSize: file.data.length,
              convertedSize: resultBytes.length,
            });
          } catch (error) {
            const failure = error as Error & { code?: string };
            const errorInfo = resolveConversionError(failure, format, originalSize);
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
            applyStatus({ type: 'error', messageKey: errorInfo.errorMessageKey, durationMs: 5000, params: errorInfo.errorParams });
          }

          updateProgress(index + 1, files.length, index + 1 === files.length);
        }

        const messageKey = files.length === 1 ? 'common.filesConvertedOne' : 'common.filesConvertedMany';
        applyStatus({ type: 'success', messageKey, durationMs: 4000, params: files.length === 1 ? {} : { count: files.length } });
      } catch (error) {
        applyStatus({ type: 'error', messageKey: 'common.errorPrefix', durationMs: 5000, params: { message: (error as Error).message ?? t('common.unknownError') } });
      } finally {
        finalizeProcessingState();
      }
    },
    [
      applyStatus,
      convertPngBytes,
      currentFormat,
      finalizeProcessingState,
      patchFileEntry,
      registerFileEntries,
      resetProcessingState,
      resolveConversionError,
      state.activeFormatId,
      state.isProcessing,
      state.settings.deletePng,
      isFixedOutputActive,
      writeToFixedDirectory,
      t,
      updateProgress,
    ]
  );

  const handleDrop = useCallback(
    async (event: React.DragEvent<HTMLDivElement>) => {
      event.preventDefault();
      event.stopPropagation();
      setIsDragOver(false);
      await processDroppedFiles(event.dataTransfer.files);
    },
    [processDroppedFiles]
  );

  const handleSelectFolder = useCallback(async () => {
    try {
      const electron = ensureElectronAPI();
      const result = await electron.selectFolder();
      if (!result.success || !result.folderPath) {
        if (result?.canceled) {
          return;
        }
        throw new Error(result.error || 'Failed to select folder');
      }
      await processFolder(result.folderPath);
    } catch (error) {
      applyStatus({ type: 'error', messageKey: 'common.folderSelectFailed', durationMs: 5000, params: { error: (error as Error).message ?? t('common.unknownError') } });
    }
  }, [applyStatus, processFolder, t]);

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

  const renderStatusMessage = () => {
    if (!state.status) {
      return null;
    }
    const statusClass = state.status.type === 'success' ? styles.statusMessageSuccess : state.status.type === 'error' ? styles.statusMessageError : styles.statusMessageInfo;
    return <div className={`${styles.statusMessage} ${statusClass}`}>{t(state.status.messageKey, state.status.params)}</div>;
  };

  const renderFileList = () => {
    if (state.fileEntries.length === 0) {
      return null;
    }
    return (
      <div className={styles.fileList}>
        {state.fileEntries.map((entry) => {
          const fileStatusClass =
            entry.status === 'success'
              ? styles.fileStatusSuccess
              : entry.status === 'processing'
                ? styles.fileStatusProcessing
                : entry.status === 'error'
                  ? styles.fileStatusError
                  : styles.fileStatusPending;
          const isError = entry.status === 'error';
          const errorTooltip = isError ? getErrorMessageForEntry(entry) : undefined;
          return (
            <div className={styles.fileItem} key={entry.id}>
              <div className={styles.fileItemHeader}>
                <span className={styles.fileName} title={entry.name}>
                  {entry.name}
                </span>
                {isError ? (
                  <button
                    type="button"
                    className={`${styles.fileStatus} ${fileStatusClass} ${styles.fileStatusInteractive}`}
                    onClick={() => {
                      void handleCopyErrorMessage(entry);
                    }}
                    onKeyDown={(event) => {
                      if (event.key === 'Enter' || event.key === ' ') {
                        event.preventDefault();
                        void handleCopyErrorMessage(entry);
                      }
                    }}
                  >
                    {formatStatusText(entry)}
                    <div className={styles.fileStatusTooltip} role="tooltip">
                      <div className={styles.fileStatusTooltipMessage}>{errorTooltip}</div>
                      <div className={styles.fileStatusTooltipHint}>{t('errors.copyHint')}</div>
                    </div>
                  </button>
                ) : (
                  <span className={`${styles.fileStatus} ${fileStatusClass}`}>{formatStatusText(entry)}</span>
                )}
              </div>
              {entry.originalSize !== undefined && entry.convertedSize !== undefined && (
                <div className={styles.fileSizeInfo}>{`${formatBytes(entry.originalSize)} → ${formatBytes(entry.convertedSize)}`}</div>
              )}
            </div>
          );
        })}
      </div>
    );
  };

  const renderProgress = () => {
    if (state.progress.total === 0) {
      return null;
    }
    const percent = state.progress.total === 0 ? 0 : Math.round((state.progress.current / state.progress.total) * 100);
    const progressText = state.progress.state === 'done' ? t('utils.progressDone') : t('utils.progressProcessing', { current: state.progress.current, total: state.progress.total });
    return (
      <div className={styles.progressContainer}>
        <div className={styles.progressBar}>
          <div className={styles.progressFill} style={{ width: `${percent}%` }} />
        </div>
        <div className={styles.progressText}>{progressText}</div>
      </div>
    );
  };

  const renderSettingsPanel = () => {
    return (
      <div id="settingsMenu" className={styles.settingsMenu}>
        <div className={styles.settingsItem}>
          <label htmlFor="outputDirectoryCheckbox">
            <input
              id="outputDirectoryCheckbox"
              type="checkbox"
              checked={isFixedOutputActive}
              disabled={isSelectingOutputDirectory || state.isProcessing}
              onChange={(event) => {
                void handleOutputModeToggle(event.target.checked);
              }}
            />
            <span>{t('common.outputDirectory.label')}</span>
          </label>
          <div className={styles.settingsDescription}>
            <div>{t(isFixedOutputActive ? 'common.outputDirectory.descriptionFixed' : 'common.outputDirectory.descriptionDefault')}</div>
            {isFixedOutputActive && (
              <div className={styles.outputDirectoryRow}>
                <span className={styles.outputDirectoryPath}>{normalizedOutputDirectoryPath || t('common.outputDirectory.missingPath')}</span>
                <button type="button" className={styles.outputDirectoryButton} disabled={isSelectingOutputDirectory || state.isProcessing} onClick={() => void handleOutputDirectoryReselect()}>
                  {t('common.outputDirectory.changeButton')}
                </button>
              </div>
            )}
          </div>
        </div>
        <div className={styles.settingsItem}>
          <label htmlFor="deletePngCheckbox">
            <input
              id="deletePngCheckbox"
              type="checkbox"
              checked={Boolean(state.settings.deletePng) && !isFixedOutputActive}
              disabled={isFixedOutputActive}
              onChange={(event) => handleSettingToggle('deletePng', event.target.checked)}
            />
            <span>{t('electron.settings.deletePngLabel')}</span>
          </label>
          <div className={styles.settingsDescription}>{t('electron.settings.deletePngDescription')}</div>
          {isFixedOutputActive && <div className={styles.settingLockedNote}>{t('common.outputDirectory.lockedOptionNote')}</div>}
        </div>
        <div className={styles.settingsItem}>
          <label htmlFor="createZipCheckbox">
            <input
              id="createZipCheckbox"
              type="checkbox"
              checked={Boolean(state.settings.createZip) && !isFixedOutputActive}
              disabled={isFixedOutputActive}
              onChange={(event) => handleSettingToggle('createZip', event.target.checked)}
            />
            <span>{t('electron.settings.createZipLabel')}</span>
          </label>
          <div className={styles.settingsDescription}>{t('electron.settings.createZipDescription')}</div>
          {isFixedOutputActive && <div className={styles.settingLockedNote}>{t('common.outputDirectory.lockedOptionNote')}</div>}
        </div>
      </div>
    );
  };

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
    return <BuildInfoPanel title={t('electron.title')} buildInfo={state.buildInfo} t={t} />;
  };

  if (initializing) {
    return <div className={styles.container} />;
  }

  const activeFormatId = currentFormat.id;
  const dropZoneClassName = [styles.dropZone, state.isProcessing ? styles.dropZoneProcessing : '', isDragOver ? styles.dropZoneDragOver : ''].filter(Boolean).join(' ');

  return (
    <div className={styles.container}>
      <div className={styles.header}>
        <h1 className={styles.title}>{t('electron.title')}</h1>
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
                onClick={() => handleFormatChange(format.id)}
              >
                {t(format.labelKey)}
              </button>
            ))}
          </div>
          <select className={styles.languageSelect} value={language} onChange={handleLanguageChange} aria-label={t('header.languageSelect.aria')}>
            {availableLanguages.map((langItem) => (
              <option key={langItem.code} value={langItem.code} title={langItem.label}>
                {langItem.flag}
              </option>
            ))}
          </select>
          <button type="button" className={styles.themeToggleButton} onClick={handleThemeToggle} title={t('header.themeToggle.title')}>
            {state.theme === 'dark' ? '☀️' : '🌙'}
          </button>
          <button
            type="button"
            className={styles.settingsButton}
            onClick={handleAdvancedSettingsClick}
            aria-haspopup="dialog"
            aria-label={t('settingsMenu.advancedSettings')}
            title={t('settingsMenu.advancedSettings')}
          >
            ⚙️
          </button>
        </div>
      </div>

      <div
        className={dropZoneClassName}
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
        onClick={() => {
          if (!state.isProcessing) {
            void handleSelectFolder();
          }
        }}
      >
        <div className={styles.dropZoneIcon}>📁</div>
        <div className={styles.dropZoneText}>{t('electron.dropzone.text')}</div>
        <div className={styles.dropZoneHint}>{t('electron.dropzone.hint')}</div>
      </div>

      {renderSettingsPanel()}

      {renderProgress()}
      {renderFileList()}
      {renderStatusMessage()}
      {renderBuildInfo()}
      {renderResetButton()}
    </div>
  );
};

const ElectronApp: React.FC = () => {
  return (
    <I18nProvider>
      <AppStateProvider>
        <ThemeProvider>
          <ElectronAppInner />
        </ThemeProvider>
      </AppStateProvider>
    </I18nProvider>
  );
};

export default ElectronApp;
