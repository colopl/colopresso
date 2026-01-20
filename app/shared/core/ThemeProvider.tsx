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

import React, { createContext, useCallback, useContext, useEffect, useMemo } from 'react';
import { useAppDispatch, useAppState } from '../state/appState';
import { Theme, applyThemeToDocument, loadTheme, saveTheme } from './theme';

interface ThemeContextValue {
  theme: Theme;
  setTheme: (theme: Theme, options?: { persist?: boolean }) => Promise<void>;
}

const ThemeContext = createContext<ThemeContextValue | undefined>(undefined);

const ThemeProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const state = useAppState();
  const dispatch = useAppDispatch();

  useEffect(() => {
    let cancelled = false;
    (async () => {
      try {
        const storedTheme = await loadTheme();
        if (!cancelled) {
          dispatch({ type: 'setTheme', theme: storedTheme });
        }
      } catch (error) {
        console.warn('Failed to load stored theme', error);
      }
    })();
    return () => {
      cancelled = true;
    };
  }, [dispatch]);

  useEffect(() => {
    applyThemeToDocument(state.theme);
  }, [state.theme]);

  const setTheme = useCallback(
    async (nextTheme: Theme, options?: { persist?: boolean }) => {
      dispatch({ type: 'setTheme', theme: nextTheme });

      if (options?.persist === false) {
        return;
      }

      await saveTheme(nextTheme);
    },
    [dispatch]
  );

  const value = useMemo<ThemeContextValue>(() => ({ theme: state.theme, setTheme }), [setTheme, state.theme]);

  return <ThemeContext.Provider value={value}>{children}</ThemeContext.Provider>;
};

export default ThemeProvider;

export function useTheme(): ThemeContextValue {
  const context = useContext(ThemeContext);
  if (!context) {
    throw new Error('useTheme must be used within ThemeProvider');
  }

  return context;
}
