# Dread Kings v3 — C++ Engineering & Robotics Discovery Engine

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/cpp/compiler_support/23)
[![Rust](https://img.shields.io/badge/rust--kings-edition%202021-orange.svg)](rust-kings/Cargo.toml)

A functional C++23 baseline for Dread Kings. It does not claim that generated
candidates are physically validated inventions; it performs computational
design-space search, evaluates declared constraints, verifies the
computational contract, and emits a robot-model artifact. A companion Rust
crate (`rust-kings/`) performs concurrent source-level analysis of C++ files.

## Quick start

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/dread-kings
```

Requirements: CMake 3.20+, and a C++23-capable compiler (recent Clang, GCC, or
MSVC). `BUILD_TESTING` is on by default, so `ctest` runs the `dread_core`,
`dread_kings_pipeline`, and `clang_rewriter` test targets.

## Architecture overview

Dread Kings is organized as a static library (`dread_kings`) built from
`src/`, with public headers under `include/dread_kings/`, consumed by the
`dread-kings` CLI executable (`examples/dread_kings.cpp`) and the test
binaries under `tests/`.

1. **Discovery** (`discovery/`) generates candidate design vectors across a
   bounded `DesignSpace` and runs a random search against an `Objective`.
2. **Verification** (`verification/`) checks each candidate against an
   explicit, declared feasibility contract — it does not perform physical
   simulation.
3. **Robotics** (`robotics/`) turns a feasible candidate into a robot-model
   artifact (e.g. SDF generation).
4. **Core / Dread Kings pipeline** (`core/`, `kings/`) runs registered
   `DreadKing` source analyzers concurrently over a `ProblemContext` and
   aggregates their verdicts.
5. **Toolchain** (`toolchain/`) probes for an external compiler and can run an
   optional syntax check via `clang++`/`g++`.
6. **Compiler rewrites** (`compiler/`) applies surgical, line/column-scoped
   source rewrites without an AST backend (see below).

## Module map

| Path | Role |
| --- | --- |
| `src/discovery/`, `include/dread_kings/discovery/` | Design vectors, candidate generation, random search |
| `src/robotics/`, `include/dread_kings/robotics/` | Robot model and SDF generation |
| `src/verification/verification.cpp`, `include/dread_kings/verification/` | Explicit feasibility contract |
| `src/verification/concurrency_king.cpp`, `include/dread_kings/kings/` | Concurrency-focused Dread King (e.g. `memory_order_relaxed` checks) |
| `src/toolchain/`, `include/dread_kings/toolchain/` | Compiler probing and optional syntax checks |
| `src/toolchain/compiler_pipeline.cpp`, `src/toolchain/clang_rewriter.cpp`, `include/dread_kings/compiler/` | Compiler checks and surgical source rewrite utilities |
| `src/core/dread_kings.cpp`, `include/dread_kings/core/` | Shared Dread King, Dread Kings runner, problem-context, and verdict contracts |
| `examples/dread_kings.cpp` | `dread-kings` CLI entry point |
| `tests/` | C++ test binaries registered with CTest |
| `rust-kings/` | Companion Rust crate for concurrent source analysis |
| `knowledge/cppreference/` | Reference-layer policy and entry points (not a substitute for the ISO C++ standard) |

## Dread Kings pipeline

The first Dread Kings 2.0 slice is available through `core::create_dread_kings()`.
Register `core::DreadKing` implementations, then submit a `core::ProblemContext`:

```cpp
auto dread_kings = dread::core::create_dread_kings();
dread_kings->register_king(
	std::make_unique<dread::concurrency::DreadConcurrencyKing>());

dread::core::ProblemContext problem{
	.context_id = "candidate.cpp",
	.raw_source_code = source,
	.requested_standard = dread::core::CppStandard::Cpp23};
const auto verdict = dread_kings->evaluate_all(problem);
```

Registered Dread Kings run concurrently and their violations are aggregated. The
concurrency Dread King flags `memory_order_relaxed` stores when threaded source is
present and reports a remediation plus ISO clause reference. This is a
source-level heuristic, not a replacement for ThreadSanitizer or formal model
checking. `dread-kings verify <file>` also performs an optional `clang++` syntax
check; set `DREAD_COMPILER` to a Clang or GCC executable to select another
compiler. Missing external compilers are reported but do not hide Dread King results.

## Rust companion crate

`rust-kings/` is a companion Rust crate (`dread-kings-rust`) that analyzes
source files using concurrent Dread Kings. It demonstrates trait objects,
iterator pipelines, `Box` to `Arc` ownership, shared `Arc<str>` context,
mutex-protected metrics, Tokio blocking tasks, completion channels, and typed
Dread King errors. Run it from the crate directory:

```bash
cd rust-kings
cargo test
cargo run -- ../examples/dread_kings.cpp
```

Domain failures use `thiserror`; the command-line boundary uses `anyhow` to add
file context. `Rc<RefCell<T>>` is deliberately not used by the concurrent Dread Kings runner:
it is for single-threaded shared mutation, while these `Send + Sync`
Dread Kings run across worker threads. The crate tests cover source findings,
duplicate registration, concurrent completion, metrics, and aggregated errors.

## Surgical rewrites

`compiler::DreadClangRewriter` applies line/column-scoped token directives and
returns the original source, modified source, replacement count, and unified
diff without writing the file. The current backend is dependency-free and
preserves source bytes; installing LLVM/Clang libTooling is required before
replacing it with a true AST traversal backend.

## Next production integrations

1. Replace the analytical evaluator with a Gazebo/MuJoCo adapter.
2. Add ROS 2 transport/control nodes.
3. Add AST-aware C++ generation/refactoring.
4. Add compiler diagnostic parsing and repair loops.
5. Add multi-objective/Pareto optimization and experiment provenance.
6. Add hardware-in-the-loop gates before any physical deployment.

## Contributing

Contributions are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for build,
test, and development instructions, and [CHANGELOG.md](CHANGELOG.md) for a
log of notable changes.

## License

Dread Kings is licensed under the [MIT License](LICENSE).
