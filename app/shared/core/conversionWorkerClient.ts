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

import type { FormatOptions } from '../types';
import type { ConversionWorkerRequest, ConversionWorkerResponse } from './conversionWorker';

export interface ConversionWorkerInitResult {
  threadsEnabled: boolean;
  pngxBridgeEnabled: boolean;
  version?: number;
  libwebpVersion?: number;
  libpngVersion?: number;
  libavifVersion?: number;
  pngxOxipngVersion?: number;
  pngxLibimagequantVersion?: number;
  buildtime?: number;
}

export interface ConversionWorkerClient {
  init(): Promise<ConversionWorkerInitResult>;
  convert(formatId: string, options: FormatOptions, inputBytes: Uint8Array): Promise<{ outputBytes: Uint8Array; inputSize: number; outputSize: number }>;
  terminate(): void;
}

class ConversionWorkerError extends Error {
  public readonly code?: string;
  public readonly inputSize?: number;
  public readonly outputSize?: number;

  constructor(message: string, options?: { code?: string; inputSize?: number; outputSize?: number }) {
    super(message);
    this.name = 'ConversionWorkerError';
    this.code = options?.code;
    this.inputSize = options?.inputSize;
    this.outputSize = options?.outputSize;
    Object.setPrototypeOf(this, new.target.prototype);
  }
}

export interface PngxBridgeFiles {
  jsSource: string;
  wasmBytes: ArrayBuffer;
}

export interface ConversionWorkerClientOptions {
  pngxBridgeUrl?: string;
  pngxBridgeLoader?: (basePath: string) => Promise<PngxBridgeFiles>;
}

export function createConversionWorkerClient(workerUrl: URL, moduleUrl: string, options?: ConversionWorkerClientOptions): ConversionWorkerClient {
  const worker = new Worker(workerUrl, { type: 'module' });
  let requestIdCounter = 0;
  const pendingRequests = new Map<number, { resolve: (response: ConversionWorkerResponse) => void; reject: (error: Error) => void }>();

  let pngxBridgeJsSource: string | undefined;
  let pngxBridgeWasmBytes: ArrayBuffer | undefined;

  worker.onmessage = (event: MessageEvent<ConversionWorkerResponse>) => {
    const response = event.data;
    const pending = pendingRequests.get(response.id);

    if (pending) {
      pendingRequests.delete(response.id);
      pending.resolve(response);
    }
  };

  worker.onerror = (event: ErrorEvent) => {
    console.error('[ConversionWorkerClient] worker error:', event);
    for (const [id, pending] of pendingRequests) {
      pending.reject(new Error(event.message || 'Worker error'));
      pendingRequests.delete(id);
    }
  };

  const sendRequest = (request: Omit<ConversionWorkerRequest, 'id'>): Promise<ConversionWorkerResponse> => {
    return new Promise((resolve, reject) => {
      const id = ++requestIdCounter;
      const fullRequest: ConversionWorkerRequest = { ...request, id };
      const transferables: Transferable[] = [];

      pendingRequests.set(id, { resolve, reject });

      if (fullRequest.inputBytes) {
        transferables.push(fullRequest.inputBytes);
      }

      worker.postMessage(fullRequest, transferables);
    });
  };

  const preFetchPngxBridge = async (baseUrl: string): Promise<{ jsSource: string; wasmBytes: ArrayBuffer }> => {
    const normalizedUrl = baseUrl.endsWith('/') ? baseUrl : baseUrl + '/';
    const jsUrl = normalizedUrl + 'pngx_bridge.js';
    const wasmUrl = normalizedUrl + 'pngx_bridge_bg.wasm';

    const [jsResponse, wasmResponse] = await Promise.all([fetch(jsUrl), fetch(wasmUrl)]);

    if (!jsResponse.ok) {
      throw new Error(`Failed to fetch pngx_bridge.js: ${jsResponse.status} ${jsResponse.statusText}`);
    }
    if (!wasmResponse.ok) {
      throw new Error(`Failed to fetch pngx_bridge_bg.wasm: ${wasmResponse.status} ${wasmResponse.statusText}`);
    }

    const jsSource = await jsResponse.text();
    const wasmBytes = await wasmResponse.arrayBuffer();

    return { jsSource, wasmBytes };
  };

  return {
    async init() {
      if (options?.pngxBridgeUrl && !pngxBridgeJsSource) {
        try {
          if (options.pngxBridgeLoader) {
            const url = new URL(options.pngxBridgeUrl);
            const basePath = decodeURIComponent(url.pathname);
            const { jsSource, wasmBytes } = await options.pngxBridgeLoader(basePath);

            pngxBridgeJsSource = jsSource;
            pngxBridgeWasmBytes = wasmBytes;
          } else {
            const { jsSource, wasmBytes } = await preFetchPngxBridge(options.pngxBridgeUrl);

            pngxBridgeJsSource = jsSource;
            pngxBridgeWasmBytes = wasmBytes;
          }
        } catch (error) {
          console.warn('[ConversionWorkerClient] Failed to pre-fetch pngx_bridge:', error);
        }
      }

      const response = await sendRequest({
        type: 'init',
        moduleUrl,
        pngxBridgeUrl: options?.pngxBridgeUrl,
        pngxBridgeJsSource,
        pngxBridgeWasmBytes,
      });
      if (!response.success) {
        throw new ConversionWorkerError(response.error ?? 'Init failed');
      }

      return {
        threadsEnabled: response.threadsEnabled ?? false,
        pngxBridgeEnabled: response.pngxBridgeEnabled ?? false,
        version: response.version,
        libwebpVersion: response.libwebpVersion,
        libpngVersion: response.libpngVersion,
        libavifVersion: response.libavifVersion,
        pngxOxipngVersion: response.pngxOxipngVersion,
        pngxLibimagequantVersion: response.pngxLibimagequantVersion,
        buildtime: response.buildtime,
      };
    },

    async convert(formatId: string, options: FormatOptions, inputBytes: Uint8Array) {
      const inputBuffer = inputBytes.buffer.slice(inputBytes.byteOffset, inputBytes.byteOffset + inputBytes.byteLength) as ArrayBuffer;
      const response = await sendRequest({
        type: 'convert',
        formatId,
        options,
        inputBytes: inputBuffer,
      });

      if (!response.success || !response.outputBytes) {
        throw new ConversionWorkerError(response.error ?? 'Conversion failed', {
          code: response.errorCode,
          inputSize: response.inputSize,
          outputSize: response.outputSize,
        });
      }

      return {
        outputBytes: new Uint8Array(response.outputBytes),
        inputSize: response.inputSize ?? inputBytes.length,
        outputSize: response.outputSize ?? response.outputBytes.byteLength,
      };
    },

    terminate() {
      worker.terminate();
      for (const [id, pending] of pendingRequests) {
        pending.reject(new Error('Worker terminated'));
        pendingRequests.delete(id);
      }
    },
  };
}
