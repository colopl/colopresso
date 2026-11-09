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
# batch saved as a ZIP, then checks every output. Usage: smoke.sh [work_dir]

set -euo pipefail

SKILL_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${SKILL_DIR}/../../.." && pwd)"
WORK_DIR="${1:-$(mktemp -d "${TMPDIR:-/tmp}/colopresso-e2e.XXXXXX")}"
: "${PLAYWRIGHT_DIR:?install playwright-core into a directory and set PLAYWRIGHT_DIR to it}"

export SCREENSHOT_DIR="${SCREENSHOT_DIR:-${WORK_DIR}/shots}"
export COLOPRESSO_E2E_USER_DATA="${COLOPRESSO_E2E_USER_DATA:-${WORK_DIR}/userdata}"
INPUT_DIR="${WORK_DIR}/in"
ASSETS=("example.png" "128x128.png" "16bit.png" "banding.png")
FOLDER_SCENARIOS=("webp" "avif" "pngx-palette256" "pngx-limited" "pngx-reduced")

rm -rf "${INPUT_DIR}"
mkdir -p "${INPUT_DIR}/drop" "${SCREENSHOT_DIR}" "${COLOPRESSO_E2E_USER_DATA}"
for scenario in "${FOLDER_SCENARIOS[@]}"; do
  mkdir -p "${INPUT_DIR}/${scenario}"
  for asset in "${ASSETS[@]}"; do
    cp "${REPO_ROOT}/assets/${asset}" "${INPUT_DIR}/${scenario}/"
  done
done

commands() {
  echo "launch"
  echo "ss 00-launch"
  for format in webp avif; do
    echo "format ${format}"
    echo "mock-folder ${INPUT_DIR}/${format}"
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
    echo "mock-folder ${INPUT_DIR}/pngx-${mode}"
    echo "pick"
    echo "wait 900"
    echo "files"
    echo "ss 02-pngx-${mode}"
    type=$((type + 1))
  done
  echo "format webp"
  echo "mock-save ${INPUT_DIR}/drop/example-single.webp"
  echo "drop ${REPO_ROOT}/assets/example.png"
  echo "wait 300"
  echo "eval (() => { const c = document.getElementById('createZipCheckbox'); if (!c.checked) c.click(); return c.checked; })()"
  echo "mock-save ${INPUT_DIR}/drop/batch.zip"
  echo "drop ${REPO_ROOT}/assets/example.png ${REPO_ROOT}/assets/128x128.png ${REPO_ROOT}/assets/banding.png"
  echo "wait 300"
  echo "status"
  echo "ss 03-drop-zip"
  echo "logs 200"
  echo "quit"
}

commands | node "${SKILL_DIR}/scripts/driver.mjs" | tee "${WORK_DIR}/driver.log"

status=0
if grep -Eq "^(ERROR:|TIMEOUT)| -> NOT_FOUND|renderer:pageerror|\[renderer:error\]" "${WORK_DIR}/driver.log"; then
  echo "driver reported problems; see ${WORK_DIR}/driver.log" >&2
  status=1
fi
python3 -I "${SKILL_DIR}/scripts/check_outputs.py" "${INPUT_DIR}" || status=1
echo "work dir: ${WORK_DIR}"
exit "${status}"
