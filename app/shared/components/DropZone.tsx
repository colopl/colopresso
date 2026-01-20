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

import React from 'react';
import styles from '../styles/App.module.css';

interface DropZoneProps {
  iconEmoji: string;
  text: string;
  hint: string;
  isProcessing: boolean;
  isDragOver: boolean;
  onDragOver: (event: React.DragEvent<HTMLDivElement>) => void;
  onDragEnter: (event: React.DragEvent<HTMLDivElement>) => void;
  onDragLeave: (event: React.DragEvent<HTMLDivElement>) => void;
  onDragEnd: () => void;
  onDrop: (event: React.DragEvent<HTMLDivElement>) => void;
  onClick: () => void;
  onCancel?: () => void;
  cancelButtonLabel?: string;
  isCancelling?: boolean;
  cancellingButtonLabel?: string;
}

const DropZone: React.FC<DropZoneProps> = ({
  iconEmoji,
  text,
  hint,
  isProcessing,
  isDragOver,
  onDragOver,
  onDragEnter,
  onDragLeave,
  onDragEnd,
  onDrop,
  onClick,
  onCancel,
  cancelButtonLabel,
  isCancelling,
  cancellingButtonLabel,
}) => {
  const dropZoneClassName = [styles.dropZone, isProcessing ? styles.dropZoneProcessing : '', isDragOver ? styles.dropZoneDragOver : ''].filter(Boolean).join(' ');

  return (
    <div className={dropZoneClassName} onDragOver={onDragOver} onDragEnter={onDragEnter} onDragLeave={onDragLeave} onDragEnd={onDragEnd} onDrop={onDrop} onClick={onClick}>
      <div className={styles.dropZoneIcon}>{iconEmoji}</div>
      <div className={styles.dropZoneText}>{text}</div>
      <div className={styles.dropZoneHint}>{hint}</div>
      {isProcessing && onCancel && (
        <button
          type="button"
          className={styles.cancelButton}
          onClick={(e) => {
            e.stopPropagation();
            onCancel();
          }}
          disabled={isCancelling}
        >
          {isCancelling ? (cancellingButtonLabel ?? 'Cancelling...') : (cancelButtonLabel ?? 'Cancel')}
        </button>
      )}
    </div>
  );
};

export default DropZone;
