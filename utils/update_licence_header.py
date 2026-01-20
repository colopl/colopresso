# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

import sys
import subprocess
from pathlib import Path
from typing import List, Optional, Sequence, Tuple
from textwrap import dedent

HEADERS = {
    "block": dedent(
        """\
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
        """
    ).strip("\n")
    + "\n\n",
    "sharp": dedent(
        """\
        # SPDX-License-Identifier: GPL-3.0-or-later
        #
        # This file is part of colopresso
        #
        # Copyright (C) 2026 COLOPL, Inc.
        #
        # Author: Go Kudo <g-kudo@colopl.co.jp>
        # Developed with AI (LLM) code assistance. See `NOTICE` for details.
        """
    ).strip("\n")
    + "\n\n",
}

SUFFIX_STYLE: Sequence[Tuple[str, str]] = (
    (".h.in", "block"),
    (".tsx", "block"),
    (".ts", "block"),
    (".c", "block"),
    (".h", "block"),
    (".rs", "block"),
    (".py", "sharp"),
    (".cmake", "sharp"),
    ("cmakelists.txt", "sharp"),
    (".typed", "sharp")
)


def list_repository_files(repo_root: Path) -> List[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-co", "--exclude-standard"],
        cwd=repo_root,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(
            "git ls-files failed; ensure this script "
            "runs inside a git repository"
        )
    files: List[Path] = []
    for line in result.stdout.splitlines():
        stripped = line.strip()
        if stripped:
            files.append(repo_root / stripped)
    return files


def determine_style(path: Path) -> Optional[str]:
    lowered = path.name.lower()
    for suffix, style in SUFFIX_STYLE:
        if lowered.endswith(suffix):
            return style
    return None


def strip_block_comment(text: str) -> Tuple[str, bool]:
    working = text
    if working.startswith("/*"):
        end = working.find("*/")
        if end != -1:
            working = working[end + 2:]
            return working.lstrip("\r\n"), True
    if working.startswith("//"):
        lines = working.splitlines(keepends=True)
        idx = 0
        while idx < len(lines):
            stripped = lines[idx].lstrip()
            if stripped.startswith("//") or not stripped.strip():
                idx += 1
                continue
            break
        if idx > 0:
            remainder = "".join(lines[idx:])
            return remainder.lstrip("\r\n"), True
    return working, False


def strip_sharp_comment(text: str) -> Tuple[str, bool]:
    working = text
    if working.startswith(('"""', "'''")):
        quote = working[:3]
        end = working.find(quote, 3)
        if end != -1:
            working = working[end + 3:]
            return working.lstrip("\r\n"), True
    lines = working.splitlines(keepends=True)
    idx = 0
    while idx < len(lines):
        stripped = lines[idx].lstrip()
        if stripped.startswith("#") or not stripped.strip():
            idx += 1
            continue
        break
    if idx > 0:
        remainder = "".join(lines[idx:])
        return remainder.lstrip("\r\n"), True
    return working, False


def strip_existing_header(text: str, style: str) -> str:
    working = text.lstrip("\r\n")
    if not working:
        return working
    if style == "block":
        working, removed = strip_block_comment(working)
    else:
        working, removed = strip_sharp_comment(working)
    if removed:
        return working.lstrip("\r\n")
    return text.lstrip("\r\n")


def apply_license_header(path: Path, style: str) -> bool:
    try:
        original = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        print(f"Skipping (encoding issue): {path}")
        return False

    text = original
    bom = ""
    if text.startswith("\ufeff"):
        bom = "\ufeff"
        text = text[1:]

    shebang = ""
    if style == "sharp" and text.startswith("#!"):
        newline_idx = text.find("\n")
        if newline_idx == -1:
            newline_idx = len(text)
        shebang = text[
            : newline_idx + 1 if newline_idx < len(text) else newline_idx
        ]
        text = text[len(shebang):]

    header = HEADERS[style]

    preview = text
    if preview.startswith(header):
        return False

    body = strip_existing_header(text, style)
    body = body.lstrip("\r\n")
    new_content = bom + shebang + header + body

    if new_content == original:
        return False

    path.write_text(new_content, encoding="utf-8")
    return True


def gather_targets(repo_root: Path) -> List[Tuple[Path, str]]:
    targets: List[Tuple[Path, str]] = []
    for path in list_repository_files(repo_root):
        if not path.is_file():
            continue
        style = determine_style(path)
        if style:
            targets.append((path, style))
    targets.sort(key=lambda item: str(item[0].relative_to(repo_root)))
    return targets


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    targets = gather_targets(repo_root)

    updated: List[Path] = []
    skipped: List[Path] = []

    for path, style in targets:
        if apply_license_header(path, style):
            updated.append(path)
        else:
            skipped.append(path)

    if updated:
        print("Updated:")
        for path in updated:
            print(f"  {path.relative_to(repo_root)}")
    if skipped:
        print("Skipped (already compliant):")
        for path in skipped:
            print(f"  {path.relative_to(repo_root)}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
