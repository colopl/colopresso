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

export function formatVersion(version?: number): string {
  if (typeof version !== 'number' || !Number.isFinite(version)) {
    return 'unknown';
  }

  const major = Math.floor(version / 1_000_000);
  const minor = Math.floor((version % 1_000_000) / 1_000);
  const patch = version % 1_000;

  return `${major}.${minor}.${patch}`;
}

export function formatWebpVersion(version?: number): string {
  if (typeof version !== 'number' || !Number.isFinite(version)) {
    return 'unknown';
  }

  const major = (version >> 16) & 0xff;
  const minor = (version >> 8) & 0xff;
  const patch = version & 0xff;

  return `${major}.${minor}.${patch}`;
}

export function formatLibpngVersion(version?: number): string {
  if (typeof version !== 'number' || !Number.isFinite(version)) {
    return 'unknown';
  }

  const major = Math.floor(version / 10_000);
  const minor = Math.floor((version % 10_000) / 100);
  const patch = version % 100;

  return `${major}.${minor}.${patch}`;
}

export function formatLibavifVersion(version?: number): string {
  if (typeof version !== 'number' || !Number.isFinite(version) || version <= 0) {
    return 'unknown';
  }

  const major = Math.floor(version / 1_000_000);
  const minor = Math.floor((version % 1_000_000) / 10_000);
  const patch = Math.floor((version % 10_000) / 100);

  return `${major}.${minor}.${patch}`;
}

function formatBridgePackedVersion(version?: number): string {
  if (typeof version !== 'number' || !Number.isFinite(version) || version <= 0) {
    return 'unknown';
  }

  const major = Math.floor(version / 10_000);
  const minor = Math.floor((version % 10_000) / 100);
  const patch = version % 100;

  return `${major}.${minor}.${patch}`;
}

export function formatOxipngVersion(version?: number): string {
  return formatBridgePackedVersion(version);
}

export function formatLibimagequantVersion(version?: number): string {
  return formatBridgePackedVersion(version);
}

export function formatBuildtime(buildtime?: number): string {
  if (typeof buildtime !== 'number' || !Number.isFinite(buildtime)) {
    return 'unknown';
  }

  const year = (buildtime >> 20) & 0xfff;
  const month = (buildtime >> 16) & 0xf;
  const day = (buildtime >> 11) & 0x1f;
  const hour = (buildtime >> 6) & 0x1f;
  const minute = buildtime & 0x3f;

  const utcMs = Date.UTC(year, month - 1, day, hour, minute, 0, 0);
  const jst = new Date(utcMs + 9 * 60 * 60 * 1000);
  const yyyy = jst.getUTCFullYear();
  const mm = String(jst.getUTCMonth() + 1).padStart(2, '0');
  const dd = String(jst.getUTCDate()).padStart(2, '0');
  const hh = String(jst.getUTCHours()).padStart(2, '0');
  const mi = String(jst.getUTCMinutes()).padStart(2, '0');

  return `${yyyy}-${mm}-${dd} ${hh}:${mi} JST`;
}

export function formatRustVersionString(version?: string): string {
  if (!version || typeof version !== 'string') {
    return 'unknown';
  }

  return version;
}

export function formatBytes(bytes: number): string {
  if (!Number.isFinite(bytes)) {
    return '0 B';
  }

  if (bytes === 0) {
    return '0 B';
  }

  const units = ['B', 'KiB', 'MiB', 'GiB'];
  const index = Math.min(Math.floor(Math.log(bytes) / Math.log(1024)), units.length - 1);
  const value = bytes / Math.pow(1024, index);

  return `${value.toFixed(1)} ${units[index]}`;
}
