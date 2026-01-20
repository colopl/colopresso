/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * This file is part of colopresso
 *
 * Copyright (C) 2026 COLOPL, Inc.
 *
 * Author: Go Kudo <g-kudo@colopl.co.jp>
 * Developed with AI (LLM) code assistance. See `NOTICE` for details.
 */

import { initializeModule, getVersionInfo, isThreadsEnabled, ModuleWithHelpers } from '../../shared/core/converter';
import { initPngxBridge, isPngxBridgeInitialized, pngxOxipngVersion, pngxLibimagequantVersion, pngxRustVersionString } from '../../shared/core/pngxBridge';
import { loadFormatConfig } from '../../shared/core/configStore';
import { getDefaultFormat, getFormat, normalizeFormatOptions } from '../../shared/formats';
import { FormatDefinition, FormatOptions } from '../../shared/types';
// @ts-ignore
import ColopressoModuleFactory from 'colopresso-module';

let moduleInstance: ModuleWithHelpers | null = null;
let modulePromise: Promise<ModuleWithHelpers> | null = null;
let pngxBridgeInitialized = false;

async function ensureModule(): Promise<ModuleWithHelpers> {
  if (moduleInstance) {
    return moduleInstance;
  }
  if (modulePromise) {
    return modulePromise;
  }
  const moduleUrl = new URL('colopresso.js', self.location.href).href;
  modulePromise = initializeModule(ColopressoModuleFactory, {
    locateFile: (resource: string) => new URL(resource, self.location.href).href,
    mainScriptUrlOrBlob: moduleUrl,
  })
    .then(async (Module) => {
      moduleInstance = Module;

      try {
        const pngxBridgeBaseUrl = new URL('./', self.location.href).href;

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

        await initPngxBridge(pngxBridgeBaseUrl, pngxThreads);
        pngxBridgeInitialized = isPngxBridgeInitialized();
      } catch {
        throw Error('pngx_bridge WASM init failed.');
      }

      return Module;
    })
    .finally(() => {
      modulePromise = null;
    });
  return modulePromise;
}

function resolveFormat(formatId?: string): FormatDefinition {
  if (formatId) {
    const match = getFormat(formatId);
    if (match) {
      return match;
    }
  }
  return getDefaultFormat();
}

function buildUint8ArrayFromNumericObject(source: Record<string, unknown>): Uint8Array | null {
  const numericKeys = Object.keys(source).filter((key) => /^\d+$/.test(key));
  if (numericKeys.length === 0) {
    return null;
  }

  const declaredLength = (source as { length?: unknown }).length;
  const maxIndex = numericKeys.reduce((max, key) => {
    const numericKey = Number(key);
    return Number.isFinite(numericKey) && numericKey > max ? numericKey : max;
  }, -1);

  const length = typeof declaredLength === 'number' && declaredLength > maxIndex ? declaredLength : maxIndex + 1;
  if (!Number.isFinite(length) || length <= 0) {
    return null;
  }

  const result = new Uint8Array(length);
  let assigned = 0;
  for (let index = 0; index <= maxIndex; index += 1) {
    const value = (source as Record<string, unknown>)[index];
    if (typeof value === 'number') {
      result[index] = value & 0xff;
      assigned += 1;
    } else if (typeof value === 'string') {
      const parsed = Number(value);
      if (Number.isFinite(parsed)) {
        result[index] = parsed & 0xff;
        assigned += 1;
      }
    }
  }

  if (assigned === 0) {
    return null;
  }

  return result;
}

async function ensureUint8Array(input: unknown, encoding?: string | null): Promise<Uint8Array> {
  if (encoding === 'uint8-array' && Array.isArray(input)) {
    return Uint8Array.from(input as Array<number>);
  }

  if (input instanceof Uint8Array) {
    return input;
  }

  if (ArrayBuffer.isView(input)) {
    const view = input as ArrayBufferView;
    return new Uint8Array(view.buffer.slice(view.byteOffset, view.byteOffset + view.byteLength));
  }

  if (input instanceof ArrayBuffer) {
    return new Uint8Array(input);
  }

  if (input instanceof Blob) {
    const buffer = await input.arrayBuffer();
    return new Uint8Array(buffer);
  }

  if (Array.isArray(input)) {
    return Uint8Array.from(input);
  }

  if (input && typeof input === 'object') {
    const asRecord = input as Record<string, unknown>;

    if (Array.isArray(asRecord.data) && (asRecord.type === 'Buffer' || typeof asRecord.type === 'string')) {
      return Uint8Array.from(asRecord.data as number[]);
    }

    const reconstructed = buildUint8ArrayFromNumericObject(asRecord);
    if (reconstructed) {
      console.warn('[chromeOffscreen]: reconstructed array-like payload into Uint8Array', {
        length: reconstructed.length,
      });
      return reconstructed;
    }
  }

  throw new Error('Unsupported input data for conversion');
}

