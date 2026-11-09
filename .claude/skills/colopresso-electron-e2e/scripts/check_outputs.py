# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

"""Check the files written by smoke.sh.

Each folder scenario must have one output per input PNG, with the right file
signature. PNG outputs report their colour type, and on macOS every output is
decoded with `sips` to confirm it is a readable image of the input's size.
The drop scenario must have produced one WebP and a ZIP with one WebP per
dropped file. Exits 1 when anything is missing or invalid.
"""

import os
import shutil
import struct
import subprocess
import sys
import zipfile

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
COLOUR_TYPES = {0: "grey", 2: "rgb", 3: "palette", 4: "grey+alpha", 6: "rgba"}
FOLDER_SCENARIOS = {
    "webp": ".webp",
    "avif": ".avif",
    "pngx-palette256": "_optimized.png",
    "pngx-limited": "_optimized.png",
    "pngx-reduced": "_optimized.png",
}
DROP_ZIP_ENTRIES = {"example.webp", "128x128.webp", "banding.webp"}


def png_size(header):
    width, height = struct.unpack(">II", header[16:24])
    return width, height


def signature_ok(path, header):
    if path.endswith(".webp"):
        return header[:4] == b"RIFF" and header[8:12] == b"WEBP"
    if path.endswith(".avif"):
        return header[4:8] == b"ftyp" and b"avif" in header[8:32]
    return header[:8] == PNG_SIGNATURE


def decoded_size(path):
    if shutil.which("sips") is None:
        return None
    result = subprocess.run(["sips", "-g", "pixelWidth", "-g", "pixelHeight", path], capture_output=True, text=True)
    if result.returncode != 0:
        return "unreadable"
    values = [line.split(":", 1)[1].strip() for line in result.stdout.splitlines() if "pixel" in line]
    return tuple(int(v) for v in values) if len(values) == 2 else "unreadable"


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    failures = 0
    for scenario, suffix in FOLDER_SCENARIOS.items():
        folder = os.path.join(root, scenario)
        inputs = sorted(f for f in os.listdir(folder) if f.endswith(".png") and not f.endswith("_optimized.png"))
        for name in inputs:
            source = os.path.join(folder, name)
            output = os.path.join(folder, name[:-4] + suffix)
            if not os.path.exists(output):
                print(f"MISSING {scenario}/{os.path.basename(output)}")
                failures += 1
                continue
            with open(source, "rb") as handle:
                expected = png_size(handle.read(32))
            with open(output, "rb") as handle:
                header = handle.read(64)
            detail = ""
            if output.endswith(".png"):
                colour_type = header[25]
                detail = f" colour={COLOUR_TYPES.get(colour_type, colour_type)}"
            decoded = decoded_size(output)
            valid = signature_ok(output, header) and decoded in (None, expected)
            failures += 0 if valid else 1
            decoded_text = "" if decoded is None else f" decoded={decoded}"
            print(f"{'OK ' if valid else 'BAD'} {scenario}/{os.path.basename(output)} {os.path.getsize(output)}B{detail}{decoded_text}")

    drop = os.path.join(root, "drop")
    single = os.path.join(drop, "example-single.webp")
    if os.path.exists(single):
        with open(single, "rb") as handle:
            valid = signature_ok(single, handle.read(64))
        print(f"{'OK ' if valid else 'BAD'} drop/example-single.webp {os.path.getsize(single)}B")
        failures += 0 if valid else 1
    else:
        print("MISSING drop/example-single.webp")
        failures += 1
    archive = os.path.join(drop, "batch.zip")
    if os.path.exists(archive):
        with zipfile.ZipFile(archive) as bundle:
            names = set(bundle.namelist())
        valid = names == DROP_ZIP_ENTRIES
        print(f"{'OK ' if valid else 'BAD'} drop/batch.zip entries={sorted(names)}")
        failures += 0 if valid else 1
    else:
        print("MISSING drop/batch.zip")
        failures += 1

    print("ALL OK" if failures == 0 else f"{failures} problem(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
