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

import React from 'react';
import { createRoot } from 'react-dom/client';
import ElectronApp from '../app/ElectronApp';

function mountApp(): void {
  const container = document.getElementById('root');
  if (!container) {
    console.error('[React] Root container not found');
    return;
  }

  const root = createRoot(container);
  root.render(<ElectronApp />);
}

document.addEventListener('DOMContentLoaded', mountApp);
