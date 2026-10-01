# Contributing to Dread Kings

Thanks for your interest in improving Dread Kings. This document covers the
practical steps for building, testing, and submitting changes.

## Prerequisites

- A C++23-capable compiler (recent Clang or GCC; MSVC also supported)
- CMake 3.20+
- Rust toolchain (stable) with `cargo`, for `rust-kings/`

## Building and testing the C++ core

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/dread-kings
```

`BUILD_TESTING` is enabled by default via `include(CTest)`, so the test
targets (`dread_core`, `dread_kings_pipeline`, `clang_rewriter`) are built and
registered automatically.

To check a specific source file with the optional compiler syntax check, set
`DREAD_COMPILER` to a Clang or GCC executable before running `dread-kings
verify <file>`. Missing external compilers are reported but do not hide Dread
King results.

## Building and testing the Rust crate

```bash
cd rust-kings
cargo build
cargo test
cargo run -- ../examples/dread_kings.cpp
```

## Code organization

See the "Module map" section in `README.md` for where source for each concern
lives (`src/discovery`, `src/robotics`, `src/verification`, `src/toolchain`,
`src/core`, and their headers under `include/dread_kings/`).

## Making changes

- Keep changes focused and minimal; avoid unrelated refactors in the same
  change.
- Match the existing code style (naming, header organization, comment style)
  in the file you are editing.
- Add or update tests under `tests/` (C++) or `rust-kings/src/` (Rust) for any
  behavior change.
- Run the full build and test suite for both the C++ and Rust parts before
  submitting a change.
- Update `README.md` and/or `CHANGELOG.md` when behavior, build steps, or
  the module layout change.

## Commit and pull request guidelines

- Write clear, descriptive commit messages.
- Describe the motivation and effect of the change in the pull request
  description.
- Note any manual verification steps you performed (build output, test runs,
  example invocations).

## Reporting issues

Please include reproduction steps, the compiler/toolchain versions in use,
and relevant command output when filing an issue.