async function convertFile(data: unknown, encoding: string | undefined, config: FormatOptions | undefined, formatId?: string) {
  const Module = await ensureModule();
  const format = resolveFormat(formatId);
  const normalized = normalizeFormatOptions(format, config);
  const inputBytes = await ensureUint8Array(data, encoding ?? null);
  const startTime = performance.now();
  const output = await format.convert({ Module, inputBytes, options: normalized });
  const endTime = performance.now();
  const conversionTimeMs = Math.round(endTime - startTime);
  return { output, format, conversionTimeMs };
}

chrome.runtime.onMessage.addListener((message: any, _sender, sendResponse) => {
  if (message?.type === 'OFFSCREEN_PING' && message?.target === 'offscreen') {
    sendResponse({ success: true });
    return true;
  }

  if (message?.type === 'PROCESS_FILE' && message?.target === 'offscreen') {
    (async () => {
      try {
        const fileData = message.fileData as { data?: unknown; dataEncoding?: string; name?: string } | undefined;
        const inputData = fileData?.data;
        if (!inputData) {
          throw new Error('No input data provided');
        }
        const logDetails: Record<string, unknown> = {
          name: fileData?.name,
          ctor: (inputData as { constructor?: { name?: string } } | null | undefined)?.constructor?.name,
          encoding: fileData?.dataEncoding,
        };
        if (inputData instanceof ArrayBuffer) {
          logDetails.byteLength = inputData.byteLength;
        } else if (ArrayBuffer.isView(inputData)) {
          logDetails.byteLength = (inputData as ArrayBufferView).byteLength;
          logDetails.viewConstructor = (inputData as ArrayBufferView).constructor?.name;
        } else if (typeof inputData === 'object' && inputData !== null) {
          const keys = Object.keys(inputData as Record<string, unknown>);
          logDetails.keysSample = keys.slice(0, 5);
          const length = (inputData as { length?: unknown }).length;
          if (typeof length === 'number') {
            logDetails.length = length;
          }
        }
        const { output, format, conversionTimeMs } = await convertFile(inputData, fileData?.dataEncoding, message.config as FormatOptions | undefined, message.formatId);
        const payload = Array.from(output);
        sendResponse({
          success: true,
          outputData: payload,
          outputEncoding: 'uint8-array',
          formatId: format.id,
          conversionTimeMs,
        });
      } catch (error) {
        const failure = error as Error & {
          code?: string;
          inputSize?: number;
          outputSize?: number;
          formatLabel?: string;
        };
        sendResponse({
          success: false,
          error: failure?.message ?? 'Unknown error',
          errorCode: failure?.code,
          errorDetails: failure
            ? {
                inputSize: failure.inputSize,
                outputSize: failure.outputSize,
                formatLabel: failure.formatLabel,
              }
            : undefined,
          formatId: message.formatId ?? 'unknown',
          errorStack: failure?.stack,
        });
      }
    })();
    return true;
  }

  if (message?.type === 'GET_VERSION_INFO' && message?.target === 'offscreen') {
    (async () => {
      try {
        const Module = await ensureModule();
        const info = getVersionInfo(Module);
        const threadsEnabled = isThreadsEnabled(Module);
        const pngxOxipng = pngxBridgeInitialized ? pngxOxipngVersion() : undefined;
        const pngxLibiq = pngxBridgeInitialized ? pngxLibimagequantVersion() : undefined;
        let rustVersion = info.rustVersionString;
        if (pngxBridgeInitialized && (!rustVersion || rustVersion === 'unknown')) {
          try {
            rustVersion = pngxRustVersionString();
          } catch {
            /* NOP */
          }
        }

        sendResponse({
          success: true,
          ...info,
          pngxOxipngVersion: pngxOxipng,
          pngxLibimagequantVersion: pngxLibiq,
          rustVersionString: rustVersion,
          isThreadsEnabled: threadsEnabled,
        });
      } catch (error) {
        sendResponse({ success: false, error: (error as Error).message ?? 'Failed to get version info' });
      }
    })();
    return true;
  }

  return false;
});

self.addEventListener('beforeunload', () => {
  moduleInstance = null;
});

void ensureModule();
