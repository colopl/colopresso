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

import React, { createContext, useContext, ReactNode } from 'react';
import { PlatformAdapter, PlatformContextValue } from '../types/platform';

const PlatformContext = createContext<PlatformContextValue | null>(null);

interface PlatformProviderProps {
  adapter: PlatformAdapter;
  children: ReactNode;
}

export const PlatformProvider: React.FC<PlatformProviderProps> = ({ adapter, children }) => {
  return <PlatformContext.Provider value={{ adapter }}>{children}</PlatformContext.Provider>;
};

export const usePlatform = (): PlatformAdapter => {
  const context = useContext(PlatformContext);
  if (!context) {
    throw new Error('usePlatform must be used within a PlatformProvider');
  }
  return context.adapter;
};

export default PlatformContext;
