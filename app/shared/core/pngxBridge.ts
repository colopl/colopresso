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

export interface PngxBridgeLosslessOptions {
  optimizationLevel?: number;
  stripSafe?: boolean;
  optimizeAlpha?: boolean;
}

export interface PngxBridgeQuantParams {
  speed?: number;
  qualityMin?: number;
  qualityMax?: number;
  maxColors?: number;
  minPosterization?: number;
  ditheringLevel?: number;
  remap?: boolean;
  importanceMap?: Uint8Array;
  fixedColors?: Uint8Array;
}

export interface PngxBridgeQuantResult {
  palette: Uint8Array;
  indices: Uint8Array;
  quality: number;
  status: number;
}

export const PNGX_QUANT_STATUS_OK = 0;
export const PNGX_QUANT_STATUS_QUALITY_TOO_LOW = 1;
export const PNGX_QUANT_STATUS_ERROR = 2;

interface WasmLosslessOptions {
  free(): void;
  set strip_safe(value: boolean);
  set optimize_alpha(value: boolean);
  set optimization_level(value: number);
}

interface WasmQuantParams {
  free(): void;
  set max_colors(value: number);
  set quality_max(value: number);
  set quality_min(value: number);
  set dithering_level(value: number);
  set min_posterization(value: number);
  set remap(value: boolean);
  set speed(value: number);
}

interface WasmQuantResult {
  free(): void;
  readonly status: number;
  readonly indices: Uint8Array;
  readonly palette: Uint8Array;
  readonly quality: number;
}

interface WasmExports {
  memory: WebAssembly.Memory;
  pngx_bridge_oxipng_version(): number;
  pngx_bridge_libimagequant_version(): number;
  pngx_bridge_init_threads(num_threads: number): number;
}

interface PngxBridgeWasmModule {
  WasmLosslessOptions: new () => WasmLosslessOptions;
  WasmQuantParams: new () => WasmQuantParams;
  pngx_wasm_libimagequant_version(): number;
  pngx_wasm_optimize_lossless(input_data: Uint8Array, options?: WasmLosslessOptions | null): Uint8Array;
  pngx_wasm_last_oxipng_error?(): string;
  pngx_wasm_oxipng_version(): number;
  pngx_wasm_rust_version_string(): string;
  pngx_wasm_quantize(pixels: Uint8Array, width: number, height: number, params?: WasmQuantParams | null): WasmQuantResult;
  pngx_wasm_quantize_advanced(
    pixels: Uint8Array,
    width: number,
    height: number,
    speed: number,
    quality_min: number,
    quality_max: number,
    max_colors: number,
    min_posterization: number,
    dithering_level: number,
    remap: boolean,
    importance_map?: Uint8Array | null,
    fixed_colors?: Uint8Array | null
  ): WasmQuantResult;
  initThreadPool?(numThreads: number): Promise<void>;
  pngx_wasm_is_threads_enabled?(): boolean;
}

let wasmModule: PngxBridgeWasmModule | null = null;
let wasmExports: WasmExports | null = null;
let initPromise: Promise<PngxBridgeWasmModule> | null = null;
let moduleBaseUrl = '';
let threadingEnabled = false;

type PngxBridgeWasmModuleWithInit = PngxBridgeWasmModule & {
  default: (input?: { module_or_path?: RequestInfo | URL | ArrayBuffer | Uint8Array } | RequestInfo | URL | ArrayBuffer | Uint8Array) => Promise<WasmExports>;
  initSync: (module: { module: BufferSource } | BufferSource) => WasmExports;
};

async function loadPngxBridgeModuleFromSource(jsSource: string): Promise<PngxBridgeWasmModuleWithInit> {
  const blob = new Blob([jsSource], { type: 'application/javascript' });
  const blobUrl = URL.createObjectURL(blob);
  const module = await import(/* @vite-ignore */ blobUrl);

  return module as PngxBridgeWasmModuleWithInit;
}

function createWorkerScript(pngxBridgeBlobUrl: string, wasmUrl: string): string {
  return `
    function waitForMsgType(target, type) {
      return new Promise(resolve => {
        target.addEventListener('message', function onMsg({ data }) {
          if (data?.type !== type) return;
          target.removeEventListener('message', onMsg);
          resolve(data);
        });
      });
    }

    waitForMsgType(self, 'wasm_bindgen_worker_init').then(async ({ init, receiver }) => {
      const pkg = await import(${JSON.stringify(pngxBridgeBlobUrl)});
      await pkg.default(init);
      postMessage({ type: 'wasm_bindgen_worker_ready' });
      pkg.wbg_rayon_start_worker(receiver);
    });
  `;
}

