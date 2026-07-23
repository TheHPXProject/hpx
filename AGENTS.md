<!-- Copyright (c) 2026 Hartmut Kaiser                                            -->
<!--                                                                              -->
<!-- SPDX-License-Identifier: BSL-1.0                                             -->
<!-- Distributed under the Boost Software License, Version 1.0. (See accompanying -->
<!-- file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)        -->

# AGENTS.md

Guidance for agents working in `TheHPXProject/hpx`.

## Project overview

HPX is a C++ standard library for concurrency and parallelism. It uses
CMake, has a large test suite, and follows Boost-style coding conventions.

## Before changing code

- Read `README.rst` and `CONTRIBUTING.md`.
- Do not update or rewrite `AGENT.md` unless explicitly asked.
- Do not stage, commit, amend, or push unless explicitly asked.
- Do not suggest staging, committing, amending, or pushing unless
  explicitly asked.
- Prefer small, focused changes.
- Check whether an existing utility in `namespace hpx` or the C++ standard
  library already solves the problem.
- If the change touches build or install behavior, look for existing CMake
  patterns in the repository first.

## Coding style

- Follow Boost coding standards.
- Use 80-character lines.
- Use spaces only; no tabs.
- Avoid raw pointers, raw loops, and raw threads wherever possible.
- Prefer STL-style identifiers: `my_class`, not `MyClass`.
- Use expressive, self-descriptive names.
- Use exceptions for error handling instead of C-style error codes.
- Add doxygen-style comments for API functions. For this, use
  `///` style comments and `\` prefixed doxygen tags.
- Use the repository's `.clang-format` and `.editorconfig` as guidance.

## Formatting rules

- Keep formatting-only changes separate from functional changes.
- Do not reformat whole files unless necessary.
- Do not configure auto-format-on-save for source files.

## New files

Add the project license header:
```cpp
//  Copyright (c) <year> <your name>
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
```

## Build and test

- Prefer CMake-based builds.
- HPX using C++20 as a minimal requirement.
- Run the smallest relevant test set first.
- For code changes, run targeted tests for the affected component/module.
- For build-system changes, verify at least one configuration that exercises
  the changed path.
- `CMakeFiles.txt` and `*.cmake` files should be formatted using
  `cmake-format` based rules defined in `.cmake-format.py` (see:
  https://github.com/cheshirekow/cmake_format).

## PR / contribution workflow

- Prefer coherent, reviewable slices.
- Keep dependencies minimal.
- Use public APIs unless explicitly told otherwise.
- Keep benchmark and diagnostic code simple and readable.
- Do not hide important timing assumptions.
- Do not create large generated files.
- Make changes on a topic branch.
- Keep commits clear and scoped.
- Mention relevant issue numbers in PR descriptions when applicable.
- If a change is documentation-only, say so explicitly in the PR.

## When in doubt

- Match local patterns in nearby files.
- Keep changes minimal.
- Preserve existing APIs and behavior unless the task explicitly asks otherwise.
