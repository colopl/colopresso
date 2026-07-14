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
import { createMacosUpdateMetadata } from './create_macos_update_metadata.mjs';

const execFileAsync = promisify(execFile);
const scriptPath = fileURLToPath(new URL('./create_macos_update_metadata.mjs', import.meta.url));

function sha512(value) {
  return createHash('sha512').update(value).digest('base64');
}

test('creates update metadata containing x64 and arm64 archives', async (context) => {
  const directory = await mkdtemp(path.join(tmpdir(), 'colopresso-update-metadata-'));
  context.after(() => rm(directory, { recursive: true, force: true }));

  const x64Contents = Buffer.from('x64 archive');
  const arm64Contents = Buffer.from('arm64 archive');
  const x64Path = path.join(directory, 'colopresso_macos_gui_x64.zip');
  const arm64Path = path.join(directory, 'colopresso_macos_gui_arm64.zip');
  const outputPath = path.join(directory, 'latest-mac.yml');
  await Promise.all([writeFile(x64Path, x64Contents), writeFile(arm64Path, arm64Contents)]);

  await execFileAsync(process.execPath, [scriptPath, '--version', '14.0.1', '--x64', x64Path, '--arm64', arm64Path, '--output', outputPath]);

  const metadata = await readFile(outputPath, 'utf8');
  const lines = metadata.split('\n');
  assert.match(metadata, /^version: '14\.0\.1'$/m);
  assert.match(metadata, /^  - url: 'colopresso_macos_gui_x64\.zip'$/m);
  assert.ok(metadata.includes(`    sha512: '${sha512(x64Contents)}'`));
  assert.ok(metadata.includes(`    size: ${x64Contents.length}`));
  assert.match(metadata, /^  - url: 'colopresso_macos_gui_arm64\.zip'$/m);
  assert.ok(metadata.includes(`    sha512: '${sha512(arm64Contents)}'`));
  assert.ok(metadata.includes(`    size: ${arm64Contents.length}`));
  assert.match(metadata, /^path: 'colopresso_macos_gui_x64\.zip'$/m);
  assert.ok(lines.includes(`sha512: '${sha512(x64Contents)}'`));
  assert.match(metadata, /^releaseDate: '\d{4}-\d{2}-\d{2}T[^']+Z'$/m);
});

test('rejects an archive with the wrong architecture suffix', async (context) => {
  const directory = await mkdtemp(path.join(tmpdir(), 'colopresso-update-metadata-'));
  context.after(() => rm(directory, { recursive: true, force: true }));

  const x64Path = path.join(directory, 'colopresso_macos_gui_x64.zip');
  const invalidArm64Path = path.join(directory, 'colopresso_macos_gui_x64-copy.zip');
  const outputPath = path.join(directory, 'latest-mac.yml');
  await Promise.all([writeFile(x64Path, 'x64 archive'), writeFile(invalidArm64Path, 'arm64 archive')]);

  await assert.rejects(
    createMacosUpdateMetadata({
      version: '14.0.1',
      x64: x64Path,
      arm64: invalidArm64Path,
      output: outputPath,
      releaseDate: '2026-07-14T02:00:00.000Z',
    }),
    /Expected arm64 archive name/
  );
});
