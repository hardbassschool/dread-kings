# Dread Kings v3 — C++ Engineering & Robotics Discovery Engine

> A C++23 and Rust toolkit for concurrent source analysis, async verification,
> and robotics design-space search.

[![CI](https://github.com/hardbassschool/dread-kings/actions/workflows/ci.yml/badge.svg)](https://github.com/hardbassschool/dread-kings/actions/workflows/ci.yml)
[![Coverage](https://img.shields.io/badge/coverage-in_progress-lightgrey)](https://github.com/hardbassschool/dread-kings/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

This is a functional C++23 baseline for Dread Kings. It does not claim that generated candidates are physically validated inventions; it performs computational design-space search, evaluates declared constraints, verifies the computational contract, and emits a robot-model artifact.

The project combines systems Rust and modern C++ for concurrency, async task
coordination, source-level verification, and robotics tooling. Its checks are
heuristics and computational contract checks—not physical validation,
ThreadSanitizer, or formal model checking.

## Try it

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/dread-kings
ctest --test-dir build --output-on-failure
```

Verify a C++ source file:

```bash
./build/dread-kings verify examples/dread_kings.cpp
```

Try the async Rust source analyzer from the crate directory:

```bash
cd rust-kings
cargo test
cargo run -- ../examples/dread_kings.cpp
```

See [CONTRIBUTING.md](CONTRIBUTING.md) for complete build, lint, and test
instructions.

## Architecture

- `src/discovery/` and `include/dread_kings/discovery/`: design vectors, candidate generation, and design-space search.
- `src/robotics/` and `include/dread_kings/robotics/`: robot model and SDF generation.
- `src/verification/` and `include/dread_kings/verification/`: feasibility contract and source-level concurrency checks.
- `src/toolchain/` and `include/dread_kings/toolchain/`: compiler probing, invocation, and source rewrite utilities.
- `src/core/` and `include/dread_kings/core/`: Dread Kings runner,
  problem-context, and verdict contracts.
- `include/dread_kings/compiler/`: compiler checks and surgical source rewrite utilities.
- `include/dread_kings/kings/`: specialized source analyzers.
- `rust-kings/`: async, concurrent Rust source-analysis companion crate.
- `knowledge/cppreference/`: reference-layer policy and entry points.

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

## Rust Dread Kings

`rust-kings/` is a companion Rust crate that analyzes source files using
concurrent Dread Kings. It demonstrates trait objects, iterator pipelines, `Box` to
`Arc` ownership, shared `Arc<str>` context, mutex-protected metrics, Tokio
blocking tasks, completion channels, and typed Dread King errors. Run it from the
crate directory:

```bash
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

## What's next

1. Replace the analytical evaluator with a Gazebo/MuJoCo adapter.
2. Add ROS 2 transport/control nodes.
3. Add AST-aware C++ generation/refactoring.
4. Add compiler diagnostic parsing and repair loops.
5. Add multi-objective/Pareto optimization and experiment provenance.
6. Add hardware-in-the-loop gates before any physical deployment.
