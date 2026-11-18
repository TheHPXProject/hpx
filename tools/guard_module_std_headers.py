#!/usr/bin/env python3
#
# Copyright (c) 2026 Mohammad Izaan
#
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

"""
Helper script to apply include guards around standard library #include
directives in HPX module headers for C++ module BMI compilation.

Standard library includes in module headers are enclosed in:
#if defined(HPX_HAVE_TEXTUAL_STD_INCLUDES)
#include <vector>
#endif

Macro-providing headers (<version>, <cassert>, <cstdint>, etc.) are left alone
so that their macros remain available in the Global Module Fragment.
"""

import argparse
import os
import re
import sys


HEADER_DEF_RE = re.compile(
    r'HPX_STD_HEADER\(\s*"([^"]+)",\s*"([^"]*)",\s*([01])\)'
)


def load_standard_headers():
    """Load standard library headers and macro headers from standard_headers.inc."""
    inc_file = os.path.normpath(
        os.path.join(
            os.path.dirname(__file__), "..", "cmake", "standard_headers.inc"
        )
    )
    headers = set()
    macro_headers = set()
    with open(inc_file, "r", encoding="utf-8") as f:
        content = f.read()
    for m in HEADER_DEF_RE.finditer(content):
        name, guard, is_macro = m.groups()
        headers.add(name)
        if is_macro == "1":
            macro_headers.add(name)
    return headers, macro_headers


STANDARD_HEADERS, MACRO_HEADERS = load_standard_headers()

GUARD_OPEN = "#if defined(HPX_HAVE_TEXTUAL_STD_INCLUDES)\n"
GUARD_CLOSE = "#endif\n"

GUARD_RE = re.compile(
    r"^\s*#\s*(?:if\s+(?:defined\s*\(\s*HPX_HAVE_TEXTUAL_STD_INCLUDES\s*\)|"
    r"HPX_HAVE_TEXTUAL_STD_INCLUDES\b|"
    r"!\s*\(\s*defined\s*\(\s*HPX_COMPILE_BMI\s*\)\s*&&"
    r"\s*defined\s*\(\s*HPX_HAVE_CXX_MODULE_STD\s*\)\s*\)|"
    r"!\s*\(\s*defined\s*\(\s*HPX_HAVE_CXX_MODULE_STD\s*\)\s*&&"
    r"\s*defined\s*\(\s*HPX_COMPILE_BMI\s*\)\s*\)|"
    r"!defined\s*\(\s*HPX_COMPILE_BMI\s*\)\s*\|\|"
    r"\s*!defined\s*\(\s*HPX_HAVE_CXX_MODULE_STD\s*\)|"
    r"!defined\s*\(\s*HPX_HAVE_CXX_MODULE_STD\s*\)\s*\|\|"
    r"\s*!defined\s*\(\s*HPX_COMPILE_BMI\s*\))|"
    r"ifdef\s+HPX_HAVE_TEXTUAL_STD_INCLUDES\b)\s*(?:(?://.*|/\*.*?\*/)\s*)*$"
)
IF_RE = re.compile(r"^\s*#\s*(if|ifdef|ifndef)\b")
ELIF_ELSE_RE = re.compile(r"^\s*#\s*(elif|elifdef|elifndef|else)\b")
ENDIF_RE = re.compile(r"^\s*#\s*endif\b")
INCLUDE_RE = re.compile(r"^\s*#\s*include\s*<([a-zA-Z0-9_/.]+)>(.*)$")
CONFIG_RE = re.compile(r"^\s*#\s*include\s*<hpx/config\.hpp>")


def strip_line_comments(
    line, in_block_comment, in_line_comment, in_raw_string, raw_delimiter
):
    """Strip comments from line while tracking multi-line comment & string states."""
    if in_line_comment:
        stripped = line.rstrip("\r\n")
        if stripped.endswith("\\"):
            return "", in_block_comment, True, in_raw_string, raw_delimiter
        return "", in_block_comment, False, in_raw_string, raw_delimiter

    clean = []
    i = 0
    n = len(line)
    in_string = False
    in_char = False

    while i < n:
        if in_block_comment:
            if i + 1 < n and line[i] == "*" and line[i + 1] == "/":
                in_block_comment = False
                i += 2
                if not clean or clean[-1] != " ":
                    clean.append(" ")
            else:
                i += 1
        elif in_raw_string:
            end_marker = ")" + raw_delimiter + '"'
            if line[i : i + len(end_marker)] == end_marker:
                in_raw_string = False
                raw_delimiter = ""
                i += len(end_marker)
            else:
                i += 1
        elif in_string:
            if line[i] == "\\" and i + 1 < n:
                clean.append(line[i : i + 2])
                i += 2
            elif line[i] == '"':
                in_string = False
                clean.append(line[i])
                i += 1
            else:
                clean.append(line[i])
                i += 1
        elif in_char:
            if line[i] == "\\" and i + 1 < n:
                clean.append(line[i : i + 2])
                i += 2
            elif line[i] == "'":
                in_char = False
                clean.append(line[i])
                i += 1
            else:
                clean.append(line[i])
                i += 1
        else:
            if i + 1 < n and line[i] == "/" and line[i + 1] == "*":
                in_block_comment = True
                i += 2
            elif i + 1 < n and line[i] == "/" and line[i + 1] == "/":
                stripped = line.rstrip("\r\n")
                if stripped.endswith("\\"):
                    in_line_comment = True
                break
            elif i + 1 < n and line[i] == "R" and line[i + 1] == '"':
                paren_pos = line.find("(", i + 2)
                if paren_pos != -1:
                    raw_delimiter = line[i + 2 : paren_pos]
                    in_raw_string = True
                    i = paren_pos + 1
                else:
                    clean.append(line[i])
                    i += 1
            elif line[i] == '"':
                in_string = True
                clean.append(line[i])
                i += 1
            elif line[i] == "'":
                # In C++, an apostrophe between digits is a numeric digit separator,
                # not the start of a character literal (e.g. 1'000).
                is_digit_separator = (
                    clean
                    and (clean[-1].isalnum() or clean[-1] in "._")
                    and (
                        i + 1 < n
                        and (line[i + 1].isalnum() or line[i + 1] == "'")
                    )
                )
                if is_digit_separator:
                    clean.append(line[i])
                    i += 1
                else:
                    in_char = True
                    clean.append(line[i])
                    i += 1
            else:
                clean.append(line[i])
                i += 1

    return (
        "".join(clean).strip(),
        in_block_comment,
        in_line_comment,
        in_raw_string,
        raw_delimiter,
    )


