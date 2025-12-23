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

import React from 'react';
import styles from '../styles/App.module.css';
import { FileEntry, TranslationParams } from '../types';

interface FileListProps {
  entries: FileEntry[];
  formatStatusText: (entry: FileEntry) => string;
  getErrorMessage: (entry: FileEntry) => string;
  onCopyError: (entry: FileEntry) => void;
  formatBytes: (bytes: number) => string;
  t: (key: string, params?: TranslationParams) => string;
}

const FileList: React.FC<FileListProps> = ({ entries, formatStatusText, getErrorMessage, onCopyError, formatBytes, t }) => {
  if (entries.length === 0) {
    return null;
  }

  return (
    <div className={styles.fileList}>
      {entries.map((entry) => {
        const fileStatusClass =
          entry.status === 'success'
            ? styles.fileStatusSuccess
            : entry.status === 'processing'
              ? styles.fileStatusProcessing
              : entry.status === 'error'
                ? styles.fileStatusError
                : styles.fileStatusPending;
        const isError = entry.status === 'error';
        const errorTooltip = isError ? getErrorMessage(entry) : undefined;

        return (
          <div className={styles.fileItem} key={entry.id}>
            <div className={styles.fileItemHeader}>
              <span className={styles.fileName} title={entry.name}>
                {entry.name}
              </span>
              {isError ? (
                <button
                  type="button"
                  className={`${styles.fileStatus} ${fileStatusClass} ${styles.fileStatusInteractive}`}
                  onClick={() => onCopyError(entry)}
                  onKeyDown={(event) => {
                    if (event.key === 'Enter' || event.key === ' ') {
                      event.preventDefault();
                      onCopyError(entry);
                    }
                  }}
                >
                  {formatStatusText(entry)}
                  <div className={styles.fileStatusTooltip} role="tooltip">
                    <div className={styles.fileStatusTooltipMessage}>{errorTooltip}</div>
                    <div className={styles.fileStatusTooltipHint}>{t('errors.copyHint')}</div>
                  </div>
                </button>
              ) : (
                <span className={`${styles.fileStatus} ${fileStatusClass}`}>{formatStatusText(entry)}</span>
              )}
            </div>
            {entry.originalSize !== undefined && entry.convertedSize !== undefined && (
              <div className={styles.fileSizeInfo}>
                {entry.conversionTimeMs !== undefined
                  ? t('common.conversionSizeAndTime', {
                      inputSize: formatBytes(entry.originalSize),
                      outputSize: formatBytes(entry.convertedSize),
                      timeMs: entry.conversionTimeMs,
                    })
                  : `${formatBytes(entry.originalSize)} → ${formatBytes(entry.convertedSize)}`}
              </div>
            )}
          </div>
        );
      })}
    </div>
  );
};

export default FileList;
