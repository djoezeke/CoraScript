# Cora JIT Compiler Implementation Guide
This document updates the implementation details to emphasize modularity, a simple API, extensibility, robust error handling/logging, testing, and benchmarking.

## Modularity and Extensibility

*   **Component Separation:** Each core part (Lexer, Parser, AST, Semantic Analyzer, IR Generator, Optimizer, Bytecode Generator, VM) is implemented as a separate C++ class/namespace. This allows for easier understanding, maintenance, and replacement.
    *   `Lexer`, `Parser`, `SemanticAnalyzer` classes.
    *   AST node hierarchy (`ASTNode` base, `Expression`, `Statement`, etc. derived classes).
    *   `SymbolTable`, `TypeChecker` classes.
    *   IR structures (`IRModule`, `IRFunction`, `BasicBlock`, `IRInstruction`, `IRValue`).
    *   `Pass` abstract base class, concrete optimization passes.
    *   `BytecodeGenerator` class.
    *   `VirtualMachine` class with `ExecutionMode` (INTERPRETER, JIT).
*   **Pluggable Components:**
    *   **Optimizations:** The `PassManager` can be configured to include/exclude specific `Pass` objects.
    *   **VM Execution:** The `CoraEngine` allows selecting the `VirtualMachine::ExecutionMode`.
*   **Extensible Instruction Set:** The `OpCode` enum in the VM and Bytecode Generator can be expanded to support new Cora features or VM capabilities.
*   **Host Function Integration:** The VM's `execute` mechanism can be extended to allow registering C++ functions callable from Cora bytecode.

## Runtime & Memory Management
- **Value Representation:** `cora::runtime::value` (using `std::variant` or a tagged union for performance).
- **Garbage Collection:** A simple Mark-and-Sweep collector initially, moving towards a Generational GC.
- **FFI:** Support for calling C++ functions directly from Cora.

## Simple API and Embedding

The `CoraEngine` class serves as the primary interface.
*   **Compilation:** The `compile` method takes Cora source code and produces a `Module` object containing bytecode. It returns a boolean indicating success or failure, with errors logged in the `DiagnosticEngine`.
*   **Execution:** The `call` method allows executing a function from the compiled module, passing arguments and retrieving results.


```cpp
#include <Cora/Cora.hpp>

int main() {
    cora::Engine engine;
    auto module = engine.Compile("func add(a, b int) int { return a + b }");
    auto result = engine.Call(module, "add", {1, 2});
    return result.asInt();
}
```

## Error Handling and Logging
Inspired by Rust and GCC:
*   **Error Handling:** The `compile` method returns a boolean indicating success or failure. Compilation errors are collected in the `DiagnosticEngine`, which can be queried by the host application to retrieve error messages and logs.
*   **Logging:** The `DiagnosticEngine` also serves as a logging mechanism for both compilation and runtime events, allowing the host application to access detailed logs. 
- **Error Levels:** Note, Warning, Error, Fatal.
- **Snippets:** Display the source line with the error highlighted.
- **Suggestions:** "Did you mean ...?" style feedback.
- **Implementation:** A `DiagnosticEngine` class that manages a collection of `Diagnostic` objects.
*   **Error Propagation:** Errors detected in early stages (Lexer, Parser, Semantic Analyzer) are logged and can halt compilation. VM execution errors are also captured and logged.
*   **API Feedback:** The `compile` method returns a boolean indicating success or failure, allowing the host application to retrieve errors via `getLog()`.

## Extensibility

*   **Component Swapability:** The modular design allows replacing or enhancing individual components. For example, a new optimization pass can be added by implementing the `Pass` interface and registering it with the `PassManager`.
*   **VM Instruction Set:** New opcodes can be added to `OpCode` and implemented in the `VirtualMachine`, enabling support for more Cora features or VM-level optimizations.
*   **JIT Backend:** The VM's JIT compiler can be designed to allow different native code generation backends (e.g., supporting different architectures or employing different JIT strategies).

## Testing and Benchmarking Strategy

*   **Testing Framework:** Use a standard C++ testing framework (e.g., Google Test, Catch2).
*   **Unit Tests:**
    *   **Lexer:** Test tokenization of various Cora constructs, including keywords, identifiers, literals, operators, and comments. Verify error detection (invalid characters, unterminated strings).
    *   **Parser:** Test grammar rules, AST generation for expressions, statements, and function definitions. Verify syntax error detection and basic recovery.
    *   **Semantic Analyzer:** Test type checking for expressions and assignments, name resolution, scope rules, and error reporting for undeclared variables or type mismatches.
    *   **IR Generator:** Test translation of AST nodes to IR instructions.
    *   **Optimization Passes:** Test each pass on specific IR patterns to ensure correctness (e.g., constant folding replaces constant operations).
    *   **Bytecode Generator:** Test IR to bytecode translation for all supported IR instructions.
    *   **VM Interpreter:** Test each bytecode opcode and control flow constructs rigorously.
    *   **VM JIT Compiler (Conceptual):** Test JIT compilation of bytecode to native code, verifying correctness and performance.
*   **Integration Tests:** Test the full compilation pipeline from Cora source code to VM execution, covering various valid and invalid code scenarios.
*   **Error Handling Tests:** Ensure all error types are correctly identified, reported with accurate locations, and logged.
*   **Performance Benchmarks:**
    *   **Objective:** Evaluate the performance of JIT-compiled code versus interpreter mode, and compare against native compilation where feasible.
    *   **Metrics:** Execution time, memory usage, JIT compilation overhead.
    *   **Methodology:** Use standard benchmarking techniques. Create representative Cora code snippets and small programs to test various language features and common patterns. Analyze results to guide further optimizations in the IR, bytecode, or JIT compiler.
    *   **Tools:** Employ C++ benchmarking libraries (e.g., Google Benchmark) or custom timing mechanisms. Integrate profiling hooks within the VM to gather runtime data.
