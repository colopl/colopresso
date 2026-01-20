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

interface ProgressBarProps {
  current: number;
  total: number;
  isVisible: boolean;
  progressText?: string;
}

const ProgressBar: React.FC<ProgressBarProps> = ({ current, total, isVisible, progressText }) => {
  if (!isVisible || total === 0) {
    return null;
  }

  const percentage = Math.round((current / total) * 100);

  return (
    <div className={styles.progressContainer}>
      <div className={styles.progressBar}>
        <div className={styles.progressFill} style={{ width: `${percentage}%` }} />
      </div>
      {progressText && <div className={styles.progressText}>{progressText}</div>}
    </div>
  );
};

export default ProgressBar;
