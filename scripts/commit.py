#!/usr/bin/env python3
"""Bump FIRMWARE_PATCH (if needed) and create a commit.

Usage:
  python3 scripts/commit.py "commit message"           # commit only
  python3 scripts/commit.py "commit message" --release # commit + tag as release
"""

import re, subprocess, os, sys

HEADER = os.path.join(os.path.dirname(__file__), "..", "src", "pairing_config.h")

def read_define(text, name, default=None):
    m = re.search(rf"#define\s+{name}\s+(\d+)", text)
    return int(m.group(1)) if m else default

def get_tagged_config_id():
    """Read PAIRING_CONFIG_ID from the latest release-0x* tag."""
    result = subprocess.run(
        ["git", "tag", "-l", "release-0x*", "--sort=-version:refname"],
        capture_output=True, text=True
    )
    tags = result.stdout.strip().splitlines()
    if not tags:
        return None
    latest_tag = tags[0]
    result = subprocess.run(
        ["git", "show", f"{latest_tag}:src/pairing_config.h"],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        return None
    return read_define(result.stdout, "PAIRING_CONFIG_ID")

def has_staged_changes():
    result = subprocess.run(["git", "diff", "--cached", "--quiet"], capture_output=True)
    return result.returncode != 0

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 scripts/commit.py \"commit message\" [--release]", file=sys.stderr)
        sys.exit(1)

    message = sys.argv[1]
    release = "--release" in sys.argv[2:]

    if not has_staged_changes():
        print("No staged changes. Stage files with git add before running.", file=sys.stderr)
        sys.exit(1)

    with open(HEADER) as f:
        text = f.read()

    current_id = read_define(text, "PAIRING_CONFIG_ID")
    current_patch = read_define(text, "FIRMWARE_PATCH", 0)
    tagged_id = get_tagged_config_id()

    if tagged_id is not None and current_id == tagged_id:
        # Same PAIRING_CONFIG_ID — bump patch
        new_patch = current_patch + 1
        new_text = re.sub(
            r"(#define\s+FIRMWARE_PATCH\s+)\d+",
            rf"\g<1>{new_patch}",
            text
        )
        with open(HEADER, "w") as f:
            f.write(new_text)
        subprocess.run(["git", "add", HEADER], check=True)
        print(f"FIRMWARE_PATCH bumped: {current_patch} → {new_patch}")
    else:
        new_patch = current_patch
        print(f"PAIRING_CONFIG_ID changed ({tagged_id} → {current_id}), patch stays at {current_patch}")

    # Read final values for version summary
    major = read_define(text, "FIRMWARE_MAJOR", 1)
    version = f"{major}.{current_id}.{new_patch}"

    subprocess.run(["git", "commit", "-m", message], check=True)
    print(f"Committed — firmware version: {version}")

    if release:
        tag = f"release-0x{current_id:02x}"
        subprocess.run(["git", "tag", "-f", tag], check=True)
        print(f"Tagged: {tag}")

if __name__ == "__main__":
    main()
