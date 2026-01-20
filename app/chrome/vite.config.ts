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

import path from 'node:path';
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

const sharedRoot = path.resolve(__dirname, '../shared');
const chromeRoot = path.resolve(__dirname);
const envOutDir = process.env.COLOPRESSO_RESOURCES_DIR ?? process.env.COLOPRESSO_SHARED_DIST_DIR;
const resolveOutDir = (dir: string) => {
  if (path.isAbsolute(dir)) {
    return dir;
  }
  return path.resolve(__dirname, '..', '..', dir);
};
if (!envOutDir) {
  throw new Error('COLOPRESSO_RESOURCES_DIR or COLOPRESSO_SHARED_DIST_DIR must be defined to build Chrome assets.');
}
const outDir = resolveOutDir(envOutDir);
const colopressoModuleSourcePath = path.resolve(outDir, 'colopresso.js');
const colopressoModulePublicPath = './colopresso.js';

export default defineConfig({
  root: sharedRoot,
  plugins: [react()],
  resolve: {
    alias: {
      'colopresso-module': colopressoModuleSourcePath,
    },
  },
  server: {
    fs: {
      allow: Array.from(new Set([sharedRoot, chromeRoot, outDir, path.dirname(colopressoModuleSourcePath)])),
    },
  },
  build: {
    outDir,
    emptyOutDir: false,
    sourcemap: false,
    cssCodeSplit: false,
    target: 'es2020',
    rollupOptions: {
      input: {
        sidepanel: path.resolve(chromeRoot, 'entries/chromeSidepanel.tsx'),
        offscreen: path.resolve(chromeRoot, 'entries/chromeOffscreen.ts'),
        background: path.resolve(chromeRoot, 'entries/chromeBackground.ts'),
      },
      external: [colopressoModuleSourcePath],
      output: {
        entryFileNames: '[name].js',
        chunkFileNames: 'chunks-chrome-[name].js',
        assetFileNames: (assetInfo) => {
          if (assetInfo.name && assetInfo.name.endsWith('.css')) {
            return 'style.css';
          }
          return '[name][extname]';
        },
        paths: {
          [colopressoModuleSourcePath]: colopressoModulePublicPath,
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
