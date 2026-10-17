#!/usr/bin/env python3
# Copyright (C) 2025 NAVRobotec Pvt Ltd
# Author: Ragnar Vallhala
# SPDX-License-Identifier: Apache-2.0
"""Derive an out-of-tree board from one that already exists.

A board description is a statement about how a PCB is wired, so a different board
owns its own statement rather than inheriting one. That is a deliberate choice --
Kconfig gates every default on `if BOARD_<NAME>`, and within a choice only one board
is ever selected, so there is no condition under which a derived board would pick up
its base's values. Making inheritance work would mean rewriting every base condition
onto an alias symbol, in every board, which buys a thin override file at the cost of
a far less readable description.

So adaptation is a copy, done once, by a tool that gets the renaming right:

  tools/derive_board.py --from navixdev --name navixdev_r4 --out ../my-boards/r4

Then edit what differs and build with it:

  cmake -B build -DNAVHAL_BOARD_DIR=../my-boards/r4 ...
  # with CONFIG_BOARD_NAVIXDEV_R4=y in .config

What you get is the base's full description with its symbol renamed throughout, its
Kconfig.choice entry, and its linker script. What you own afterwards is a copy: the
base moving on will not reach you, which is the honest trade for not inheriting.
"""

import argparse
import re
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--from", dest="base", required=True,
                    help="in-tree board to copy, e.g. navixdev")
    ap.add_argument("--name", required=True,
                    help="slug for the new board, e.g. navixdev_r4")
    ap.add_argument("--out", required=True, help="directory to create")
    ap.add_argument("--force", action="store_true", help="overwrite --out if it exists")
    a = ap.parse_args()

    base_dir = REPO / "src" / "board" / a.base
    if not base_dir.is_dir():
        print(f"error: no such in-tree board: {a.base}", file=sys.stderr)
        print("  available:", ", ".join(sorted(
            p.name for p in (REPO / "src" / "board").iterdir() if p.is_dir())),
            file=sys.stderr)
        return 2

    slug = a.name.lower()
    if not re.fullmatch(r"[a-z0-9_]+", slug):
        print(f"error: --name must be lower-case letters, digits and underscores: {a.name}",
              file=sys.stderr)
        return 2

    out = Path(a.out).resolve()
    if out.exists():
        if not a.force:
            print(f"error: {out} exists (use --force to overwrite)", file=sys.stderr)
            return 2
        shutil.rmtree(out)
    out.mkdir(parents=True)

    old_sym = f"BOARD_{a.base.upper()}"
    new_sym = f"BOARD_{slug.upper()}"

    for name in ("Kconfig", "Kconfig.choice"):
        src = base_dir / name
        if not src.exists():
            print(f"error: {src} is missing; {a.base} cannot be derived from",
                  file=sys.stderr)
            return 2
        text = src.read_text()
        # The symbol, every `if BOARD_<BASE>` condition, and the board's own name
        # string. Word-boundary so BOARD_NAVIXDEV does not also rewrite
        # BOARD_NAVIXDEV_R4 on a second run.
        text = re.sub(rf'\b{re.escape(old_sym)}\b', new_sym, text)
        text = text.replace(f'"{a.base}"', f'"{slug}"')
        header = (f"# Derived from the in-tree board '{a.base}' by tools/derive_board.py.\n"
                  f"# This is a copy, not an overlay: changes to {a.base} do not reach it.\n"
                  f"# Edit what differs on this hardware and leave the rest alone.\n\n")
        (out / name).write_text(header + text)

    copied = []
    for extra in ("linker.ld", "startup.s"):
        src = base_dir / extra
        if src.exists():
            shutil.copy2(src, out / extra)
            copied.append(extra)
    for sub in ("boot",):
        src = base_dir / sub
        if src.is_dir():
            shutil.copytree(src, out / sub)
            copied.append(sub + "/")

    print(f"  derived {slug} from {a.base} in {out}")
    print(f"  symbol: {new_sym}")
    print(f"  files:  Kconfig, Kconfig.choice" + ("".join(f", {c}" for c in copied)))
    print()
    print("  build it with:")
    print(f"    echo CONFIG_{new_sym}=y >> .config")
    print(f"    cmake -B build -DNAVHAL_BOARD_DIR={out} \\")
    print("          -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/<arch>-toolchain.cmake")
    return 0


if __name__ == "__main__":
    sys.exit(main())
