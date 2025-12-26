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
import { useAppDispatch } from '../state/appState';

export function useProcessingState() {
  const dispatch = useAppDispatch();

  const resetProcessingState = useCallback(
    (total: number = 0) => {
      dispatch({ type: 'setProcessing', processing: true });
      dispatch({
        type: 'setProgress',
        progress: { current: 0, total, state: total === 0 ? 'idle' : 'processing' },
      });
    },
    [dispatch]
  );

  const updateProgress = useCallback(
    (current: number, total: number, done = false) => {
      dispatch({
        type: 'setProgress',
        progress: { current, total, state: done ? 'done' : 'processing' },
      });
    },
    [dispatch]
  );

  const finalizeProcessingState = useCallback(() => {
    dispatch({ type: 'setProcessing', processing: false });
  }, [dispatch]);

  return { resetProcessingState, updateProgress, finalizeProcessingState };
}
