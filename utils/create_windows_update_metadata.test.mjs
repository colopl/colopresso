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
import { execFile } from 'node:child_process';
import { createHash } from 'node:crypto';
import { mkdtemp, readFile, rm, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import path from 'node:path';
import test from 'node:test';
import { promisify } from 'node:util';
import { fileURLToPath } from 'node:url';
import { createWindowsUpdateMetadata } from './create_windows_update_metadata.mjs';

const execFileAsync = promisify(execFile);
const scriptPath = fileURLToPath(new URL('./create_windows_update_metadata.mjs', import.meta.url));

function sha512(value) {
  return createHash('sha512').update(value).digest('base64');
}

test('creates update metadata listing the x64 installer before the arm64 installer', async (context) => {
  const directory = await mkdtemp(path.join(tmpdir(), 'colopresso-update-metadata-'));
  context.after(() => rm(directory, { recursive: true, force: true }));

  const x64Contents = Buffer.from('x64 installer');
  const arm64Contents = Buffer.from('arm64 installer');
  const x64Path = path.join(directory, 'colopresso_colopl_internal_windows_gui_x64.exe');
  const arm64Path = path.join(directory, 'colopresso_colopl_internal_windows_gui_arm64.exe');
  const outputPath = path.join(directory, 'colopl-internal.yml');
  await Promise.all([writeFile(x64Path, x64Contents), writeFile(arm64Path, arm64Contents)]);

  await execFileAsync(process.execPath, [scriptPath, '--version', '14.1.0-colopl-internal', '--x64', x64Path, '--arm64', arm64Path, '--output', outputPath]);

  const metadata = await readFile(outputPath, 'utf8');
  const lines = metadata.split('\n');
  assert.match(metadata, /^version: '14\.1\.0-colopl-internal'$/m);
  assert.equal(lines[1], 'files:');
  assert.equal(lines[2], "  - url: 'colopresso_colopl_internal_windows_gui_x64.exe'");
  assert.equal(lines[3], `    sha512: '${sha512(x64Contents)}'`);
  assert.equal(lines[4], `    size: ${x64Contents.length}`);
  assert.equal(lines[5], "  - url: 'colopresso_colopl_internal_windows_gui_arm64.exe'");
  assert.equal(lines[6], `    sha512: '${sha512(arm64Contents)}'`);
  assert.equal(lines[7], `    size: ${arm64Contents.length}`);
  assert.match(metadata, /^path: 'colopresso_colopl_internal_windows_gui_x64\.exe'$/m);
  assert.ok(lines.includes(`sha512: '${sha512(x64Contents)}'`));
  assert.match(metadata, /^releaseDate: '\d{4}-\d{2}-\d{2}T[^']+Z'$/m);
});

test('rejects an installer with the wrong architecture suffix', async (context) => {
  const directory = await mkdtemp(path.join(tmpdir(), 'colopresso-update-metadata-'));
  context.after(() => rm(directory, { recursive: true, force: true }));

  const x64Path = path.join(directory, 'colopresso_windows_gui_x64.exe');
  const invalidArm64Path = path.join(directory, 'colopresso_windows_gui.exe');
  const outputPath = path.join(directory, 'latest.yml');
  await Promise.all([writeFile(x64Path, 'x64 installer'), writeFile(invalidArm64Path, 'universal installer')]);

  await assert.rejects(
    createWindowsUpdateMetadata({
      version: '14.1.0',
      x64: x64Path,
      arm64: invalidArm64Path,
      output: outputPath,
      releaseDate: '2026-08-28T00:00:00.000Z',
    }),
    /Expected arm64 installer name/
  );
});

test('rejects an invalid version string', async (context) => {
  const directory = await mkdtemp(path.join(tmpdir(), 'colopresso-update-metadata-'));
  context.after(() => rm(directory, { recursive: true, force: true }));

  await assert.rejects(
    createWindowsUpdateMetadata({
      version: 'v14.1.0',
      x64: path.join(directory, 'colopresso_windows_gui_x64.exe'),
      arm64: path.join(directory, 'colopresso_windows_gui_arm64.exe'),
      output: path.join(directory, 'latest.yml'),
    }),
    /Invalid version/
  );
});
