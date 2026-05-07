# CoraScript

CoraScript is a high-performance, embeddable scripting language implemented in C++17. It features a complete compiler pipeline, intermediate representation (IR) with optimizations, and a virtual machine with both interpreter and JIT execution modes.

## Project Overview

- **Language**: C++17
- **Architecture**:
    - **Frontend**: Lexer and Parser (`src/Parser`) generating an Abstract Syntax Tree (`src/AST`).
    - **Middle-end**: IR generation (`src/IR`), optimization (`src/IR/IROptimizer.cpp`), and verification.
    - **Back-end**: Bytecode generation (`src/Bytecode`) and Virtual Machine (`src/VMachine`).
    - **JIT**: Assembler and Backend-specific code generation (`src/JITCom`).
    - **Runtime**: Value system, scope management, and garbage collection (`src/Runtime`).
    - **Embedding**: Public API in `include/Cora` for C++ integration.

## Building and Running

### Prerequisites
- CMake 3.16+
- C++17 compliant compiler
- Ninja (recommended)

### Build Commands
The project uses CMake presets for convenience.

```powershell
# Configure the project
cmake --preset default

# Build the project
cmake --build --preset default

# Run tests
ctest --preset default
```

### Key Executables
- `build/src/Cora/cora`: Main interpreter and compiler tool.
- `build/src/Cora/coravm`: Bytecode execution engine.

## Development Conventions

### Coding Style
- **Namespaces**: Rooted in `cora`, with sub-namespaces like `cora::parser`, `cora::vmachine`, etc.
- **Naming**:
    - Classes: `PascalCase` (e.g., `Lexer`, `BytecodeProgram`)
    - Methods: `PascalCase` (e.g., `NextToken()`, `Run()`)
    - Member Variables: `m_CamelCase` (e.g., `m_Source`, `m_Position`)
    - Constants: `kCamelCase` (e.g., `kRegisterCount`)
- **Formatting**:
    - 4-space indentation.
    - Braces on new lines (Allman style).
- **Files**: `PascalCase.cpp` and `PascalCase.hpp`.

### Testing
- Tests are located in the `tests/` directory.
- Built using CTest.
- When adding new features, add corresponding tests in the relevant subfolder (`tests/parser`, `tests/ir`, `tests/bytecode`).

## Architecture Details

- **Pipeline**: Source -> Lexer -> Parser -> AST -> IR -> Optimizer -> Bytecode -> VM.
- **FFI**: Cora provides a robust Foreign Function Interface for binding C++ functions and classes to the scripting environment (see `include/Cora/Cora.hpp`).
- **Memory Management**: Uses a custom Garbage Collector (`src/Runtime/GarbageCollector.hpp`) and reference-counted `Value` objects.

enhance lexer, parser, ast, ir, bytecode, value, gc and vm to successfully run @sample.cora. fix extended to support new syntax and allow vm to register builtin modules. fix ir and bytecode reader, writer and printer classes to cover all instructions.