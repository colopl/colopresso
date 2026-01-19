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
import { initializeModule, isThreadsEnabled, ModuleWithHelpers } from '../../shared/core/converter';
import { createConversionWorkerClient, ConversionWorkerClient } from '../../shared/core/conversionWorkerClient';
import { readFileAsArrayBuffer } from '../../shared/core/files';
import { FileEntry, FormatDefinition, FormatOptions, SettingsState, StatusMessage, BuildInfoPayload, SETTINGS_SCHEMA_VERSION, SETTINGS_SCHEMA_VERSION_STORAGE_KEY } from '../../shared/types';
import BuildInfoPanel from '../../shared/components/BuildInfoPanel';
import { AppHeader, DropZone, FileList, ProgressBar, SettingsPanel, StatusMessage as StatusMessageComponent, SettingItemConfig } from '../../shared/components';

const ELECTRON_SETTINGS_STORAGE_KEY = 'settings';
const CHECK_FOR_UPDATES_AFTER_RELOAD_KEY = 'checkForUpdatesAfterReload';
const COLOPRESSO_MODULE_URL = new URL(/* @vite-ignore */ './colopresso.js', import.meta.url);
const COLOPRESSO_WORKER_URL = new URL(/* @vite-ignore */ './conversionWorker.js', import.meta.url);

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
  const [deferredDownloadVersion, setDeferredDownloadVersion] = useState<string | null>(null);
  const [deferredUpdateVersion, setDeferredUpdateVersion] = useState<string | null>(null);
  const [isUpdateDownloading, setIsUpdateDownloading] = useState(false);
  const [isUpdateRestarting, setIsUpdateRestarting] = useState(false);
  const moduleRef = useRef<ModuleWithHelpers | null>(null);
  const modulePromiseRef = useRef<Promise<ModuleWithHelpers> | null>(null);
  const workerClientRef = useRef<ConversionWorkerClient | null>(null);
  const workerClientPromiseRef = useRef<Promise<ConversionWorkerClient> | null>(null);
  const workerInitResultRef = useRef<{
    threadsEnabled: boolean;
    pngxBridgeEnabled: boolean;
    version?: number;
    libwebpVersion?: number;
    libpngVersion?: number;
    libavifVersion?: number;
    pngxOxipngVersion?: number;
    pngxLibimagequantVersion?: number;
    buildtime?: number;
  } | null>(null);
  const threadsEnabledRef = useRef<boolean>(true);
  const statusTimerRef = useRef<number | null>(null);
  const advancedSettingsControllerRef = useRef<AdvancedSettingsController | null>(null);
  const latestAdvancedFormatRef = useRef<string | null>(null);
  const latestAdvancedConfigRef = useRef<FormatOptions | null>(null);
  const cancelRequestedRef = useRef<boolean>(false);
  const [isCancelling, setIsCancelling] = useState(false);
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
    const moduleUrl = COLOPRESSO_MODULE_URL.href;
    const creation = loadColopressoModuleFactory()
      .then((factory) =>
        initializeModule(factory, {
          locateFile,
          mainScriptUrlOrBlob: moduleUrl,
        })
      )
      .then((Module) => {
        moduleRef.current = Module;
        threadsEnabledRef.current = isThreadsEnabled(Module);

        return Module;
      })
      .finally(() => {
        modulePromiseRef.current = null;
      });
    modulePromiseRef.current = creation;
    return creation;
  }, []);

  const ensureWorkerClient = useCallback(async (): Promise<ConversionWorkerClient> => {
    if (workerClientRef.current) {
      return workerClientRef.current;
    }
    if (workerClientPromiseRef.current) {
      return workerClientPromiseRef.current;
    }

    const creation = (async () => {
      const pngxBridgeUrl = await window.electronAPI?.getPngxBridgeUrl?.();
      let pngxThreads: number | undefined;

      try {
        const pngxFormat = getFormat('pngx');
        if (pngxFormat) {
          const pngxConfig = await loadFormatConfig(pngxFormat);
          pngxThreads = typeof pngxConfig.pngx_threads === 'number' ? pngxConfig.pngx_threads : undefined;
        }
      } catch {
        /* NOP */
      }

      const client = createConversionWorkerClient(COLOPRESSO_WORKER_URL, COLOPRESSO_MODULE_URL.href, {
        pngxBridgeUrl,
        pngxThreads,
      });
      const initResult = await client.init();

      threadsEnabledRef.current = initResult.threadsEnabled;
      workerInitResultRef.current = initResult;
      workerClientRef.current = client;

      return client;
    })();

    workerClientPromiseRef.current = creation;

    try {
      return await creation;
    } finally {
      workerClientPromiseRef.current = null;
    }
  }, []);

  const loadBuildInfoFromModule = useCallback(async (): Promise<BuildInfoPayload | null> => {
    try {
      await ensureWorkerClient();

      const initResult = workerInitResultRef.current;
      if (!initResult) {
        return null;
      }

      const getUpdateChannel = window.electronAPI?.getUpdateChannel;
      const getArchitecture = window.electronAPI?.getArchitecture;
      let releaseChannel: string | undefined;
      let architecture: string | undefined;

      if (typeof getUpdateChannel === 'function') {
        const channel = await getUpdateChannel();
        if (typeof channel === 'string' && channel.trim().length > 0) {
          releaseChannel = channel.trim();
        }
      }

      if (typeof getArchitecture === 'function') {
        const arch = await getArchitecture();
        if (typeof arch === 'string' && arch.trim().length > 0) {
          architecture = arch.trim();
        }
      }

      const base: BuildInfoPayload = {
        version: initResult.version,
        libavifVersion: initResult.libavifVersion,
        libwebpVersion: initResult.libwebpVersion,
        libpngVersion: initResult.libpngVersion,
        pngxOxipngVersion: initResult.pngxOxipngVersion,
        pngxLibimagequantVersion: initResult.pngxLibimagequantVersion,
        compilerVersionString: initResult.compilerVersionString,
        rustVersionString: initResult.rustVersionString,
        buildtime: initResult.buildtime,
      };

      if (releaseChannel || architecture) {
        return {
          ...base,
          ...(releaseChannel ? { releaseChannel } : {}),
          ...(architecture ? { architecture } : {}),
        };
      }

      return base;
    } catch (_error) {
      return null;
    }
  }, [ensureWorkerClient]);

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

  useEffect(() => {
    const api = window.electronAPI;
    if (!api) {
      return undefined;
    }

    let updatePhase: 'downloading' | 'extracting' = 'downloading';
    let maxPercentSeen = 0;
    let lastTransferred: number | undefined;

    const offStart = api.onUpdateDownloadStart?.((payload) => {
      const typed = (payload ?? {}) as { version?: string };
      const progressText = t('updater.toast.progressUnknown');

      updatePhase = 'downloading';
      maxPercentSeen = 0;
      lastTransferred = undefined;
      setDeferredDownloadVersion(null);
      setIsUpdateDownloading(true);

      applyStatusRef.current({ type: 'info', messageKey: 'updater.toast.downloading', durationMs: 0, params: { progress: progressText, version: typed.version } });
    });

    const offProgress = api.onUpdateDownloadProgress?.((payload) => {
      const typed = (payload ?? {}) as { percent?: number; transferred?: number; version?: string };
      const progressValue = typeof typed.percent === 'number' && Number.isFinite(typed.percent) ? Math.max(0, Math.min(100, typed.percent)) : undefined;
      const progressText = typeof progressValue === 'number' ? `${Math.round(progressValue)}%` : t('updater.toast.progressUnknown');

      if (typeof progressValue === 'number') {
        maxPercentSeen = Math.max(maxPercentSeen, progressValue);
      }

      const transferred = typeof typed.transferred === 'number' && Number.isFinite(typed.transferred) ? typed.transferred : undefined;
      const likelyResetAfterComplete =
        updatePhase === 'downloading' &&
        maxPercentSeen >= 99.5 &&
        ((typeof progressValue === 'number' && progressValue <= 5) || (typeof transferred === 'number' && typeof lastTransferred === 'number' && transferred < lastTransferred));

      if (likelyResetAfterComplete) {
        updatePhase = 'extracting';
      }

      lastTransferred = transferred ?? lastTransferred;

      const messageKey = updatePhase === 'extracting' ? 'updater.toast.extracting' : 'updater.toast.downloading';

      applyStatusRef.current({ type: 'info', messageKey, durationMs: 0, params: { progress: progressText, version: typed.version } });
    });

    const offComplete = api.onUpdateDownloadComplete?.((payload) => {
      const typed = (payload ?? {}) as { version?: string };

      setIsUpdateDownloading(false);

      applyStatusRef.current({ type: 'success', messageKey: 'updater.toast.downloaded', durationMs: 2000, params: { version: typed.version } });
    });

    const offDownloadDeferred = api.onUpdateDownloadDeferred?.((payload) => {
      const typed = (payload ?? {}) as { version?: string };
      const version = typeof typed.version === 'string' ? typed.version.trim() : '';

      setDeferredDownloadVersion(version.length > 0 ? version : '');
    });

    const offDeferred = api.onUpdateInstallDeferred?.((payload) => {
      const typed = (payload ?? {}) as { version?: string };
      const version = typeof typed.version === 'string' ? typed.version.trim() : '';

      setDeferredDownloadVersion(null);
      setDeferredUpdateVersion(version.length > 0 ? version : '');
    });

    const offError = api.onUpdateDownloadError?.((payload) => {
      const typed = (payload ?? {}) as { message?: unknown };
      const message = typeof typed.message === 'string' && typed.message.trim().length > 0 ? typed.message : t('common.unknownError');

      setIsUpdateDownloading(false);

      applyStatusRef.current({ type: 'error', messageKey: 'updater.toast.downloadFailed', durationMs: 0, params: { message } });
    });

    return () => {
      offStart?.();
      offProgress?.();
      offComplete?.();
      offDownloadDeferred?.();
      offDeferred?.();
      offError?.();
    };
  }, [t]);

  const handleDownloadUpdateNow = useCallback(async () => {
    const api = window.electronAPI;
    if (!api?.downloadUpdateNow) {
      applyStatus({ type: 'error', messageKey: 'common.unknownError', durationMs: 4000 });
      return;
    }

    if (isUpdateDownloading || isUpdateRestarting) {
      return;
    }

    setIsUpdateDownloading(true);
    setDeferredDownloadVersion(null);

    try {
      const result = await api.downloadUpdateNow();
      if (!result?.success) {
        const message = typeof result?.error === 'string' && result.error.trim().length > 0 ? result.error : t('common.unknownError');

        applyStatus({ type: 'error', messageKey: 'updater.toast.downloadFailed', durationMs: 0, params: { message } });

        setIsUpdateDownloading(false);
      }
    } catch (error) {
      const message = (error as Error | undefined)?.message ?? t('common.unknownError');

      applyStatus({ type: 'error', messageKey: 'updater.toast.downloadFailed', durationMs: 0, params: { message } });

      setIsUpdateDownloading(false);
    }
  }, [applyStatus, isUpdateDownloading, isUpdateRestarting, t]);

  const handleInstallUpdateNow = useCallback(async () => {
    const api = window.electronAPI;
    if (!api?.installUpdateNow) {
      applyStatus({ type: 'error', messageKey: 'common.unknownError', durationMs: 4000 });
      return;
    }

    if (isUpdateRestarting) {
      return;
    }

    const versionLabel = deferredUpdateVersion && deferredUpdateVersion.trim().length > 0 ? deferredUpdateVersion : '';
    if (api.confirmInstallUpdate) {
      const confirmed = await api.confirmInstallUpdate({ version: versionLabel });
      if (!confirmed?.confirmed) {
        return;
      }
    } else {
      const confirmation = `${t('updater.dialog.readyToInstall.title')}\n\n${t('updater.dialog.readyToInstall.message', { version: versionLabel })}\n\n${t('updater.dialog.readyToInstall.detail')}`;
      if (typeof window !== 'undefined' && !window.confirm(confirmation)) {
        return;
      }
    }

    setIsUpdateRestarting(true);
    applyStatus({ type: 'info', messageKey: 'updater.toast.restarting', durationMs: 0 });

    try {
      const result = await api.installUpdateNow();
      if (!result?.success) {
        const message = typeof result?.error === 'string' && result.error.trim().length > 0 ? result.error : t('common.unknownError');

        applyStatus({ type: 'error', messageKey: 'updater.toast.downloadFailed', durationMs: 0, params: { message } });
        setIsUpdateRestarting(false);
      }
    } catch (error) {
      const message = (error as Error | undefined)?.message ?? t('common.unknownError');

      applyStatus({ type: 'error', messageKey: 'updater.toast.downloadFailed', durationMs: 0, params: { message } });
      setIsUpdateRestarting(false);
    }
  }, [applyStatus, deferredUpdateVersion, isUpdateRestarting, t]);

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

  const copyTextToClipboard = useCallback(
    async (message: string, options?: { restoreStatus?: StatusMessage | null }) => {
      const restoreStatus = options?.restoreStatus ?? null;
      const scheduleRestore = (durationMs: number) => {
        if (!restoreStatus) {
          return;
        }

        window.setTimeout(() => {
          applyStatus(restoreStatus);
        }, durationMs);
      };

      if (!message) {
        applyStatus({ type: 'error', messageKey: 'errors.copyFailed', durationMs: 4000 });
        scheduleRestore(4000);
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
        scheduleRestore(2000);
      } catch (_copyError) {
        applyStatus({ type: 'error', messageKey: 'errors.copyFailed', durationMs: 4000 });
        scheduleRestore(4000);
      }
    },
    [applyStatus]
  );

  const handleCopyErrorMessage = useCallback(
    async (entry: FileEntry) => {
      const message = getErrorMessageForEntry(entry);
      await copyTextToClipboard(message);
    },
    [copyTextToClipboard, getErrorMessageForEntry]
  );

  const handleCopyStatusMessage = useCallback(
    async (message: string, statusToRestore: StatusMessage | null) => {
      await copyTextToClipboard(message, statusToRestore ? { restoreStatus: statusToRestore } : undefined);
    },
    [copyTextToClipboard]
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
    const shouldCheck = sessionStorage.getItem(CHECK_FOR_UPDATES_AFTER_RELOAD_KEY);
    if (shouldCheck) {
      sessionStorage.removeItem(CHECK_FOR_UPDATES_AFTER_RELOAD_KEY);
      if (window.electronAPI?.checkForUpdates) {
        void window.electronAPI.checkForUpdates();
      }
    }
  }, []);

  useEffect(() => {
    return () => {
      moduleRef.current = null;
      modulePromiseRef.current = null;
      if (workerClientRef.current) {
        workerClientRef.current.terminate();
        workerClientRef.current = null;
      }
      workerClientPromiseRef.current = null;
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

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape' && state.isProcessing) {
        cancelRequestedRef.current = true;
        setIsCancelling(true);

        if (workerClientRef.current) {
          workerClientRef.current.terminate();
          workerClientRef.current = null;
        }
        workerClientPromiseRef.current = null;
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
      sessionStorage.setItem(CHECK_FOR_UPDATES_AFTER_RELOAD_KEY, 'true');
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
      cancelRequestedRef.current = false;
      setIsCancelling(false);
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
    cancelRequestedRef.current = false;
    setIsCancelling(false);
    dispatch({ type: 'setProcessing', processing: false });
  }, [dispatch]);

  const handleCancelProcessing = useCallback(() => {
    cancelRequestedRef.current = true;
    setIsCancelling(true);

    if (workerClientRef.current) {
      workerClientRef.current.terminate();
      workerClientRef.current = null;
    }
    workerClientPromiseRef.current = null;
  }, []);

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
    async (pngData: Uint8Array): Promise<{ result: Uint8Array; conversionTimeMs: number }> => {
      const workerClient = await ensureWorkerClient();
      const format = getFormat(state.activeFormatId) ?? getDefaultFormat();
      const options = normalizeFormatOptions(format, formatConfigs[format.id]);
      const startTime = performance.now();
      const { outputBytes } = await workerClient.convert(format.id, options, pngData);
      const endTime = performance.now();
      const conversionTimeMs = Math.round(endTime - startTime);

      return { result: outputBytes, conversionTimeMs };
    },
    [ensureWorkerClient, formatConfigs, state.activeFormatId]
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
        if (cancelRequestedRef.current) {
          dispatch({ type: 'setFileEntries', entries: [] });
          dispatch({ type: 'setProgress', progress: { current: 0, total: 0, state: 'idle' } });
          applyStatus({ type: 'info', messageKey: 'common.conversionCanceled', durationMs: 4000 });
          finalizeProcessingState();
          return;
        }

        const file = pngFiles[index];
        const entry = entries[index];
        patchFileEntry(entry.id, { status: 'processing' });

        let originalSize = 0;

        try {
          const arrayBuffer = await readFileAsArrayBuffer(file);
          const pngData = new Uint8Array(arrayBuffer);
          const { result, conversionTimeMs } = await convertPngBytes(pngData);
          const outputName = file.name.replace(/\.png$/i, `.${format.outputExtension}`);
          const relativeCandidate = sanitizeRelativeOutputPath(file.name);
          const targetRelativePath = (relativeCandidate && relativeCandidate.replace(/\.png$/i, `.${format.outputExtension}`)) || outputName;

          originalSize = pngData.length;

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
            conversionTimeMs,
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

        const deleteOriginal = !isFixedOutputActive && Boolean(state.settings.deletePng);
        const overwriteOriginalForPngx = deleteOriginal && format.id === 'pngx';

        for (let index = 0; index < files.length; index += 1) {
          if (cancelRequestedRef.current) {
            dispatch({ type: 'setFileEntries', entries: [] });
            dispatch({ type: 'setProgress', progress: { current: 0, total: 0, state: 'idle' } });
            applyStatus({ type: 'info', messageKey: 'common.conversionCanceled', durationMs: 4000 });
            finalizeProcessingState();
            return;
          }

          const file = files[index];
          const entry = entries[index];
          patchFileEntry(entry.id, { status: 'processing' });

          const originalSize = file.data.length;

          try {
            const { result: resultBytes, conversionTimeMs } = await convertPngBytes(file.data);
            let outputPath: string | null = null;

            if (isFixedOutputActive) {
              const relativeCandidate = sanitizeRelativeOutputPath(file.name);
              const relativePath = (relativeCandidate && relativeCandidate.replace(/\.png$/i, `.${format.outputExtension}`)) || file.name.replace(/\.png$/i, `.${format.outputExtension}`);

              await writeToFixedDirectory(relativePath, resultBytes);
            } else {
              if (format.id === 'pngx' && !overwriteOriginalForPngx) {
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

            const shouldDeleteOriginalFile = deleteOriginal && outputPath !== null && outputPath !== file.path;
            if (shouldDeleteOriginalFile) {
              const deleteResult = await electron.deleteFile(file.path);
              if (!deleteResult.success) {
                applyStatus({ type: 'error', messageKey: 'common.deletePngFailed', durationMs: 5000, params: { path: file.path } });
              }
            }

            patchFileEntry(entry.id, {
              status: 'success',
              originalSize: file.data.length,
              convertedSize: resultBytes.length,
              conversionTimeMs,
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

  const statusMessage = useMemo(() => {
    if (!state.status) {
      return null;
    }

    return t(state.status.messageKey, state.status.params);
  }, [state.status, t]);

  const statusAction = useMemo(() => {
    if (!state.status || state.status.messageKey !== 'updater.toast.downloadFailed') {
      return undefined;
    }

    const message = t(state.status.messageKey, state.status.params);

    return {
      label: t('errors.copyAction'),
      title: t('errors.copyHint'),
      onClick: () => {
        void handleCopyStatusMessage(message, state.status);
      },
    };
  }, [state.status, t, handleCopyStatusMessage]);

  const progressText = useMemo(() => {
    if (state.progress.total === 0) {
      return '';
    }

    return state.progress.state === 'done' ? t('utils.progressDone') : t('utils.progressProcessing', { current: state.progress.current, total: state.progress.total });
  }, [state.progress.current, state.progress.total, state.progress.state, t]);

  const settingsItems: SettingItemConfig[] = useMemo(
    () => [
      {
        id: 'outputDirectoryCheckbox',
        type: 'custom',
        labelKey: 'common.outputDirectory.label',
        customContent: (
          <>
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
          </>
        ),
      },
      {
        id: 'deletePngCheckbox',
        type: 'checkbox',
        labelKey: 'electron.settings.deletePngLabel',
        descriptionKey: 'electron.settings.deletePngDescription',
        value: Boolean(state.settings.deletePng) && !isFixedOutputActive,
        disabled: isFixedOutputActive,
        locked: isFixedOutputActive,
        lockedNoteKey: 'common.outputDirectory.lockedOptionNote',
        onChange: (value) => handleSettingToggle('deletePng', value as boolean),
      },
      {
        id: 'createZipCheckbox',
        type: 'checkbox',
        labelKey: 'electron.settings.createZipLabel',
        descriptionKey: 'electron.settings.createZipDescription',
        value: Boolean(state.settings.createZip) && !isFixedOutputActive,
        disabled: isFixedOutputActive,
        locked: isFixedOutputActive,
        lockedNoteKey: 'common.outputDirectory.lockedOptionNote',
        onChange: (value) => handleSettingToggle('createZip', value as boolean),
      },
    ],
    [
      isFixedOutputActive,
      isSelectingOutputDirectory,
      state.isProcessing,
      state.settings.deletePng,
      state.settings.createZip,
      normalizedOutputDirectoryPath,
      handleOutputModeToggle,
      handleOutputDirectoryReselect,
      handleSettingToggle,
      t,
    ]
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
    return <BuildInfoPanel title={t('electron.title')} buildInfo={state.buildInfo} t={t} />;
  };

  if (initializing) {
    return <div className={styles.container} />;
  }

  const activeFormatId = currentFormat.id;

  return (
    <div className={styles.container}>
      <AppHeader
        title={t('electron.title')}
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

      {(deferredUpdateVersion !== null || deferredDownloadVersion !== null) && (
        <div className={styles.dropZoneUpdateCta}>
          <button
            type="button"
            className={styles.dropZoneUpdateCtaButton}
            onClick={() => {
              if (deferredUpdateVersion !== null) {
                void handleInstallUpdateNow();
                return;
              }
              void handleDownloadUpdateNow();
            }}
            disabled={isUpdateRestarting || isUpdateDownloading}
            aria-label={t(deferredUpdateVersion !== null ? 'updater.cta.restartToUpdate' : 'updater.cta.downloadUpdate')}
            title={t(deferredUpdateVersion !== null ? 'updater.cta.restartToUpdate' : 'updater.cta.downloadUpdate')}
          >
            {t(deferredUpdateVersion !== null ? 'updater.cta.restartToUpdate' : 'updater.cta.downloadUpdate')}
          </button>
        </div>
      )}

      <DropZone
        iconEmoji="📁"
        text={t('electron.dropzone.text')}
        hint={t('electron.dropzone.hint')}
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
        onClick={() => {
          if (!state.isProcessing) {
            void handleSelectFolder();
          }
        }}
        onCancel={handleCancelProcessing}
        cancelButtonLabel={t('common.cancelButton')}
        isCancelling={isCancelling}
        cancellingButtonLabel={t('common.cancellingButton')}
      />

      <SettingsPanel items={settingsItems} isProcessing={state.isProcessing} t={t} />

      <ProgressBar current={state.progress.current} total={state.progress.total} isVisible={state.progress.total > 0} progressText={progressText} />
      <FileList entries={state.fileEntries} formatStatusText={formatStatusText} getErrorMessage={getErrorMessageForEntry} onCopyError={handleCopyErrorMessage} formatBytes={formatBytes} t={t} />
      <StatusMessageComponent message={statusMessage} type={state.status?.type ?? 'info'} action={statusAction} />
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
