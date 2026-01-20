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

import { defineConfig } from 'vite';
import path from 'node:path';
import { builtinModules } from 'node:module';

type RollupExternal = Array<string | RegExp>;

const nodeBuiltins = Array.from(new Set([...builtinModules, ...builtinModules.map((mod) => `node:${mod}`)]));
const external: RollupExternal = ['electron', ...nodeBuiltins];

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
