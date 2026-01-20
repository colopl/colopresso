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

import { useCallback, useRef } from 'react';
import { useAppDispatch } from '../state/appState';
import { StatusMessage } from '../types';

export interface UseStatusMessageResult {
  applyStatus: (status: StatusMessage | null) => void;
  applyStatusRef: React.MutableRefObject<(status: StatusMessage | null) => void>;
}

export function useStatusMessage(): UseStatusMessageResult {
  const dispatch = useAppDispatch();
  const statusTimerRef = useRef<number | null>(null);

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
  applyStatusRef.current = applyStatus;

  return { applyStatus, applyStatusRef };
}
