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

import { useCallback } from 'react';
import { StatusMessage } from '../types';

export function useClipboard(applyStatus: (status: StatusMessage | null) => void) {
  const copyViaFallback = useCallback((message: string) => {
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
  }, []);

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

      try {
        if (typeof navigator !== 'undefined' && navigator.clipboard && navigator.clipboard.writeText) {
          await navigator.clipboard.writeText(message);
        } else {
          copyViaFallback(message);
        }

        applyStatus({ type: 'success', messageKey: 'errors.copySuccess', durationMs: 2000 });
        scheduleRestore(2000);
      } catch (_copyError) {
        applyStatus({ type: 'error', messageKey: 'errors.copyFailed', durationMs: 4000 });
        scheduleRestore(4000);
      }
    },
    [applyStatus, copyViaFallback]
  );

  return { copyTextToClipboard };
}
