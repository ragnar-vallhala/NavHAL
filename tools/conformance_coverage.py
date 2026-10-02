#!/usr/bin/env python3
# Copyright (C) 2025 NAVRobotec Pvt Ltd
# Author: Ragnar Vallhala
# SPDX-License-Identifier: Apache-2.0
#
# What of the public HAL surface the conformance suite actually exercises.
#
#     tools/conformance_coverage.py            # the numbers + what is missing
#     tools/conformance_coverage.py --gate     # and fail on an undeclared gap
#     tools/conformance_coverage.py --list     # every entry point and its state
#
# A "public entry point" is a function declared in include/common/*.h whose name
# starts with hal_. That is the surface a port has to implement and a consumer is
# allowed to call; the inline hot paths under include/port/ are deliberately out,
# since they are a vendor's own accessors rather than a shared contract.
#
# "Covered" means the conformance suite calls it. Another tier calling it does
# not count: the question this answers is whether a NEW PORT would be caught
# getting it wrong, and only the portable conformance suite runs everywhere.
#
# An entry point that cannot be covered is declared in uncovered.txt with its
# reason, next to the suite. --gate fails only on a gap that is in neither --
# not on the count, which would just invite picking a threshold that passes.
#
# The vendor half of the same question is checked here too: every ops table a
# port has to fill in must have a completeness case in test_vtable.c. That suite
# walks a table's bytes, so a table that GROWS an entry is covered the day it
# grows -- but a table that is newly ADDED is covered only when someone writes
# its case, and nothing noticed if they did not.

import argparse
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PUBLIC_DIR = os.path.join(REPO, "include", "common")
SUITE = os.path.join(REPO, "tests", "portable", "conformance", "test_conformance.c")
EXCLUDES = os.path.join(REPO, "tests", "portable", "conformance", "uncovered.txt")
OPS_DIR = os.path.join(REPO, "include", "internal")
VTABLE = os.path.join(REPO, "tests", "portable", "conformance", "test_vtable.c")

_COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)
# A declaration, not a definition: `<type> hal_name(...);` with no body. The
# negative lookbehind for `(*` keeps callback typedefs out -- those name a
# pointer type, not an entry point a port implements.
_DECL = re.compile(
    r"(?<![\w*])(?<!\(\*)(hal_[a-z0-9_]+)\s*\([^;{)]*\)?[^;{]*;", re.S)


# A function-like macro is not an entry point, and leaving it in costs more than
# a false positive: `#define hal_uart_print(uart, val) _Generic(...)` ends without
# a semicolon, so _DECL -- which spans newlines -- ran from the macro name to the
# next `;` anywhere below, swallowing the real declarations in between and
# reporting the macro in their place. Directives go before matching.
_DIRECTIVE = re.compile(r"^[ \t]*#(?:[^\n\\]|\\\n|\\[^\n])*", re.M)


def strip_comments(text):
    return _COMMENT.sub(" ", text)


def strip_directives(text):
    return _DIRECTIVE.sub(" ", text)


def public_entry_points():
    """{symbol: header} for every hal_* declared in include/common/."""
    out = {}
    for fn in sorted(os.listdir(PUBLIC_DIR)):
        if not fn.endswith(".h"):
            continue
        path = os.path.join(PUBLIC_DIR, fn)
        with open(path, errors="replace") as f:
            text = strip_comments(f.read())
        # Drop inline definitions before matching: a body means the port does
        # not supply it, so it is not part of the surface a port implements.
        text = strip_directives(text)
        text = re.sub(r"\bstatic\s+inline\b[^;{]*\{", " ", text)
        for m in _DECL.finditer(text):
            out.setdefault(m.group(1), fn)
    return out


def called_symbols(path):
    with open(path, errors="replace") as f:
        text = strip_comments(f.read())
    return set(re.findall(r"\b(hal_[a-z0-9_]+)\s*\(", text))


def declared_exclusions():
    """{symbol: reason} from uncovered.txt (`symbol: reason`, # comments)."""
    out = {}
    if not os.path.exists(EXCLUDES):
        return out
    with open(EXCLUDES, errors="replace") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            sym, _, reason = line.partition(":")
            out[sym.strip()] = reason.strip()
    return out


def vendor_table_gaps():
    """Ops tables declared under include/internal/ with no completeness case."""
    declared = {}
    if not os.path.isdir(OPS_DIR):
        return {}
    for fn in sorted(os.listdir(OPS_DIR)):
        if not fn.endswith("_ops.h"):
            continue
        path = os.path.join(OPS_DIR, fn)
        with open(path, errors="replace") as f:
            text = strip_comments(f.read())
        for sym in re.findall(r"\b(_hal_[a-z0-9_]+_ops)\b", text):
            declared.setdefault(sym, fn)
    if not os.path.exists(VTABLE):
        return declared
    with open(VTABLE, errors="replace") as f:
        asserted = set(re.findall(r"ASSERT_TABLE_COMPLETE\(\s*(_hal_[a-z0-9_]+_ops)\s*\)",
                                  strip_comments(f.read())))
    return {k: v for k, v in declared.items() if k not in asserted}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--gate", action="store_true",
                    help="exit non-zero on an entry point that is neither "
                         "covered nor declared in uncovered.txt")
    ap.add_argument("--list", action="store_true",
                    help="print every entry point and its state")
    args = ap.parse_args()

    public = public_entry_points()
    covered = called_symbols(SUITE) & set(public)
    excluded = declared_exclusions()
    gaps = sorted(set(public) - covered - set(excluded))
    stale = sorted(set(excluded) & covered)

    declared = set(excluded) & set(public)
    debt = {s for s in declared if excluded[s].startswith("debt:")}
    permanent = declared - debt
    table_gaps = vendor_table_gaps()

    total = len(public)
    print(f"public entry points : {total}")
    print(f"conformance-covered : {len(covered)}")
    print(f"cannot be covered   : {len(permanent)}")
    print(f"known debt          : {len(debt)}")
    print(f"undeclared gaps     : {len(gaps)}")
    print(f"ops tables unchecked: {len(table_gaps)}")

    if args.list:
        for sym in sorted(public):
            if sym in covered:
                state = "covered"
            elif sym in excluded:
                state = f"declared  ({excluded[sym]})"
            else:
                state = "GAP"
            print(f"  {sym:42} {public[sym]:24} {state}")

    if gaps:
        print("\nneither covered nor declared:")
        for sym in gaps:
            print(f"  {sym:42} {public[sym]}")
        print("\nAdd a conformance case, or a line to "
              "tests/portable/conformance/uncovered.txt saying why it cannot "
              "have one.")

    # A symbol listed as uncoverable that the suite now calls is a stale
    # exclusion: the reason it named has stopped being true, and leaving it
    # there would excuse the next gap for free.
    if stale:
        print("\ndeclared uncovered but the suite calls them "
              "(drop these lines):")
        for sym in stale:
            print(f"  {sym}")

    if table_gaps:
        print("\nops tables with no completeness case in test_vtable.c:")
        for sym, hdr in sorted(table_gaps.items()):
            print(f"  {sym:42} {hdr}")
        print("\nAdd ASSERT_TABLE_COMPLETE for it, or a port can ship the "
              "table half-filled and link fine.")

    if args.gate and (gaps or stale or table_gaps):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
