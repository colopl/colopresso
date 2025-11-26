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

import { defineConfig } from 'vite';
import path from 'node:path';

type RollupExternal = Array<string | RegExp>;

const external: RollupExternal = ['electron', 'node:fs', 'node:path', 'fs', 'fs/promises', 'path'];

const envOutDir = process.env.COLOPRESSO_ELECTRON_OUT_DIR;
const outDir = envOutDir ? (path.isAbsolute(envOutDir) ? envOutDir : path.resolve(__dirname, '..', '..', envOutDir)) : path.resolve(__dirname, '../../build/electron');

export default defineConfig({
  build: {
    outDir,
    emptyOutDir: false,
    sourcemap: false,
    target: 'node20',
    rollupOptions: {
      input: {
        main: path.resolve(__dirname, 'main.ts'),
        preload: path.resolve(__dirname, 'preload.ts'),
      },
      external,
      output: {
        entryFileNames: '[name].js',
        chunkFileNames: '[name].js',
        assetFileNames: '[name][extname]',
        format: 'cjs',
      },
    },
  },
});
