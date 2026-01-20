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

import { FormatOptions } from '../types';
import { initializeModule, prewarmThreadPool, isThreadsEnabled, getVersionInfo, ModuleWithHelpers } from './converter';
import { initPngxBridge, isPngxBridgeInitialized, pngxOxipngVersion, pngxLibimagequantVersion, pngxRustVersionString } from './pngxBridge';
import { getFormat, getDefaultFormat, normalizeFormatOptions } from '../formats';

export interface ConversionWorkerRequest {
  type: 'init' | 'convert';
  id: number;
  moduleUrl?: string;
  pngxBridgeUrl?: string;
  pngxThreads?: number;
  formatId?: string;
  options?: FormatOptions;
  inputBytes?: ArrayBuffer;
}

export interface ConversionWorkerResponse {
  type: 'init-complete' | 'convert-result' | 'error';
  id: number;
  success: boolean;
  threadsEnabled?: boolean;
  pngxBridgeEnabled?: boolean;
  version?: number;
  libwebpVersion?: number;
  libpngVersion?: number;
  libavifVersion?: number;
  pngxOxipngVersion?: number;
  pngxLibimagequantVersion?: number;
  compilerVersionString?: string;
  rustVersionString?: string;
  buildtime?: number;
  outputBytes?: ArrayBuffer;
  error?: string;
  errorCode?: string;
  inputSize?: number;
  outputSize?: number;
}

type ColopressoModuleFactory = (opts?: Record<string, unknown>) => Promise<ModuleWithHelpers>;

let moduleRef: ModuleWithHelpers | null = null;
let modulePromise: Promise<ModuleWithHelpers> | null = null;
let threadsEnabled = false;
let pngxBridgeEnabled = false;
let cachedVersionInfo: {
  version?: number;
  libwebpVersion?: number;
  libpngVersion?: number;
  libavifVersion?: number;
  pngxOxipngVersion?: number;
  pngxLibimagequantVersion?: number;
  compilerVersionString?: string;
  rustVersionString?: string;
  buildtime?: number;
} | null = null;

const postResponse = (response: ConversionWorkerResponse) => {
  const transferables: Transferable[] = [];
  if (response.outputBytes) {
    transferables.push(response.outputBytes);
  }
  (self as unknown as Worker).postMessage(response, transferables);
};

const handleInit = async (request: ConversionWorkerRequest) => {
  if (moduleRef) {
    postResponse({
      type: 'init-complete',
      id: request.id,
      success: true,
      threadsEnabled,
      pngxBridgeEnabled,
      ...cachedVersionInfo,
    });

    return;
  }

  if (modulePromise) {
    await modulePromise;
    postResponse({
      type: 'init-complete',
      id: request.id,
      success: true,
      threadsEnabled,
      pngxBridgeEnabled,
      ...cachedVersionInfo,
    });

    return;
  }

  try {
    const moduleUrl = request.moduleUrl;
    if (!moduleUrl) {
      throw new Error('moduleUrl is required for init');
    }

    const locateFile = (resource: string) => {
      try {
        return new URL(resource, moduleUrl).href;
      } catch {
        return resource;
      }
    };

    modulePromise = import(/* @vite-ignore */ moduleUrl)
      .then((mod) => mod.default as ColopressoModuleFactory)
      .then((factory) =>
        initializeModule(factory, {
          locateFile,
          mainScriptUrlOrBlob: moduleUrl,
          printErr: (...args: unknown[]) => {
            console.error('[conversionWorker][stderr]', ...args);
          },
        })
      );

    const Module = await modulePromise;
    moduleRef = Module;
    modulePromise = null;
    threadsEnabled = isThreadsEnabled(Module);

    if (threadsEnabled) {
      const hardwareConcurrency = typeof navigator !== 'undefined' && navigator.hardwareConcurrency > 0 ? navigator.hardwareConcurrency : 1;
      prewarmThreadPool(Module, hardwareConcurrency);
    }

    if (request.pngxBridgeUrl) {
      try {
        await initPngxBridge(request.pngxBridgeUrl, request.pngxThreads);
        pngxBridgeEnabled = isPngxBridgeInitialized();
      } catch (pngxError) {
        throw new Error('pngx_bridge init failed.');
      }
    } else {
      /* NOP */
    }

    const versionInfo = getVersionInfo(Module);
    const pngxOxipng = pngxBridgeEnabled ? pngxOxipngVersion() : undefined;
    const pngxLibiq = pngxBridgeEnabled ? pngxLibimagequantVersion() : undefined;
    let rustVersion = versionInfo.rustVersionString;

    if (pngxBridgeEnabled && (!rustVersion || rustVersion === 'unknown')) {
      try {
        rustVersion = pngxRustVersionString();
      } catch {
        /* NOP */
      }
    }

    cachedVersionInfo = {
      version: versionInfo.version,
      libwebpVersion: versionInfo.libwebpVersion,
      libpngVersion: versionInfo.libpngVersion,
      libavifVersion: versionInfo.libavifVersion,
      pngxOxipngVersion: pngxOxipng,
      pngxLibimagequantVersion: pngxLibiq,
      compilerVersionString: versionInfo.compilerVersionString,
      rustVersionString: rustVersion,
      buildtime: versionInfo.buildtime,
    };

    postResponse({
      type: 'init-complete',
      id: request.id,
      success: true,
      threadsEnabled,
      pngxBridgeEnabled,
      ...cachedVersionInfo,
    });
  } catch (error) {
    modulePromise = null;
    const message = error instanceof Error ? error.message : String(error);
    console.error('[conversionWorker] init failed:', message);
    postResponse({
      type: 'error',
      id: request.id,
      success: false,
      error: message,
    });
  }
};

const handleConvert = async (request: ConversionWorkerRequest) => {
  if (!moduleRef) {
    postResponse({
      type: 'error',
      id: request.id,
      success: false,
      error: 'Module not initialized. Call init first.',
    });
    return;
  }

  const { formatId, options, inputBytes } = request;
  if (!inputBytes) {
    postResponse({
      type: 'error',
      id: request.id,
      success: false,
      error: 'inputBytes is required',
    });
    return;
  }

  const format = getFormat(formatId ?? '') ?? getDefaultFormat();
  const normalizedOptions = normalizeFormatOptions(format, options);
  const inputData = new Uint8Array(inputBytes);

  try {
    const result = await format.convert({
      Module: moduleRef,
      inputBytes: inputData,
      options: normalizedOptions,
    });

    const outputBuffer = result.buffer.slice(result.byteOffset, result.byteOffset + result.byteLength) as ArrayBuffer;

    postResponse({
      type: 'convert-result',
      id: request.id,
      success: true,
      outputBytes: outputBuffer,
      inputSize: inputData.length,
      outputSize: result.length,
    });
  } catch (error) {
    const typed = error as Error & { code?: string; inputSize?: number; outputSize?: number };
    postResponse({
      type: 'error',
      id: request.id,
      success: false,
      error: typed.message ?? String(error),
      errorCode: typed.code,
      inputSize: typed.inputSize ?? inputData.length,
      outputSize: typed.outputSize,
    });
  }
};

self.onmessage = async (event: MessageEvent<ConversionWorkerRequest>) => {
  const request = event.data;

  switch (request.type) {
    case 'init':
      await handleInit(request);
      break;
    case 'convert':
      await handleConvert(request);
      break;
    default:
      postResponse({
        type: 'error',
        id: request.id,
        success: false,
        error: `Unknown request type: ${(request as ConversionWorkerRequest).type}`,
      });
  }
};
