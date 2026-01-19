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

function serializeBinaryPayload(data: unknown, existingEncoding?: string): { payload: unknown; encoding?: string; changed: boolean } {
  if (Array.isArray(data)) {
    return { payload: data, encoding: existingEncoding, changed: false };
  }

  if (data instanceof ArrayBuffer) {
    const array = Array.from(new Uint8Array(data));
    return { payload: array, encoding: 'uint8-array', changed: true };
  }

  if (ArrayBuffer.isView(data)) {
    const view = data as ArrayBufferView;
    const array = Array.from(new Uint8Array(view.buffer, view.byteOffset, view.byteLength));
    return { payload: array, encoding: 'uint8-array', changed: true };
  }

  if (data instanceof Uint8Array) {
    return { payload: Array.from(data), encoding: 'uint8-array', changed: true };
  }

  return { payload: data, encoding: undefined, changed: false };
}
let offscreenCreationPromise: Promise<void> | null = null;

const FORWARD_MARKER = '__colopressoForwarded';

function sendMessageToOffscreen(message: Record<string, unknown>): Promise<unknown> {
  const payload: Record<string, unknown> = { ...message, [FORWARD_MARKER]: true };
  return new Promise((resolve, reject) => {
    try {
      chrome.runtime.sendMessage(payload, (response) => {
        if (chrome.runtime.lastError) {
          reject(new Error(chrome.runtime.lastError.message));
          return;
        }
        resolve(response);
      });
    } catch (error) {
      reject(error as Error);
    }
  });
}

async function waitForOffscreenReady(maxAttempts = 5): Promise<void> {
  let attempt = 0;
  let lastError: Error | null = null;

  while (attempt < maxAttempts) {
    try {
      const response = (await sendMessageToOffscreen({ type: 'OFFSCREEN_PING', target: 'offscreen' })) as { success?: boolean } | undefined;
      if (!response || response.success !== false) {
        return;
      }
      lastError = new Error('Offscreen ping reported failure');
    } catch (error) {
      lastError = error as Error;
    }

    await new Promise((resolve) => setTimeout(resolve, 100 * (attempt + 1)));
    attempt += 1;
  }

  throw lastError ?? new Error('Offscreen document did not respond to ping');
}

async function ensureOffscreenDocument(): Promise<void> {
  if (offscreenCreationPromise) {
    return offscreenCreationPromise;
  }

  offscreenCreationPromise = (async () => {
    const contexts =
      (await (chrome.runtime as unknown as { getContexts?: (options: unknown) => Promise<chrome.runtime.ExtensionContext[]> }).getContexts?.({
        contextTypes: ['OFFSCREEN_DOCUMENT'],
      })) ?? [];

    if (contexts.length === 0) {
      try {
        await chrome.offscreen.createDocument({
          url: 'offscreen.html',
          reasons: ['WORKERS' as chrome.offscreen.Reason],
          justification: 'PNG to Advanced Format conversion using WASM',
        } as chrome.offscreen.CreateParameters);
      } catch (error) {
        if (!(error as Error | undefined)?.message?.includes('Only a single offscreen document')) {
          throw error;
        }
      }
    }

    await waitForOffscreenReady();
  })().finally(() => {
    offscreenCreationPromise = null;
  });

  return offscreenCreationPromise;
}

chrome.action.onClicked.addListener((tab) => {
  chrome.sidePanel.open({ windowId: tab.windowId });
});

async function closeOffscreenDocument(): Promise<void> {
  try {
    const contexts =
      (await (chrome.runtime as unknown as { getContexts?: (options: unknown) => Promise<chrome.runtime.ExtensionContext[]> }).getContexts?.({
        contextTypes: ['OFFSCREEN_DOCUMENT'],
      })) ?? [];

    if (contexts.length > 0) {
      await chrome.offscreen.closeDocument();
    }
  } catch {
    /* NOP */
  }
  offscreenCreationPromise = null;
}

chrome.runtime.onMessage.addListener((message, _sender, sendResponse) => {
  if (message && typeof message === 'object' && FORWARD_MARKER in message) {
    return false;
  }

  if (message?.type === 'ENSURE_OFFSCREEN' && message?.target === 'background') {
    ensureOffscreenDocument()
      .then(() => sendResponse({ success: true }))
      .catch((error: Error) => sendResponse({ success: false, error: error.message }));
    return true;
  }

  if (message?.type === 'CANCEL_PROCESSING' && message?.target === 'background') {
    closeOffscreenDocument()
      .then(() => sendResponse({ success: true }))
      .catch((error: Error) => sendResponse({ success: false, error: error.message }));
    return true;
  }

  if (message?.target === 'offscreen') {
    let forwardPayload = message as Record<string, unknown>;
    const fileData = (message as { fileData?: { data?: unknown; [key: string]: unknown } }).fileData;
    if (fileData && typeof fileData === 'object') {
      const existingEncoding = typeof fileData.dataEncoding === 'string' ? fileData.dataEncoding : undefined;
      const { payload, encoding, changed } = serializeBinaryPayload(fileData.data, existingEncoding);
      const nextEncoding = encoding ?? existingEncoding;
      if (changed || encoding) {
        forwardPayload = {
          ...message,
          fileData: {
            ...fileData,
            data: payload,
            dataEncoding: nextEncoding,
          },
        } as Record<string, unknown>;
      }
    }
    ensureOffscreenDocument()
      .then(() => sendMessageToOffscreen(forwardPayload))
      .then((response) => sendResponse(response))
      .catch((error: Error) => sendResponse({ success: false, error: error.message }));
    return true;
  }
  return false;
});
