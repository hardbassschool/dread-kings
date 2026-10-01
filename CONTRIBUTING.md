# Contributing

Thanks for your interest in Dread Kings. Contributions that improve the
design-space search, source analysis, robotics verification, documentation, or
tests are welcome.

## Prerequisites

- CMake 3.20 or newer and a C++23-capable compiler.
- Rust and Cargo (stable, edition 2021) for the companion crate.

## Build and test

From the repository root, build and test the C++ project:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The C++ executable provides a design-search example and can verify a source
file:

```bash
./build/dread-kings
DREAD_COMPILER=c++ ./build/dread-kings verify examples/dread_kings.cpp
```

The companion Rust crate lives in `rust-kings/`. From that directory, run its
lint and test checks and try its source-analysis CLI:

```bash
cd rust-kings
cargo clippy --all-targets --all-features -- -D warnings
cargo test
cargo run -- ../examples/dread_kings.cpp
```

## Pull requests

- Keep changes focused and explain their motivation and user-visible effect.
- Add or update tests when behavior changes.
- Run the checks relevant to your change and report any checks you could not run.
- Avoid unrelated formatting changes.
