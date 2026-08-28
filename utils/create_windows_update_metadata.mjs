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

import { createReadStream, realpathSync } from 'node:fs';
import { stat, writeFile } from 'node:fs/promises';
import { basename, resolve } from 'node:path';
import { createHash } from 'node:crypto';
import { fileURLToPath } from 'node:url';

function parseArguments(argumentsList) {
  const options = {};

  for (let index = 0; index < argumentsList.length; index += 2) {
    const option = argumentsList[index];
    const value = argumentsList[index + 1];

    if (!option?.startsWith('--') || value === undefined) {
      throw new Error(`Invalid argument near ${option ?? '(end of arguments)'}`);
    }
    options[option.slice(2)] = value;
  }

  for (const required of ['version', 'arm64', 'x64', 'output']) {
    if (!options[required]) {
      throw new Error(`Missing required option --${required}`);
    }
  }

  return options;
}

function quoteYamlString(value) {
  return `'${value.replaceAll("'", "''")}'`;
}

async function calculateSha512(filePath) {
  const hash = createHash('sha512');

  await new Promise((resolvePromise, rejectPromise) => {
    const stream = createReadStream(filePath);
    stream.on('data', (chunk) => hash.update(chunk));
    stream.on('error', rejectPromise);
    stream.on('end', resolvePromise);
  });

  return hash.digest('base64');
}

async function inspectInstaller(filePath, architecture) {
  const installerName = basename(filePath);
  const expectedSuffix = `_windows_gui_${architecture}.exe`;

  if (!installerName.endsWith(expectedSuffix)) {
    throw new Error(`Expected ${architecture} installer name to end with ${expectedSuffix}: ${installerName}`);
  }

  const [fileStat, sha512] = await Promise.all([stat(filePath), calculateSha512(filePath)]);
  if (!fileStat.isFile()) {
    throw new Error(`Installer is not a regular file: ${filePath}`);
  }

  return {
    url: installerName,
    sha512,
    size: fileStat.size,
  };
}

// x64 is listed first and used for the legacy path/sha512 fields: updaters that
// do not pick a file by architecture take the first entry, and the x64 build
// also runs on arm64 Windows through emulation.
function renderMetadata(version, files, releaseDate) {
  const fallback = files.find((file) => file.url.endsWith('_x64.exe'));
  if (!fallback) {
    throw new Error('Missing x64 fallback installer');
  }

  const lines = [`version: ${quoteYamlString(version)}`, 'files:'];
  for (const file of files) {
    lines.push(`  - url: ${quoteYamlString(file.url)}`);
    lines.push(`    sha512: ${quoteYamlString(file.sha512)}`);
    lines.push(`    size: ${file.size}`);
  }
  lines.push(`path: ${quoteYamlString(fallback.url)}`);
  lines.push(`sha512: ${quoteYamlString(fallback.sha512)}`);
  lines.push(`releaseDate: ${quoteYamlString(releaseDate)}`);

  return `${lines.join('\n')}\n`;
}

export async function createWindowsUpdateMetadata({ version, arm64, x64, output, releaseDate = new Date().toISOString() }) {
  if (!/^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$/.test(version)) {
    throw new Error(`Invalid version: ${version}`);
  }
  if (Number.isNaN(Date.parse(releaseDate))) {
    throw new Error(`Invalid release date: ${releaseDate}`);
  }

  const [x64File, arm64File] = await Promise.all([inspectInstaller(x64, 'x64'), inspectInstaller(arm64, 'arm64')]);
  const metadata = renderMetadata(version, [x64File, arm64File], releaseDate);
  await writeFile(output, metadata, 'utf8');

  return metadata;
}

async function main() {
  const options = parseArguments(process.argv.slice(2));
  await createWindowsUpdateMetadata(options);
}

function isExecutedDirectly() {
  if (!process.argv[1]) {
    return false;
  }

  try {
    return realpathSync(resolve(process.argv[1])) === fileURLToPath(import.meta.url);
  } catch {
    return false;
  }
}

if (isExecutedDirectly()) {
  main().catch((error) => {
    console.error(error instanceof Error ? error.message : error);
    process.exitCode = 1;
  });
}
