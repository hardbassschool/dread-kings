# Contributing

## Prerequisites

- CMake 3.20 or newer and a C++23-capable compiler.
- Rust and Cargo (stable, edition 2021) to build or test the companion crate.

## Build and test

From the repository root, configure and build the C++ project, then run its
CTest suite:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The C++ build enables `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` on
non-MSVC compilers. There is no repository-wide C++ formatter configuration;
keep changes consistent with the surrounding code and avoid unrelated
formatting changes.

The Rust crate is in `rust-agents/`. Run its formatting, lint, and test checks
from that directory:

```bash
cargo fmt --check
cargo clippy --all-targets --all-features -- -D warnings
cargo test
```

## Pull requests

- Keep changes focused and describe their motivation and user-visible effect.
- Add or update tests when behavior changes.
- Run the relevant build, formatting, lint, and test checks, and include the
  results in the pull-request description.
- Note any checks that could not be run locally.
