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

- Read `README.rst` and `.github/CONTRIBUTING.md`.
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
- Use 80-character (soft limit) lines that should not exceed 90
  characters (hard limit).
- Use spaces only; no tabs.
- Avoid raw pointers, raw loops, and raw threads wherever possible.
- Prefer STL-style identifiers: `my_class`, not `MyClass`.
- Use expressive, self-descriptive names.
- Use exceptions for error handling instead of C-style error codes.
- Add doxygen-style comments for API functions. For this, use
  `///` style comments and `\` prefixed doxygen tags.
- Use the repository's `.clang-format` and `.editorconfig` as guidance.
- In the HPX libraries use `HPX_MOVE(x)` and `HPX_FORWARD(T, x)` instead of
  `std::move` and `std::forward`. Tests and examples use the standard
  functions.

## Data layout and hot paths

- Use `alignas` only for data that is allocated during initialization
  (for instance the scheduler's data). Types that are allocated all the
  time (threads, futures, shared states, tasks) use explicit padding
  instead, so that the allocator doesn't have to align them.
- Don't give bit fields default member initializers; some supported
  compilers don't accept them. Initialize them in every constructor.
- Don't read clocks (`std::chrono::steady_clock::now()` and similar) inside
  spin loops or other hot paths; a clock read costs far more than a spin
  round. Calibrate once at startup and count rounds instead.

## Modules

- Module dependencies must not be circular: a module must not depend,
  directly or through other modules, on a module that depends on it.
  This also means that `libs/core` modules must not depend on `libs/full`
  modules.
- Classes and functions used outside their module need the export macro
  of their library: `HPX_CORE_EXPORT` in `libs/core`, `HPX_EXPORT` in
  `libs/full`. A missing export macro links on Linux and fails only on
  Windows.
- List new headers and sources in the module's `CMakeLists.txt`
  (`<module>_headers`, `<module>_sources`).
- Every header must compile on its own: the header tests
  (`HPX_WITH_TESTS_HEADERS=ON`) compile each one alone.
- HPX can also be built as C++ modules (`HPX_WITH_CXX_MODULES=ON`), which
  CI checks separately; changes to public headers can break that build
  only. In that build, declarations that other modules or users need must
  be exported from their C++ module: mark them with `HPX_CXX_CORE_EXPORT`
  in `libs/core` and `HPX_CXX_EXPORT` in `libs/full` (they expand to
  `export` in that build and to nothing otherwise). Keep the exported
  interface as small as possible: export only what is needed outside the
  module, and avoid exporting declarations in `detail` namespaces where
  possible. These macros are separate from the library export macros
  above.

## Compiler-specific rules

CI builds with MSVC, GCC, and Clang, with warnings as errors in several
configurations. Each compiler warns about different things:

- MSVC: a class exported from a module (`HPX_CORE_EXPORT` and similar)
  whose members have standard library types needs
  `#include <hpx/config/warnings_prefix.hpp>` after the other includes and
  `#include <hpx/config/warnings_suffix.hpp>` at the end of the header;
  otherwise MSVC warns (C4251) in every file that includes the header.
- MSVC: implicit narrowing conversions that GCC and Clang accept produce
  warnings (C4267, C4244). Use `std::size_t` for sizes and indices, and
  cast explicitly where narrowing is intended.
- GCC: initialize a `std::array` with double braces (`{{...}}`); single
  braces fail with `-Werror=missing-braces`.
- Compile a change with more than one of these compilers before
  submitting it, at least the files it touches.

## Formatting rules

- Keep formatting-only changes separate from functional changes.
- Do not reformat whole files unless necessary.
- Do not configure auto-format-on-save for source files.
- Format C++ files with clang-format 20, using the rules in
  `.clang-format`; clang-format 20 is the version of the CI check
  (`.github/workflows/check-formatting.yml`), other versions format some
  constructs differently.
- Format `CMakeLists.txt` and `*.cmake` files with `cmake-format`, using
  the rules in `.cmake-format.py` (see
  https://github.com/cheshirekow/cmake_format); the CI checks this too.

## New files

Add the project license header:
```cpp
//  Copyright (c) <year> <your name>
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
```

Every new file needs this license information, not only C++ files (the
repository follows REUSE, see `REUSE.toml`): use the comment syntax of the
file type, as the HTML comment at the top of this file does for Markdown.

## Build and test

- Prefer CMake-based builds.
- HPX using C++20 as a minimal requirement.
- Run the smallest relevant test set first.
- For code changes, run targeted tests for the affected component/module.
- For build-system changes, verify at least one configuration that exercises
  the changed path.
- Add a unit test to the `tests` list in the `CMakeLists.txt` of the
  module's test directory; extra arguments go into
  `<test>_PARAMETERS` (e.g. `THREADS_PER_LOCALITY 4`). The target is named
  `<test>_test`, the CTest name is
  `tests.unit.modules.<module>.<test>`, so `ctest -R <test>` runs it.
- Tests check with the `HPX_TEST*` macros from `hpx/modules/testing.hpp`
  and return `hpx::util::report_errors()`; a test of the local runtime
  starts it with `hpx::local::init`.

## PR / contribution workflow

- Prefer coherent, reviewable slices.
- Keep dependencies minimal.
- Use public APIs unless explicitly told otherwise.
- Keep benchmark and diagnostic code simple and readable.
- Do not hide important timing assumptions.
- Do not create large generated files.
- Make changes on a topic branch.
- Keep commits clear and scoped.
- Sign off every commit (`git commit -s`); CI checks the DCO sign-off.
- Document user-visible additions in `docs/sphinx`: configuration
  settings and command-line options in
  `manual/launching_and_configuring_hpx_applications.rst`, performance
  counters in `manual/optimizing_hpx_applications.rst`. Add CMake options
  with `hpx_option`.
- Mention relevant issue numbers in PR descriptions when applicable.
- If a change is documentation-only, say so explicitly in the PR.

## When in doubt

- Match local patterns in nearby files.
- Keep changes minimal.
- Preserve existing APIs and behavior unless the task explicitly asks otherwise.

## Additional guidelines

- Never #include HPX module-internal headers (paths under
  `libs/<module>/include/hpx/<module>/...`, e.g. `hpx/serialization/serialize.hpp`)
  from outside that module. Only code that is itself part of a given module may
  include its internal headers directly. Any other consumer (another module, a test
  executable, or application code) must include the generated umbrella header
  `hpx/modules/<module_name>.hpp` instead
  (e.g. `#include <hpx/modules/serialization.hpp>`).
