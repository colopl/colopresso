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

import path from 'node:path';
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

const sharedRoot = path.resolve(__dirname);
const chromeRoot = path.resolve(__dirname, '../chrome');
const electronRoot = path.resolve(__dirname, '../electron');

const outDir = process.env.COLOPRESSO_DEST_DIR;
if (!outDir) {
  throw new Error('COLOPRESSO_DEST_DIR must be defined to build shared assets.');
}

const buildTarget = process.env.COLOPRESSO_BUILD_TARGET ?? 'electron';

const inputs: Record<string, string> = {};
if (buildTarget === 'electron') {
  inputs['renderer'] = path.resolve(electronRoot, 'entries/electronRenderer.tsx');
  inputs['conversionWorker'] = path.resolve(sharedRoot, 'core/conversionWorker.ts');
} else if (buildTarget === 'chrome') {
  inputs['sidepanel'] = path.resolve(chromeRoot, 'entries/chromeSidepanel.tsx');
  inputs['offscreen'] = path.resolve(chromeRoot, 'entries/chromeOffscreen.ts');
  inputs['background'] = path.resolve(chromeRoot, 'entries/chromeBackground.ts');
} else {
  throw new Error(`Unsupported COLOPRESSO_BUILD_TARGET value: ${buildTarget}`);
}

const chunkFilePattern = buildTarget === 'electron' ? 'chunks-render-[name].js' : 'chunks-chrome-[name].js';
const allowedPaths = Array.from(new Set([sharedRoot, chromeRoot, electronRoot, outDir]));

export default defineConfig({
  root: sharedRoot,
  plugins: [react()],
  server: {
    fs: {
      allow: allowedPaths,
    },
  },
  build: {
    outDir,
    emptyOutDir: true,
    sourcemap: false,
    cssCodeSplit: false,
    target: 'es2020',
    rollupOptions: {
      input: inputs,
      output: {
        entryFileNames: '[name].js',
        chunkFileNames: chunkFilePattern,
        assetFileNames: (assetInfo) => {
          if (assetInfo.name && assetInfo.name.endsWith('.css')) {
            return 'style.css';
          }
          return '[name][extname]';
        },
      },
      onwarn(warning, warn) {
        if (warning.code === 'PLUGIN_WARNING' && warning.message?.includes('has been externalized for browser compatibility')) {
          return;
        }
        warn(warning);
      },
    },
  },
});
