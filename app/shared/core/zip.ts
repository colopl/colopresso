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

export class ColoZip {
  private files: Array<{ filename: string; data: Uint8Array }> = [];
  private crc32Table: Uint32Array | null = null;

  addFile(filename: string, data: Uint8Array | ArrayLike<number>): void {
    const buffer = data instanceof Uint8Array ? data : new Uint8Array(data);
    this.files.push({ filename, data: buffer });
  }

  generate(): Uint8Array {
    const centralDirectory: Uint8Array[] = [];
    const fileData: Uint8Array[] = [];
    let offset = 0;

    for (const file of this.files) {
      const filenameBytes = new TextEncoder().encode(file.filename);
      const data = file.data;
      const crc32 = this.calculateCRC32(data);

      const localHeader = new Uint8Array(30 + filenameBytes.length);
      const localView = new DataView(localHeader.buffer);

      localView.setUint32(0, 0x04034b50, true);
      localView.setUint16(4, 20, true);
      localView.setUint16(6, 0, true);
      localView.setUint16(8, 0, true);
      localView.setUint16(10, 0, true);
      localView.setUint16(12, 0, true);
      localView.setUint32(14, crc32, true);
      localView.setUint32(18, data.length, true);
      localView.setUint32(22, data.length, true);
      localView.setUint16(26, filenameBytes.length, true);
      localView.setUint16(28, 0, true);
      localHeader.set(filenameBytes, 30);

      fileData.push(localHeader, data);

      const centralHeader = new Uint8Array(46 + filenameBytes.length);
      const centralView = new DataView(centralHeader.buffer);

      centralView.setUint32(0, 0x02014b50, true);
      centralView.setUint16(4, 20, true);
      centralView.setUint16(6, 20, true);
      centralView.setUint16(8, 0, true);
      centralView.setUint16(10, 0, true);
      centralView.setUint16(12, 0, true);
      centralView.setUint16(14, 0, true);
      centralView.setUint32(16, crc32, true);
      centralView.setUint32(20, data.length, true);
      centralView.setUint32(24, data.length, true);
      centralView.setUint16(28, filenameBytes.length, true);
      centralView.setUint16(30, 0, true);
      centralView.setUint16(32, 0, true);
      centralView.setUint16(34, 0, true);
      centralView.setUint16(36, 0, true);
      centralView.setUint32(38, 0, true);
      centralView.setUint32(42, offset, true);
      centralHeader.set(filenameBytes, 46);

      centralDirectory.push(centralHeader);

      offset += localHeader.length + data.length;
    }

    const centralDirectorySize = centralDirectory.reduce((acc, entry) => acc + entry.length, 0);

    const endOfCentralDir = new Uint8Array(22);
    const endView = new DataView(endOfCentralDir.buffer);

    endView.setUint32(0, 0x06054b50, true);
    endView.setUint16(4, 0, true);
    endView.setUint16(6, 0, true);
    endView.setUint16(8, this.files.length, true);
    endView.setUint16(10, this.files.length, true);
    endView.setUint32(12, centralDirectorySize, true);
    endView.setUint32(16, offset, true);
    endView.setUint16(20, 0, true);

    const totalSize = offset + centralDirectorySize + endOfCentralDir.length;
    const zip = new Uint8Array(totalSize);
    let position = 0;

    for (const chunk of fileData) {
      zip.set(chunk, position);
      position += chunk.length;
    }

    for (const entry of centralDirectory) {
      zip.set(entry, position);
      position += entry.length;
    }

    zip.set(endOfCentralDir, position);
    return zip;
  }

  private calculateCRC32(data: Uint8Array): number {
    const table = this.getCRC32Table();
    let crc = 0xffffffff;
    for (let i = 0; i < data.length; i += 1) {
      crc = (crc >>> 8) ^ table[(crc ^ data[i]) & 0xff];
    }
    return (crc ^ 0xffffffff) >>> 0;
  }

  private getCRC32Table(): Uint32Array {
    if (this.crc32Table) {
      return this.crc32Table;
    }
    const table = new Uint32Array(256);
    for (let i = 0; i < 256; i += 1) {
      let c = i;
      for (let j = 0; j < 8; j += 1) {
        if ((c & 1) !== 0) {
          c = 0xedb88320 ^ (c >>> 1);
        } else {
          c >>>= 1;
        }
      }
      table[i] = c >>> 0;
    }
    this.crc32Table = table;
    return table;
  }
}

export default ColoZip;
