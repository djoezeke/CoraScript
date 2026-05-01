<!-- Copilot / AI agent instructions for working on the Cora codebase -->

# Quick orientation

- **What this project is:** an ahead-of-time / VM language implementation (Cora) — compiler, IR, bytecode, and VM/JIT components live under `src/`.
- **High-level flow:** `src/Parser` -> `src/AST` -> `src/Semantic` -> `src/IR` -> `src/Bytecode` -> `src/VMachine` / `JITCom`.
- **Key entry points:** `src/Cora/cora.cpp`, `src/Cora/corac.cpp`, `src/Cora/coravm.cpp` and the orchestration in `src/Cora/Pipeline.cpp`.

# Build & test (exact commands)

- Configure (creates `build/`):

  cmake --preset default

- Build (uses Ninja per preset):

  cmake --build build

- Run tests (CTest output on failure):

  ctest --test-dir build --output-on-failure

- Notes:
  - `CMakePresets.json` sets `generator: Ninja` and `binaryDir: ${sourceDir}/build`.
  - A full compile database is available at `build/compile_commands.json` for linters/IDE integration.

# Project structure & patterns to rely on

- `src/` is organized by compilation pipeline phases and subsystems (AST, Parser, Semantic, IR, Bytecode, VMachine, JITCom, Builtin).
- Each subsystem typically has a `CMakeLists.txt` and paired `.cpp/.hpp` files (e.g. `src/AST/ASTNode.hpp`, `src/AST/ASTExpr.cpp`).
- Use `src/Basic` for diagnostics, errors and pretty-print helpers (`Diagnostics.cpp`, `ErrorContext.cpp`, `PrettyPrinter.cpp`). Follow those patterns for new diagnostics.
- Builtins and standard library pieces: `src/Builtin` and `lib/std` (e.g. `lib/std/io.cora`, `modules/io.cpp`).

# Cross-component conventions

- Pipeline direction is explicit: transform trees/IR progressively rather than scattering logic across layers. Look for `Pipeline.cpp` to understand stage boundaries.
- Bytecode/IR serialization has reader/writer helpers (`src/Bytecode/BCReader.cpp`, `src/IR/IRReader.cpp`) — prefer those helpers when adding formats or compatibility changes.

# Common developer workflows

- Add a new language pass: create files under the appropriate subsystem (e.g. `src/IR`), add the target or object to the nearest `CMakeLists.txt`, and wire it into `src/Cora/Pipeline.cpp`.
- Add runtime/native modules: place C++ glue in `modules/` and headers under `include/Cora` so they can be discovered by the build and the `Builtin` registration code.
- To run specific tests:

  ctest --test-dir build -R parser # runs parser tests

# Debugging and analysis

- Use `build/compile_commands.json` with clangd, clang-tidy or an IDE for navigation and quick static checks.
- For local debugging, build and run the relevant binary (targets include `cora`, `corac`, `coravm` — see `src/Cora/*`) and pass sample scripts from the repo root (`hello.cora`, `sample.cora`).

# Files worth reading first (quick tour)

- `src/Cora/Pipeline.cpp` — shows how stages are composed and how artifacts flow.
- `src/Parser/Parser.cpp` and `src/AST/ASTNode.hpp` — parsing → AST representation.
- `src/Semantic` (folder) — type/semantic checks and symbol handling.
- `src/IR/IRBuilder.cpp` and `src/Bytecode/BCBuilder.cpp` — lowering to IR/bytecode.
- `src/VMachine` + `src/JITCom` — runtime, assembler and JIT integration.

# Agent behavior rules (practical / repo-specific)

- When proposing code changes, update the nearest `CMakeLists.txt` rather than adding top-level build hacks.
- Prefer reusing existing helper functions for error reporting and printing (see `src/Basic/*`) to maintain consistent messages and formatting.
- When editing ABI/serialization (IR/Bytecode readers or writers), include a small compatibility test under `tests/ir` or `tests/bytecode`.
- Avoid changing broad build presets; prefer adding a new CMake option only if necessary and document it in the root `CMakePresets.json`.

# If unsure, read these files for context before changes

- `CMakePresets.json` (build presets), `build/compile_commands.json` (compile DB), `src/Cora/Pipeline.cpp`, `src/Basic/ErrorContext.cpp`.

---

If any section is unclear or you'd like me to add examples (PR snippets, typical unit-test stubs, or a checklist for adding new passes), tell me which area to expand.
