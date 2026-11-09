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
USER_DATA_DIR="$(posix_path "${COLOPRESSO_E2E_USER_DATA:-${WORK_DIR}/userdata}")"
ASSETS=("example.png" "128x128.png" "16bit.png" "banding.png")
FOLDER_SCENARIOS=("webp" "avif" "pngx-palette256" "pngx-limited" "pngx-reduced")

rm -rf "${INPUT_DIR}"
mkdir -p "${INPUT_DIR}/drop" "${SHOT_DIR}" "${USER_DATA_DIR}"
for scenario in "${FOLDER_SCENARIOS[@]}"; do
  mkdir -p "${INPUT_DIR}/${scenario}"
  for asset in "${ASSETS[@]}"; do
    cp "${REPO_ROOT}/assets/${asset}" "${INPUT_DIR}/${scenario}/"
  done
done

export SCREENSHOT_DIR COLOPRESSO_E2E_USER_DATA
SCREENSHOT_DIR="$(native_path "${SHOT_DIR}")"
COLOPRESSO_E2E_USER_DATA="$(native_path "${USER_DATA_DIR}")"
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
    echo "mock-folder ${IN}/${format}"
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
    echo "mock-folder ${IN}/pngx-${mode}"
    echo "pick"
    echo "wait 900"
    echo "files"
    echo "ss 02-pngx-${mode}"
    type=$((type + 1))
  done
  echo "format webp"
  echo "mock-save ${IN}/drop/example-single.webp"
  echo "drop ${ASSET_DIR}/example.png"
  echo "wait 300"
  echo "eval (() => { const c = document.getElementById('createZipCheckbox'); if (!c.checked) c.click(); return c.checked; })()"
  echo "mock-save ${IN}/drop/batch.zip"
  echo "drop ${ASSET_DIR}/example.png ${ASSET_DIR}/128x128.png ${ASSET_DIR}/banding.png"
  echo "wait 300"
  echo "status"
  echo "ss 03-drop-zip"
  echo "logs 200"
  echo "quit"
}

status=0
commands | node "$(native_path "${SCRIPT_DIR}/driver.mjs")" | tee "${WORK_DIR}/driver.log" || status=1

if grep -Eq "^(ERROR:|TIMEOUT)| -> NOT_FOUND|renderer:pageerror|\[renderer:error\]" "${WORK_DIR}/driver.log"; then
  echo "driver reported problems; see ${WORK_DIR}/driver.log" >&2
  status=1
fi
"${PYTHON}" -I "$(native_path "${SCRIPT_DIR}/check_outputs.py")" "${IN}" || status=1
echo "work dir: $(native_path "${WORK_DIR}")"
exit "${status}"
