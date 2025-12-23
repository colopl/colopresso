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

import { useCallback } from 'react';
import { useI18n } from '../i18n';
import { formatBytes } from '../core/formatting';
import { FileEntry, FormatDefinition } from '../types';

export interface ConversionErrorInfo {
  errorMessageKey: string;
  errorParams: Record<string, string | number | undefined>;
  errorRawMessage: string;
  inputSize?: number;
  outputSize?: number;
}

export function useConversionErrors() {
  const { t } = useI18n();

  const resolveConversionError = useCallback(
    (error: unknown, format: FormatDefinition, originalSize?: number): ConversionErrorInfo => {
      const fallbackMessage = (error as Error | undefined)?.message ?? t('common.unknownError');
      const baseInputSize = typeof originalSize === 'number' && Number.isFinite(originalSize) ? originalSize : undefined;
      const base: ConversionErrorInfo = {
        errorMessageKey: 'common.errorPrefix',
        errorParams: { message: fallbackMessage },
        errorRawMessage: fallbackMessage,
        inputSize: baseInputSize,
        outputSize: undefined,
      };

      if (!error || typeof error !== 'object') {
        return base;
      }

      const typed = error as Error & {
        code?: string;
        inputSize?: number;
        outputSize?: number;
        details?: { inputSize?: number; outputSize?: number };
      };

      const rawDetails = typed.details ?? {
        inputSize: typed.inputSize,
        outputSize: typed.outputSize,
      };

      const extractSize = (value: unknown): number | undefined => {
        return typeof value === 'number' && Number.isFinite(value) ? value : undefined;
      };

      const detailedInputSize = extractSize(rawDetails?.inputSize);
      const detailedOutputSize = extractSize(rawDetails?.outputSize);
      const inputSize = detailedInputSize ?? baseInputSize;
      const outputSize = detailedOutputSize;

      const describeSize = (size?: number) => {
        if (typeof size !== 'number' || Number.isNaN(size)) {
          return t('errors.sizeUnknown');
        }

        return `${formatBytes(size)} (${size} B)`;
      };

      if (typed.code === 'output_larger_than_input') {
        return {
          errorMessageKey: 'errors.outputLarger',
          errorParams: {
            format: t(format.labelKey),
            inputSize: describeSize(inputSize),
            outputSize: describeSize(outputSize),
          },
          errorRawMessage: fallbackMessage,
          inputSize,
          outputSize,
        };
      }

      return {
        ...base,
        inputSize,
        outputSize,
      };
    },
    [t]
  );

  const getErrorMessageForEntry = useCallback(
    (entry: FileEntry): string => {
      if (entry.errorMessageKey) {
        return t(entry.errorMessageKey, entry.errorParams);
      }

      if (entry.errorParams?.message) {
        return String(entry.errorParams.message);
      }

      if (entry.errorMessageRaw) {
        return entry.errorMessageRaw;
      }

      return t('common.unknownError');
    },
    [t]
  );

  return { resolveConversionError, getErrorMessageForEntry };
}
