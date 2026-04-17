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

import path from 'node:path';
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

const sharedRoot = path.resolve(__dirname);
const electronRoot = path.resolve(__dirname, '../electron');

const outDir = process.env.COLOPRESSO_DEST_DIR;
if (!outDir) {
  throw new Error('COLOPRESSO_DEST_DIR must be defined to build shared assets.');
}

const inputs: Record<string, string> = {
  renderer: path.resolve(electronRoot, 'entries/electronRenderer.tsx'),
  conversionWorker: path.resolve(sharedRoot, 'core/conversionWorker.ts'),
};

const allowedPaths = Array.from(new Set([sharedRoot, electronRoot, outDir]));

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
        chunkFileNames: 'chunks-render-[name].js',
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
