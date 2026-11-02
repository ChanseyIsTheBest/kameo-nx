#!/usr/bin/env python3
"""A test config with some keys changed, for one emulator experiment (kameo-nx, 2026-11-03).

    python tools/emu_toml_with.py out/emu/emu_verify.toml out/emu/x.toml banjo_noncpu_enhancements=false [key=value ...]

A key already in the file is changed where it stands; a new one goes before the first [section], where it is still
top-level. Lines are "key = value"; values are written as given.
"""
import re
import sys


def main():
    source, target, pairs = sys.argv[1], sys.argv[2], sys.argv[3:]
    lines = open(source, encoding="utf-8").read().splitlines()
    wanted = dict(p.split("=", 1) for p in pairs)
    first_section = next((i for i, l in enumerate(lines) if l.startswith("[")), len(lines))
    for key, value in wanted.items():
        pattern = re.compile(r"^\s*" + re.escape(key) + r"\s*=")
        at = next((i for i, l in enumerate(lines[:first_section]) if pattern.match(l)), None)
        if at is not None:
            print(f"changed: {lines[at][:70]} -> {value}")
            lines[at] = f"{key} = {value}"
        else:
            print(f"added: {key} = {value}")
            lines.insert(first_section, f"{key} = {value}")
            first_section += 1
    open(target, "w", encoding="utf-8", newline="\n").write("\n".join(lines) + "\n")


if __name__ == "__main__":
    main()
