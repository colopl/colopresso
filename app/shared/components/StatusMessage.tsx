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

import React from 'react';
import styles from '../styles/App.module.css';

export type StatusType = 'success' | 'error' | 'info';

interface StatusAction {
  label: string;
  title?: string;
  onClick: () => void;
}

interface StatusMessageProps {
  message: string | null;
  type: StatusType;
  action?: StatusAction;
  onClose?: () => void;
}

const StatusMessage: React.FC<StatusMessageProps> = ({ message, type, action, onClose }) => {
  if (!message) {
    return null;
  }

  const statusClassName = [
    styles.statusMessage,
    type === 'success' ? styles.statusMessageSuccess : '',
    type === 'error' ? styles.statusMessageError : '',
    type === 'info' ? styles.statusMessageInfo : '',
  ]
    .filter(Boolean)
    .join(' ');

  return (
    <div className={statusClassName}>
      <div className={styles.statusMessageContent}>
        <span className={styles.statusMessageText}>{message}</span>
        {action && (
          <button type="button" className={styles.statusMessageButton} title={action.title} onClick={action.onClick}>
            {action.label}
          </button>
        )}
      </div>
      {onClose && (
        <button className={styles.statusClose} onClick={onClose}>
          ×
        </button>
      )}
    </div>
  );
};

export default StatusMessage;
