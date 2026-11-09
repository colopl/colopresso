# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

"""Check the files written by smoke.sh.

Each folder scenario must have one output per input PNG whose dimensions
match the input. PNG outputs are fully parsed (chunk CRCs, zlib stream and
the expected amount of image data); WebP and AVIF outputs are checked from
their container headers. Where `sips` exists (macOS), every output is also
decoded. The drop scenario must have produced one WebP of the right size and
a ZIP with one WebP per dropped file. Exits 1 when anything is wrong.
"""

import os
import shutil
import struct
import subprocess
import sys
import zipfile
import zlib

PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
CHANNELS = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}
COLOUR_TYPES = {0: "grey", 2: "rgb", 3: "palette", 4: "grey+alpha", 6: "rgba"}
ADAM7 = ((0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4), (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2))
FOLDER_SCENARIOS = {
    "webp": ".webp",
    "avif": ".avif",
    "pngx-palette256": "_optimized.png",
    "pngx-limited": "_optimized.png",
    "pngx-reduced": "_optimized.png",
}
DROP_ZIP_ENTRIES = {"example.webp", "128x128.webp", "banding.webp"}


class Invalid(Exception):
    pass


def png_info(data):
    """Return (width, height, colour type), validating the whole PNG."""
    if data[:8] != PNG_SIGNATURE:
        raise Invalid("not a PNG signature")
    offset, header, idat, ended = 8, None, b"", False
    while offset + 12 <= len(data):
        (length,) = struct.unpack(">I", data[offset : offset + 4])
        kind = data[offset + 4 : offset + 8]
        body = data[offset + 8 : offset + 8 + length]
        (crc,) = struct.unpack(">I", data[offset + 8 + length : offset + 12 + length])
        if len(body) != length or zlib.crc32(kind + body) != crc:
            raise Invalid(f"bad {kind!r} chunk")
        if kind == b"IHDR":
            header = struct.unpack(">IIBBBBB", body)
        elif kind == b"IDAT":
            idat += body
        elif kind == b"IEND":
            ended = True
            break
        offset += 12 + length
    if header is None or not ended or not idat:
        raise Invalid("missing IHDR, IDAT or IEND")
    width, height, depth, colour, _, _, interlace = header
    bits = depth * CHANNELS[colour]

    def raw_size(w, h):
        return 0 if w == 0 or h == 0 else h * (1 + (w * bits + 7) // 8)

    if interlace:
        expected = sum(raw_size((width - x + dx - 1) // dx, (height - y + dy - 1) // dy) for x, y, dx, dy in ADAM7)
    else:
        expected = raw_size(width, height)
    if len(zlib.decompress(idat)) != expected:
        raise Invalid("image data has the wrong length")
    return width, height, colour


def webp_size(data):
    if data[:4] != b"RIFF" or data[8:12] != b"WEBP":
        raise Invalid("not a WebP container")
    if struct.unpack("<I", data[4:8])[0] + 8 != len(data):
        raise Invalid("RIFF size does not match the file")
    chunk = data[12:16]
    if chunk == b"VP8X":
        return 1 + int.from_bytes(data[24:27], "little"), 1 + int.from_bytes(data[27:30], "little")
    if chunk == b"VP8L":
        if data[20] != 0x2F:
            raise Invalid("bad VP8L signature")
        bits = int.from_bytes(data[21:25], "little")
        return (bits & 0x3FFF) + 1, ((bits >> 14) & 0x3FFF) + 1
    if chunk == b"VP8 ":
        if data[23:26] != b"\x9d\x01\x2a":
            raise Invalid("bad VP8 start code")
        return struct.unpack("<H", data[26:28])[0] & 0x3FFF, struct.unpack("<H", data[28:30])[0] & 0x3FFF
    raise Invalid(f"unknown WebP chunk {chunk!r}")


def avif_size(data):
    if data[4:8] != b"ftyp" or b"avif" not in data[8 : struct.unpack(">I", data[:4])[0]]:
        raise Invalid("not an AVIF file")
    index = data.find(b"ispe")
    if index < 0:
        raise Invalid("no ispe box")
    return struct.unpack(">II", data[index + 8 : index + 16])


def output_size(path, data):
    if path.endswith(".webp"):
        return webp_size(data), ""
    if path.endswith(".avif"):
        return avif_size(data), ""
    width, height, colour = png_info(data)
    return (width, height), f" colour={COLOUR_TYPES.get(colour, colour)}"


def decoded_size(path):
    if shutil.which("sips") is None:
        return None
    result = subprocess.run(["sips", "-g", "pixelWidth", "-g", "pixelHeight", path], capture_output=True, text=True)
    values = [line.split(":", 1)[1].strip() for line in result.stdout.splitlines() if "pixel" in line]
    return tuple(int(v) for v in values) if result.returncode == 0 and len(values) == 2 else "unreadable"


def check(label, path, expected):
    with open(path, "rb") as handle:
        data = handle.read()
    try:
        size, detail = output_size(path, data)
    except (Invalid, zlib.error, struct.error, IndexError, KeyError) as error:
        print(f"BAD {label} {len(data)}B: {error}")
        return 1
    decoded = decoded_size(path)
    valid = size == expected and decoded in (None, expected)
    decoded_text = "" if decoded is None else f" decoded={decoded}"
    print(f"{'OK ' if valid else 'BAD'} {label} {len(data)}B size={size}{detail}{decoded_text}")
    return 0 if valid else 1


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    failures = 0
    sizes = {}
    for scenario, suffix in FOLDER_SCENARIOS.items():
        folder = os.path.join(root, scenario)
        for name in sorted(f for f in os.listdir(folder) if f.endswith(".png") and not f.endswith("_optimized.png")):
            with open(os.path.join(folder, name), "rb") as handle:
                sizes[name] = png_info(handle.read())[:2]
            output = os.path.join(folder, name[:-4] + suffix)
            if not os.path.exists(output):
                print(f"MISSING {scenario}/{os.path.basename(output)}")
                failures += 1
                continue
            failures += check(f"{scenario}/{os.path.basename(output)}", output, sizes[name])

    single = os.path.join(root, "drop", "example-single.webp")
    if os.path.exists(single):
        failures += check("drop/example-single.webp", single, sizes.get("example.png"))
    else:
        print("MISSING drop/example-single.webp")
        failures += 1
    archive = os.path.join(root, "drop", "batch.zip")
    if os.path.exists(archive):
        with zipfile.ZipFile(archive) as bundle:
            names = set(bundle.namelist())
            entries_ok = names == DROP_ZIP_ENTRIES
            for name in sorted(names & DROP_ZIP_ENTRIES):
                try:
                    entry_ok = webp_size(bundle.read(name)) == sizes.get(name[:-5] + ".png")
                except (Invalid, struct.error, IndexError):
                    entry_ok = False
                entries_ok = entries_ok and entry_ok
        print(f"{'OK ' if entries_ok else 'BAD'} drop/batch.zip entries={sorted(names)}")
        failures += 0 if entries_ok else 1
    else:
        print("MISSING drop/batch.zip")
        failures += 1

    print("ALL OK" if failures == 0 else f"{failures} problem(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
