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
import { BuildInfoState, TranslationParams } from '../types';
import { formatVersion, formatWebpVersion, formatLibpngVersion, formatLibavifVersion, formatOxipngVersion, formatLibimagequantVersion, formatBuildtime } from '../core/formatting';

interface BuildInfoPanelProps {
  title: string;
  buildInfo: BuildInfoState;
  t: (key: string, params?: TranslationParams) => string;
}

const BuildInfoPanel: React.FC<BuildInfoPanelProps> = ({ title, buildInfo, t }) => {
  if (buildInfo.status === 'ready' && buildInfo.payload) {
    const payload = buildInfo.payload;
    const libcolopressoVersion = formatVersion(payload.version);
    const libwebpVersion = formatWebpVersion(payload.libwebpVersion);
    const libpngVersion = formatLibpngVersion(payload.libpngVersion);
    const libavifVersion = formatLibavifVersion(payload.libavifVersion);
    const oxipngVersion = formatOxipngVersion(payload.pngxOxipngVersion);
    const libimagequantVersion = formatLibimagequantVersion(payload.pngxLibimagequantVersion);
    const buildtime = formatBuildtime(payload.buildtime);
    const releaseChannel = (payload.releaseChannel ?? '').trim();

    return (
      <div className={styles.buildInfo}>
        <div className={styles.buildInfoTitle}>{title}</div>
        <div className={styles.buildInfoItem}>- libcolopresso: {libcolopressoVersion}</div>
        <div className={styles.buildInfoItem}>- libwebp: {libwebpVersion}</div>
        <div className={styles.buildInfoItem}>- libpng: {libpngVersion}</div>
        <div className={styles.buildInfoItem}>- libavif: {libavifVersion}</div>
        <div className={styles.buildInfoItem}>- oxipng: {oxipngVersion}</div>
        <div className={styles.buildInfoItem}>- libimagequant: {libimagequantVersion}</div>
        <div className={styles.buildInfoItem}>- buildtime: {buildtime}</div>
        {releaseChannel && <div className={styles.buildInfoItem}>- Release Channel: {releaseChannel}</div>}
      </div>
    );
  }

  if (buildInfo.status === 'error') {
    return (
      <div className={styles.buildInfo}>
        <div className={styles.buildInfoTitle}>{title}</div>
        <div className={styles.buildInfoLoading}>{t('buildInfo.error')}</div>
      </div>
    );
  }

  return (
    <div className={styles.buildInfo}>
      <div className={styles.buildInfoTitle}>{title}</div>
      <div className={styles.buildInfoLoading}>{t('buildInfo.loading')}</div>
    </div>
  );
};

export default BuildInfoPanel;
