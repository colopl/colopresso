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

import React, { createContext, useContext, useMemo, useReducer } from 'react';
import { BuildInfoState, FileEntry, ProgressState, SettingsState, StatusMessage } from '../types';
import { Theme } from '../core/theme';

interface AppState {
  activeFormatId: string;
  settings: SettingsState;
  fileEntries: FileEntry[];
  progress: ProgressState;
  status: StatusMessage | null;
  isProcessing: boolean;
  theme: Theme;
  buildInfo: BuildInfoState;
  advancedSettingsVisible: boolean;
}

const defaultSettings: SettingsState = {
  useOriginalOutput: true,
};

const initialState: AppState = {
  activeFormatId: 'webp',
  settings: defaultSettings,
  fileEntries: [],
  progress: { current: 0, total: 0, state: 'idle' },
  status: null,
  isProcessing: false,
  theme: 'light',
  buildInfo: { status: 'idle' },
  advancedSettingsVisible: false,
};

type AppAction =
  | { type: 'setActiveFormat'; id: string }
  | { type: 'setSettings'; settings: SettingsState }
  | { type: 'mergeSettings'; settings: Partial<SettingsState> }
  | { type: 'setFileEntries'; entries: FileEntry[] }
  | { type: 'patchFileEntry'; id: string; patch: Partial<FileEntry> }
  | { type: 'setProgress'; progress: ProgressState }
  | { type: 'setStatus'; status: StatusMessage | null }
  | { type: 'setProcessing'; processing: boolean }
  | { type: 'setTheme'; theme: Theme }
  | { type: 'setBuildInfo'; buildInfo: BuildInfoState }
  | { type: 'setAdvancedSettingsVisible'; visible: boolean };

function reducer(state: AppState, action: AppAction): AppState {
  switch (action.type) {
    case 'setActiveFormat':
      return { ...state, activeFormatId: action.id };
    case 'setSettings':
      return { ...state, settings: { ...action.settings } };
    case 'mergeSettings':
      return { ...state, settings: { ...state.settings, ...action.settings } };
    case 'setFileEntries':
      return { ...state, fileEntries: action.entries };
    case 'patchFileEntry':
      return {
        ...state,
        fileEntries: state.fileEntries.map((entry) => (entry.id === action.id ? { ...entry, ...action.patch } : entry)),
      };
    case 'setProgress':
      return { ...state, progress: action.progress };
    case 'setStatus':
      return { ...state, status: action.status };
    case 'setProcessing':
      return { ...state, isProcessing: action.processing };
    case 'setTheme':
      return { ...state, theme: action.theme };
    case 'setBuildInfo':
      return { ...state, buildInfo: action.buildInfo };
    case 'setAdvancedSettingsVisible':
      return { ...state, advancedSettingsVisible: action.visible };
    default:
      return state;
  }
}

const StateContext = createContext<AppState | undefined>(undefined);
const DispatchContext = createContext<React.Dispatch<AppAction> | undefined>(undefined);

export const AppStateProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [state, dispatch] = useReducer(reducer, initialState);
  const stateValue = useMemo(() => state, [state]);
  return (
    <StateContext.Provider value={stateValue}>
      <DispatchContext.Provider value={dispatch}>{children}</DispatchContext.Provider>
    </StateContext.Provider>
  );
};

export function useAppState(): AppState {
  const state = useContext(StateContext);
  if (!state) {
    throw new Error('useAppState must be used within AppStateProvider');
  }

  return state;
}

export function useAppDispatch(): React.Dispatch<AppAction> {
  const dispatch = useContext(DispatchContext);
  if (!dispatch) {
    throw new Error('useAppDispatch must be used within AppStateProvider');
  }

  return dispatch;
}
