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

import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { createRequire } from 'node:module';
import path from 'node:path';
import { test } from 'node:test';

const require = createRequire(import.meta.url);
const addonPath = path.resolve(process.env.COLOPRESSO_NATIVE_ADDON_PATH ?? path.join(import.meta.dirname, '../../../build/electron/native/colopresso_native.node'));
const assetsDir = path.resolve(process.env.COLOPRESSO_TEST_ASSETS_DIR ?? path.join(import.meta.dirname, '../../../assets'));
const addon = require(addonPath);
const input = readFileSync(path.join(assetsDir, 'example.png'));

const PNG_COLOR_TYPE_RGBA = 6;
const PNG_COLOR_TYPE_PALETTE = 3;
const PNG_IHDR_COLOR_TYPE_OFFSET = 25;

async function convertPngx(options) {
  const result = await addon.convert('pngx', { pngx_lossy_enable: true, ...options }, input, 1);
  return Buffer.from(result.outputBytes);
}

test('string-typed lossy type selects the same encoder as the numeric value', async () => {
  const numeric = await convertPngx({ pngx_lossy_type: 1 });
  const text = await convertPngx({ pngx_lossy_type: '1' });

  assert.equal(numeric[PNG_IHDR_COLOR_TYPE_OFFSET], PNG_COLOR_TYPE_RGBA);
  assert.deepEqual(text, numeric);
});

test('string-typed palette lossy type still produces a palette image', async () => {
  const palette = await convertPngx({ pngx_lossy_type: '0' });

  assert.equal(palette[PNG_IHDR_COLOR_TYPE_OFFSET], PNG_COLOR_TYPE_PALETTE);
});

test('string-typed booleans are honored', async () => {
  const lossy = await convertPngx({ pngx_lossy_type: 1, pngx_lossy_enable: true });
  const lossyText = await convertPngx({ pngx_lossy_type: 1, pngx_lossy_enable: 'true' });
  const lossless = await convertPngx({ pngx_lossy_type: 1, pngx_lossy_enable: false });
  const losslessText = await convertPngx({ pngx_lossy_type: 1, pngx_lossy_enable: 'false' });

  assert.deepEqual(lossyText, lossy);
  assert.deepEqual(losslessText, lossless);
  assert.notDeepEqual(lossless, lossy);
});

test('numeric strings are accepted up to the scalar buffer limit and rejected beyond it', async () => {
  const numeric = await convertPngx({ pngx_lossy_type: 1 });
  const atLimit = await convertPngx({ pngx_lossy_type: '1'.padStart(63, '0') });
  const beyondLimit = await convertPngx({ pngx_lossy_type: '1'.padStart(64, '0') });
  const palette = await convertPngx({ pngx_lossy_type: 0 });

  assert.deepEqual(atLimit, numeric);
  assert.equal(beyondLimit[PNG_IHDR_COLOR_TYPE_OFFSET], palette[PNG_IHDR_COLOR_TYPE_OFFSET]);
});

test('out-of-range and non-finite numbers are ignored instead of being converted', async () => {
  const palette = await convertPngx({ pngx_lossy_type: 0 });
  const reference = await convertPngx({ pngx_lossy_type: 1 });

  for (const value of ['1e100', 1e100, -1e100, Number.NaN, Number.POSITIVE_INFINITY]) {
    const ignoredType = await convertPngx({ pngx_lossy_type: value });
    const ignoredFields = await convertPngx({ pngx_lossy_type: 1, pngx_lossy_quality_min: value, pngx_postprocess_smooth_importance_cutoff: value });

    assert.equal(ignoredType[PNG_IHDR_COLOR_TYPE_OFFSET], palette[PNG_IHDR_COLOR_TYPE_OFFSET], `pngx_lossy_type=${value}`);
    assert.ok(ignoredFields.equals(reference), `int/float fields=${value}`);
  }

  for (const value of [Number.NaN, Number.POSITIVE_INFINITY]) {
    const ignoredDither = await convertPngx({ pngx_lossy_type: 1, pngx_lossy_dither_level: value });

    assert.ok(ignoredDither.equals(reference), `pngx_lossy_dither_level=${value}`);
  }
});

test('non-numeric strings fall back to the default instead of failing', async () => {
  const fallback = await convertPngx({ pngx_lossy_type: 'not-a-number' });
  const palette = await convertPngx({ pngx_lossy_type: 0 });

  assert.equal(fallback[PNG_IHDR_COLOR_TYPE_OFFSET], palette[PNG_IHDR_COLOR_TYPE_OFFSET]);
});
