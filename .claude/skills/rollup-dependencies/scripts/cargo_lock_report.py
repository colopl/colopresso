# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of colopresso
#
# Copyright (C) 2025-2026 COLOPL, Inc.
#
# Author: Go Kudo <g-kudo@colopl.co.jp>
# Developed with AI (LLM) code assistance. See `NOTICE` for details.

"""Report on the crates in a Cargo.lock for a dependency rollup.

* Crates whose version differs from a base git revision, with their crates.io
  publish time, so versions younger than the release-age window stand out.
* Known advisories (RustSec and others via OSV) for every registry crate in the
  current lock file.

Only public crate names and versions are sent, to crates.io and api.osv.dev.
Standard library only; run it with `python3 -I`.

Exit status: 0 when nothing was found, 1 when a crate is younger than the window
or has an advisory, 2 when a lookup failed and the report is incomplete.
"""

import argparse
import datetime
import json
import re
import subprocess
import sys
import urllib.error
import urllib.request

USER_AGENT = "colopresso-rollup-dependencies"
PACKAGE_RE = re.compile(r'\[\[package\]\]\nname = "([^"]+)"\nversion = "([^"]+)"\nsource = "registry\+')


def registry_packages(text):
    return set(PACKAGE_RE.findall(text))


def http_json(url, payload=None):
    data = json.dumps(payload).encode() if payload is not None else None
    headers = {"User-Agent": USER_AGENT}
    if data is not None:
        headers["Content-Type"] = "application/json"
    request = urllib.request.Request(url, data=data, headers=headers)
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)


def changed_crates(lock_path, base, now, min_age_hours):
    with open(lock_path, encoding="utf-8") as handle:
        current = registry_packages(handle.read())
    result = subprocess.run(["git", "show", f"{base}:{lock_path}"], capture_output=True, text=True)
    if result.returncode != 0:
        print(f"Cannot read {lock_path} at {base}: {result.stderr.strip()}", file=sys.stderr)
        return current, 0, 1
    previous = registry_packages(result.stdout)

    print(f"## Changed crates ({base} -> working tree)\n")
    removed = sorted(previous - current)
    added = sorted(current - previous)
    if not added and not removed:
        print("No changes.\n")
    young = errors = 0
    for name, version in added:
        try:
            info = http_json(f"https://crates.io/api/v1/crates/{name}/{version}")["version"]
        except (urllib.error.URLError, KeyError, json.JSONDecodeError) as error:
            print(f"- {name} {version}: publish time unavailable ({error})")
            errors += 1
            continue
        created = datetime.datetime.fromisoformat(info["created_at"].replace("Z", "+00:00"))
        age = (now - created).total_seconds() / 3600
        flags = []
        if age < min_age_hours:
            flags.append(f"YOUNGER THAN {min_age_hours}h")
            young += 1
        if info.get("yanked"):
            flags.append("YANKED")
        suffix = f"  <-- {', '.join(flags)}" if flags else ""
        print(f"- {name} {version}: published {info['created_at'][:16]}Z ({age:.0f}h ago){suffix}")
    for name, version in removed:
        if not any(n == name for n, _ in added):
            print(f"- {name} {version}: removed")
    print()
    return current, young, errors


def advisories(packages):
    print("## Advisories (OSV)\n")
    ordered = sorted(packages)
    query = {"queries": [{"package": {"name": n, "ecosystem": "crates.io"}, "version": v} for n, v in ordered]}
    try:
        results = http_json("https://api.osv.dev/v1/querybatch", query)["results"]
    except (urllib.error.URLError, KeyError, json.JSONDecodeError) as error:
        print(f"OSV query failed: {error}\n")
        return 0, 1
    found = errors = 0
    for (name, version), result in zip(ordered, results):
        for vuln in result.get("vulns", []):
            found += 1
            try:
                detail = http_json(f"https://api.osv.dev/v1/vulns/{vuln['id']}")
            except (urllib.error.URLError, json.JSONDecodeError):
                detail = {}
                errors += 1
            fixed = sorted(
                event["fixed"]
                for affected in detail.get("affected", [])
                for ranges in affected.get("ranges", [])
                for event in ranges.get("events", [])
                if "fixed" in event
            )
            print(f"- {name} {version}: {vuln['id']} {detail.get('summary', '')} (fixed: {', '.join(fixed) or 'none'})")
    if not found:
        print(f"No known advisories for {len(ordered)} registry crates.")
    print()
    return found, errors


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--lock", default="library/pngx_bridge/Cargo.lock", help="Cargo.lock path relative to the repository root")
    parser.add_argument("--base", default="origin/main", help="git revision to compare against")
    parser.add_argument("--min-age-hours", type=float, default=24.0, help="release-age window in hours")
    args = parser.parse_args()

    now = datetime.datetime.now(datetime.timezone.utc)
    print(f"Checked at {now.isoformat(timespec='minutes')}\n")
    packages, young, lookup_errors = changed_crates(args.lock, args.base, now, args.min_age_hours)
    found, advisory_errors = advisories(packages)
    errors = lookup_errors + advisory_errors
    print(f"Summary: {young} crate(s) younger than {args.min_age_hours:g}h, {found} advisory match(es), {errors} failed lookup(s).")
    if errors:
        print("The report is incomplete; rerun it before relying on the result.", file=sys.stderr)
        return 2
    return 1 if young or found else 0


if __name__ == "__main__":
    sys.exit(main())
