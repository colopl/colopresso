#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

# Smoke test of the packaged GUI: converts copies of assets/*.png through the GUI as WebP, AVIF and
# PNGX (palette256, limited, reduced) via the folder flow, converts one dropped file and a dropped
# batch saved as a ZIP, then checks every output. Runs on macOS and on Windows under Git Bash.
# Usage: smoke.sh [work_dir]

set -euo pipefail

# Paths handed to node and python must be native on Windows (Git Bash uses /c/... paths).
native_path() {
  if command -v cygpath > /dev/null 2>&1; then
    cygpath -m "$1"
  else
    printf '%s' "$1"
  fi
}

posix_path() {
  if command -v cygpath > /dev/null 2>&1; then
    cygpath -u "$1"
  else
    printf '%s' "$1"
  fi
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
WORK_DIR="$(posix_path "${1:-$(mktemp -d "${TMPDIR:-/tmp}/colopresso-e2e.XXXXXX")}")"
case "$(uname -s)" in
  MINGW* | MSYS* | CYGWIN*) PYTHON="${PYTHON:-python}" ;;
  *) PYTHON="${PYTHON:-python3}" ;;
esac

INPUT_DIR="${WORK_DIR}/in"
SHOT_DIR="$(posix_path "${SCREENSHOT_DIR:-${WORK_DIR}/shots}")"
USER_DATA_ROOT="$(posix_path "${COLOPRESSO_E2E_USER_DATA:-${WORK_DIR}/userdata}")"
ASSETS=("example.png" "128x128.png" "16bit.png" "banding.png")
FOLDER_SCENARIOS=("webp" "avif" "pngx-palette256" "pngx-limited" "pngx-reduced")

RUN_ATTEMPTS="${COLOPRESSO_E2E_RUN_ATTEMPTS:-3}"

# Every attempt starts from fresh copies of the inputs and a new, empty user data directory, so
# settings changed by an earlier attempt do not carry over. Nothing outside this script's own
# directories is deleted; screenshots of a later attempt overwrite those of an earlier one.
prepare_inputs() {
  rm -rf "${INPUT_DIR}"
  mkdir -p "${INPUT_DIR}/drop" "${SHOT_DIR}" "${USER_DATA_ROOT}"
  COLOPRESSO_E2E_USER_DATA="$(native_path "$(mktemp -d "${USER_DATA_ROOT}/attempt-${attempt}.XXXXXX")")"
  for scenario in "${FOLDER_SCENARIOS[@]}"; do
    mkdir -p "${INPUT_DIR}/${scenario}"
    for asset in "${ASSETS[@]}"; do
      cp "${REPO_ROOT}/assets/${asset}" "${INPUT_DIR}/${scenario}/"
    done
  done
}

export SCREENSHOT_DIR COLOPRESSO_E2E_USER_DATA
SCREENSHOT_DIR="$(native_path "${SHOT_DIR}")"
if [[ -n "${COLOPRESSO_APP:-}" ]]; then
  export COLOPRESSO_APP
  COLOPRESSO_APP="$(native_path "${COLOPRESSO_APP}")"
fi
IN="$(native_path "${INPUT_DIR}")"
ASSET_DIR="$(native_path "${REPO_ROOT}/assets")"

commands() {
  echo "launch"
  echo "ss 00-launch"
  for format in webp avif; do
    echo "format ${format}"
    echo "mock-folder \"${IN}/${format}\""
    echo "pick"
    echo "wait 900"
    echo "files"
    echo "ss 01-${format}"
  done
  echo "format png"
  local type=0
  for mode in palette256 limited reduced; do
    echo "settings"
    echo "field pngx_lossy_type ${type}"
    echo "apply"
    echo "mock-folder \"${IN}/pngx-${mode}\""
    echo "pick"
    echo "wait 900"
    echo "files"
    echo "ss 02-pngx-${mode}"
    type=$((type + 1))
  done
  echo "format webp"
  echo "mock-save \"${IN}/drop/example-single.webp\""
  echo "drop \"${ASSET_DIR}/example.png\""
  echo "wait 300"
  echo "eval (() => { const c = document.getElementById('createZipCheckbox'); if (!c.checked) c.click(); return c.checked; })()"
  echo "mock-save \"${IN}/drop/batch.zip\""
  echo "drop \"${ASSET_DIR}/example.png\" \"${ASSET_DIR}/128x128.png\" \"${ASSET_DIR}/banding.png\""
  echo "wait 300"
  echo "status"
  echo "ss 03-drop-zip"
  echo "logs 200"
  echo "quit"
}

# stderr carries Playwright's DEBUG output when it is enabled, so it goes to a file. The driver exits
# with 2 when a command hangs (the app stops responding, seen intermittently on CI runners); only that
# case is retried, from scratch, so ordinary failures are never hidden by a retry.
status=0
attempt=1
while true; do
  prepare_inputs
  driver_status=0
  commands | node "$(native_path "${SCRIPT_DIR}/driver.mjs")" 2> "${WORK_DIR}/driver.stderr.log" | tee "${WORK_DIR}/driver.log" || driver_status="${PIPESTATUS[1]}"
  if [[ "${driver_status}" -eq 2 && "${attempt}" -lt "${RUN_ATTEMPTS}" ]]; then
    mv "${WORK_DIR}/driver.log" "${WORK_DIR}/driver.attempt-${attempt}.log"
    mv "${WORK_DIR}/driver.stderr.log" "${WORK_DIR}/driver.stderr.attempt-${attempt}.log"
    echo "RETRY: attempt ${attempt}/${RUN_ATTEMPTS} hung; starting over" >&2
    attempt=$((attempt + 1))
    continue
  fi
  if [[ "${driver_status}" -ne 0 ]]; then
    status=1
  fi
  break
done

if grep -Eq "^(ERROR:|TIMEOUT)| -> NOT_FOUND|renderer:pageerror|\[renderer:error\]" "${WORK_DIR}/driver.log"; then
  echo "driver reported problems; see ${WORK_DIR}/driver.log" >&2
  status=1
fi
if [[ "${status}" -ne 0 ]]; then
  echo "--- last lines of ${WORK_DIR}/driver.stderr.log ---" >&2
  tail -n 40 "${WORK_DIR}/driver.stderr.log" >&2 || true
fi
"${PYTHON}" -I "$(native_path "${SCRIPT_DIR}/check_outputs.py")" "${IN}" || status=1
echo "work dir: $(native_path "${WORK_DIR}")"
exit "${status}"
