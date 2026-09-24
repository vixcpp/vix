#!/usr/bin/env python3
"""Download README.md for every package in the vix registry index."""

import json
import os
import sys
import urllib.request
import urllib.error
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

REGISTRY_INDEX = os.path.expanduser("~/.vix/registry/index/index")
OUTPUT_DIR = Path(__file__).resolve().parent.parent / "registry"


def load_packages():
    """Read all package JSON files and return a list of (namespace, name, raw_url)."""
    packages = []
    for fname in sorted(os.listdir(REGISTRY_INDEX)):
        if not fname.endswith(".json"):
            continue
        fpath = os.path.join(REGISTRY_INDEX, fname)
        with open(fpath) as f:
            data = json.load(f)
        ns = data.get("namespace", "")
        name = data.get("name", "")
        repo_url = data.get("repo", {}).get("url", "")
        branch = data.get("repo", {}).get("defaultBranch", "main")
        if not repo_url:
            continue
        # https://github.com/owner/repo -> owner/repo
        parts = repo_url.rstrip("/").split("github.com/")
        if len(parts) != 2:
            continue
        owner_repo = parts[1]
        raw_url = f"https://raw.githubusercontent.com/{owner_repo}/{branch}/README.md"
        packages.append((ns, name, raw_url))
    return packages


def download_readme(ns, name, url):
    """Download a single README. Returns (ns, name, success, error_msg)."""
    dest_dir = OUTPUT_DIR / ns
    dest_dir.mkdir(parents=True, exist_ok=True)
    dest_file = dest_dir / f"{name}.md"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "vix-registry-docs/1.0"})
        with urllib.request.urlopen(req, timeout=15) as resp:
            content = resp.read()
        dest_file.write_bytes(content)
        return (ns, name, True, None)
    except (urllib.error.URLError, urllib.error.HTTPError, OSError) as e:
        return (ns, name, False, str(e))


def main():
    packages = load_packages()
    print(f"Found {len(packages)} packages in registry index")
    print(f"Output directory: {OUTPUT_DIR}\n")

    ok, fail = 0, 0
    failures = []

    with ThreadPoolExecutor(max_workers=10) as pool:
        futures = {
            pool.submit(download_readme, ns, name, url): (ns, name)
            for ns, name, url in packages
        }
        for future in as_completed(futures):
            ns, name, success, err = future.result()
            if success:
                ok += 1
                print(f"  OK  {ns}.{name}")
            else:
                fail += 1
                failures.append((ns, name, err))
                print(f" FAIL {ns}.{name} — {err}")

    print(f"\nDone: {ok} downloaded, {fail} failed, {ok + fail} total")
    if failures:
        print("\nFailed packages:")
        for ns, name, err in sorted(failures):
            print(f"  {ns}.{name}: {err}")
    return 0 if fail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