function createStartWorkersCode(workerBlobUrl: string): string {
  return `
let _workers;

export async function startWorkers(module, memory, builder) {
  if (builder.numThreads() === 0) {
    throw new Error('num_threads must be > 0.');
  }

  const workerInit = {
    type: 'wasm_bindgen_worker_init',
    init: { module_or_path: module, memory },
    receiver: builder.receiver()
  };

  function waitForMsgType(target, type) {
    return new Promise(resolve => {
      target.addEventListener('message', function onMsg({ data }) {
        if (data?.type !== type) return;
        target.removeEventListener('message', onMsg);
        resolve(data);
      });
    });
  }

  _workers = await Promise.all(
    Array.from({ length: builder.numThreads() }, async () => {
      const worker = new Worker(${JSON.stringify(workerBlobUrl)}, { type: 'module' });
      worker.postMessage(workerInit);
      await waitForMsgType(worker, 'wasm_bindgen_worker_ready');
      return worker;
    })
  );
  builder.build();
}
`;
}

async function loadPngxBridgeModule(jsUrl: string, wasmUrl: string): Promise<PngxBridgeWasmModuleWithInit> {
  const isElectronProtocol = jsUrl.startsWith('colopresso://') || jsUrl.startsWith('file://');

  if (!isElectronProtocol) {
    try {
      const module = await import(/* @vite-ignore */ jsUrl);
      return module as PngxBridgeWasmModuleWithInit;
    } catch {
      /* NOP */
    }
  }
  const response = await fetch(jsUrl);
  if (!response.ok) {
    throw new Error(`Failed to fetch pngx_bridge.js: ${response.status} ${response.statusText}`);
  }

  let jsSource = await response.text();

  jsSource = jsSource.replace(/new URL\(['"]pngx_bridge_bg\.wasm['"],\s*import\.meta\.url\)/g, JSON.stringify(wasmUrl));

  jsSource = jsSource.replace(/import\s*\{\s*startWorkers\s*\}\s*from\s*['"]\.\/snippets\/[^'"]+['"];?\s*/g, '');

  const pngxBridgeBlob = new Blob([jsSource], { type: 'application/javascript' });
  const pngxBridgeBlobUrl = URL.createObjectURL(pngxBridgeBlob);

  const workerScript = createWorkerScript(pngxBridgeBlobUrl, wasmUrl);
  const workerBlob = new Blob([workerScript], { type: 'application/javascript' });
  const workerBlobUrl = URL.createObjectURL(workerBlob);

  const startWorkersCode = createStartWorkersCode(workerBlobUrl);
  jsSource = jsSource + '\n' + startWorkersCode;

  return loadPngxBridgeModuleFromSource(jsSource);
}

export async function initPngxBridge(baseUrl: string, numThreads?: number): Promise<void> {
  if (wasmModule) {
    return;
  }

  if (initPromise) {
    await initPromise;
    return;
  }

  moduleBaseUrl = baseUrl.endsWith('/') ? baseUrl : baseUrl + '/';

  initPromise = (async () => {
    const jsUrl = moduleBaseUrl + 'pngx_bridge.js';
    const wasmUrl = moduleBaseUrl + 'pngx_bridge_bg.wasm';
    const module = await loadPngxBridgeModule(jsUrl, wasmUrl);
    const isNode = typeof process !== 'undefined' && typeof (process as unknown as { versions?: { node?: string } }).versions?.node === 'string';
    let exports: WasmExports;
    if (isNode && wasmUrl.startsWith('file:')) {
      const fs = await import('node:fs/promises');
      const wasmBytes = await fs.readFile(new URL(wasmUrl));
      const view = new Uint8Array(wasmBytes.buffer, wasmBytes.byteOffset, wasmBytes.byteLength);
      exports = await module.default({ module_or_path: view });
    } else {
      exports = await module.default({ module_or_path: wasmUrl });
    }
    wasmExports = exports;

    if (module.initThreadPool && module.pngx_wasm_is_threads_enabled?.()) {
      const hardwareConcurrency = typeof navigator !== 'undefined' ? navigator.hardwareConcurrency : 1;
      const threads = numThreads && numThreads > 0 ? numThreads : hardwareConcurrency;

      try {
        await module.initThreadPool(threads);
        threadingEnabled = true;
      } catch (error) {
        console.warn('[pngxBridge] Failed to initialize thread pool, falling back to single-threaded mode:', error);
        threadingEnabled = false;
      }
    } else {
      threadingEnabled = false;
    }

    wasmModule = module;

    return module;
  })();

  await initPromise;
}

export function isPngxBridgeInitialized(): boolean {
  return wasmModule !== null;
}

export function isPngxBridgeThreadingEnabled(): boolean {
  return threadingEnabled;
}

function getModule(): PngxBridgeWasmModule {
  if (!wasmModule) {
    throw new Error('pngx_bridge not initialized. Call initPngxBridge first.');
  }

  return wasmModule;
}

export function pngxOptimizeLossless(inputData: Uint8Array, options?: PngxBridgeLosslessOptions): Uint8Array {
  const module = getModule();
  const wasmOptions = new module.WasmLosslessOptions();

  try {
    if (options?.optimizationLevel !== undefined) {
      wasmOptions.optimization_level = options.optimizationLevel;
    }
    if (options?.stripSafe !== undefined) {
      wasmOptions.strip_safe = options.stripSafe;
    }
    if (options?.optimizeAlpha !== undefined) {
      wasmOptions.optimize_alpha = options.optimizeAlpha;
    }

    const result = module.pngx_wasm_optimize_lossless(inputData, wasmOptions);
    const out = new Uint8Array(result);

    const message = module.pngx_wasm_last_oxipng_error?.() ?? '';
    if (message) {
      console.warn('[pngxOptimizeLossless] oxipng error:', message);
    }

    return out;
  } finally {
    wasmOptions.free();
  }
}

export function pngxQuantize(pixels: Uint8Array, width: number, height: number, params?: PngxBridgeQuantParams): PngxBridgeQuantResult {
  const module = getModule();

  if (params?.importanceMap || params?.fixedColors) {
    let result;

    try {
      result = module.pngx_wasm_quantize_advanced(
        pixels,
        width,
        height,
        params?.speed ?? 3,
        params?.qualityMin ?? 0,
        params?.qualityMax ?? 100,
        params?.maxColors ?? 256,
        params?.minPosterization ?? 0,
        params?.ditheringLevel ?? 1.0,
        params?.remap ?? true,
        params?.importanceMap ?? null,
        params?.fixedColors ?? null
      );
    } catch (e) {
      console.error('[pngxQuantize] pngx_wasm_quantize_advanced threw:', e);
      throw e;
    }

    try {
      return {
        palette: new Uint8Array(result.palette),
        indices: new Uint8Array(result.indices),
        quality: result.quality,
        status: result.status,
      };
    } finally {
      result.free();
    }
  }

  const wasmParams = new module.WasmQuantParams();

  try {
    if (params?.speed !== undefined) wasmParams.speed = params.speed;
    if (params?.qualityMin !== undefined) wasmParams.quality_min = params.qualityMin;
    if (params?.qualityMax !== undefined) wasmParams.quality_max = params.qualityMax;
    if (params?.maxColors !== undefined) wasmParams.max_colors = params.maxColors;
    if (params?.minPosterization !== undefined) wasmParams.min_posterization = params.minPosterization;
    if (params?.ditheringLevel !== undefined) wasmParams.dithering_level = params.ditheringLevel;
    if (params?.remap !== undefined) wasmParams.remap = params.remap;

    const result = module.pngx_wasm_quantize(pixels, width, height, wasmParams);

    try {
      return {
        palette: new Uint8Array(result.palette),
        indices: new Uint8Array(result.indices),
        quality: result.quality,
        status: result.status,
      };
    } finally {
      result.free();
    }
  } finally {
    wasmParams.free();
  }
}

function getWasmExports(): WasmExports {
  if (!wasmExports) {
    throw new Error('pngx_bridge not initialized. Call initPngxBridge first.');
  }
  return wasmExports;
}

export function pngxOxipngVersion(): number {
  return getWasmExports().pngx_bridge_oxipng_version();
}

export function pngxLibimagequantVersion(): number {
  return getWasmExports().pngx_bridge_libimagequant_version();
}

export function pngxRustVersionString(): string {
  const module = getModule();
  return module.pngx_wasm_rust_version_string();
}
