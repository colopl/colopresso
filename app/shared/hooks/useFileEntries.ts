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

import { useCallback } from 'react';
import { useAppDispatch } from '../state/appState';
import { FileEntry } from '../types';

export function useFileEntries() {
  const dispatch = useAppDispatch();

  const registerFileEntries = useCallback(
    (names: string[]): FileEntry[] => {
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

  const clearFileEntries = useCallback(() => {
    dispatch({ type: 'setFileEntries', entries: [] });
  }, [dispatch]);

  return { registerFileEntries, patchFileEntry, clearFileEntries };
}