def guard_file(filepath):
    """Apply include guards around standard library includes in filepath."""
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    lines = content.splitlines(keepends=True)

    unguarded_indices = []
    has_config_include = False
    in_block_comment = False
    in_line_comment = False
    in_raw_string = False
    raw_delimiter = ""
    if_stack = []

    for i, line in enumerate(lines):
        (
            clean,
            in_block_comment,
            in_line_comment,
            in_raw_string,
            raw_delimiter,
        ) = strip_line_comments(
            line,
            in_block_comment,
            in_line_comment,
            in_raw_string,
            raw_delimiter,
        )
        if not clean:
            continue

        if CONFIG_RE.match(clean):
            has_config_include = True

        if GUARD_RE.match(clean):
            if_stack.append({"is_guard": True, "in_else": False})
        elif IF_RE.match(clean):
            if_stack.append({"is_guard": False, "in_else": False})
        elif ELIF_ELSE_RE.match(clean):
            if if_stack and if_stack[-1]["is_guard"]:
                if_stack[-1]["in_else"] = True
        elif ENDIF_RE.match(clean):
            if if_stack:
                if_stack.pop()

        m = INCLUDE_RE.match(clean)
        if m:
            header = m.group(1)
            if "hpxinspect:nostdincludeguard" in line:
                continue
            if header in MACRO_HEADERS:
                continue
            is_guarded = any(
                f["is_guard"] and not f["in_else"] for f in if_stack
            )
            if header in STANDARD_HEADERS and not is_guarded:
                unguarded_indices.append(i)

    if not unguarded_indices:
        return 0

    # Group contiguous blocks of unguarded includes
    groups = []
    current_group = []
    for idx in unguarded_indices:
        if not current_group:
            current_group.append(idx)
        elif idx == current_group[-1] + 1:
            current_group.append(idx)
        else:
            groups.append(current_group)
            current_group = [idx]
    if current_group:
        groups.append(current_group)

    # Apply guards backwards so line numbers remain valid
    for grp in reversed(groups):
        start_idx = grp[0]
        end_idx = grp[-1]

        if not lines[end_idx].endswith(("\n", "\r\n")):
            lines[end_idx] += "\n"

        lines.insert(end_idx + 1, GUARD_CLOSE)
        lines.insert(start_idx, GUARD_OPEN)

    # If <hpx/config.hpp> was not included, add it before the first guard
    if not has_config_include:
        first_guard_idx = groups[0][0]
        lines.insert(first_guard_idx, "#include <hpx/config.hpp>\n\n")

    with open(filepath, "w", encoding="utf-8") as f:
        f.writelines(lines)

    return len(unguarded_indices)


def find_headers(path):
    """Yield all C++ header files in path."""
    for root, _, files in os.walk(path):
        for f in files:
            if f.endswith((".hpp", ".h", ".hxx", ".ipp")):
                yield os.path.join(root, f)


def main():
    parser = argparse.ArgumentParser(
        description="Apply include guards around standard library includes in HPX headers."
    )
    parser.add_argument(
        "targets",
        nargs="+",
        help="Module directories, include paths, or header files.",
    )
    args = parser.parse_args()

    total_modified = 0
    total_guarded = 0

    for target in args.targets:
        if os.path.isfile(target):
            files = [target]
        elif os.path.isdir(target):
            include_dir = os.path.join(target, "include")
            search_dir = include_dir if os.path.isdir(include_dir) else target
            files = list(find_headers(search_dir))
        else:
            print(f"Error: target not found: {target}", file=sys.stderr)
            continue

        for filepath in files:
            count = guard_file(filepath)
            if count > 0:
                total_modified += 1
                total_guarded += count
                print(f"Guarded {count} include(s) in {filepath}")

    print(
        f"Applied include guards: modified {total_modified} file(s), "
        f"guarded {total_guarded} include(s)."
    )


if __name__ == "__main__":
    main()
