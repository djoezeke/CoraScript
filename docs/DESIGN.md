# Cora JIT Compiler Design

This document details the design considerations for the custom backend of the Cora JIT compiler, now including a Virtual Machine (VM) with JIT compilation capabilities for executing bytecode. The focus is on efficiency, extensibility, and robustness.

## 1. Custom IR Generator Design

*   **Intermediate Representation (IR):** A custom Static Single Assignment (SSA) form IR.
    *   **Structure:** Remains largely the same as previously defined (Instructions, Basic Blocks, CFG, Values).
    *   **Goal:** To be suitable for translation into efficient VM bytecode.

## 2. Optimization Passes Design

*   **Passes:** Operate on the custom SSA IR.
*   **Goal:** To produce IR that translates efficiently into optimized bytecode, reducing the burden on the VM's JIT or interpreter.

## 3. Bytecode Generator Design

*   **Bytecode Format:** Design a custom bytecode instruction set. This bytecode will be the target for the Code Generator and the source for the Virtual Machine.
    *   **Instruction Set:**
        *   **Register-based:** To match the current `BCInstruction` design which uses register indices (`regs`). Register-based VMs typically require fewer instructions than stack-based VMs for the same task.
        *   **Opcodes:** Include instructions for:
            *   Arithmetic/Logical operations (`ADD`, `SUB`, `MUL`, `DIV`).
            *   Data movement (`PUSH`, `POP`, `LOAD_LOCAL`, `STORE_LOCAL`, `LOAD_GLOBAL`, `STORE_GLOBAL`).
            *   Control Flow (`JUMP`, `BRANCH_IF_TRUE`, `BRANCH_IF_FALSE`, `CALL`, `RETURN`).
            *   Memory Operations (`ALLOCATE_STACK`, `FREE_STACK`, `LOAD_MEM`, `STORE_MEM`).
            *   Type Handling/Conversions.
            *   Function/Method Calls.
    *   **Constants Pool:** For storing immediate values (integers, floats, strings).
    *   **Function Definitions:** Bytecode representation of functions.
*   **IR to Bytecode Translation:** Map optimized IR instructions to sequences of VM bytecode instructions.
*   **Error Handling:** Report errors during bytecode generation if IR cannot be translated or if there are format issues.

## 4. Virtual Machine (VM) Design with JIT Compilation

This is the core runtime component. It can operate in two modes: interpreter or JIT-compiled VM. The request implies a JIT-compiled VM for performance.
The VM is the primary execution engine for Cora bytecode.
*   **VM Architecture:**
    * **Register-Based:** To match the current `BCInstruction` design which uses register indices (`regs`). Register-based VMs typically require fewer instructions than stack-based VMs for the same task.
    * **Dispatch:** Computed goto (if supported by compiler) or a highly optimized switch-dispatch loop.
    * **Frame Management:** Each function call creates a new stack frame containing local registers.
    *   **Execution Stack:** For local variables, function arguments, and return addresses.
    *   **Heap:** For dynamic memory allocation (managed by the GC).
    *   **Instruction Pointer (IP):** Points to the current bytecode instruction being executed.
    *   **Call Stack:** To manage function calls, storing return addresses and stack frames.
*   **Interpreter Mode (Optional/Fallback):**
    *   A simple loop that fetches, decodes, and executes bytecode instructions one by one. Useful for debugging or simpler scenarios.
*   **JIT Compiler for VM Bytecode:**
    *   **Purpose:** To compile frequently executed bytecode sequences into native machine code on-the-fly.
    *   **Strategy:**
        *   **On-Demand Compilation:** Compile bytecode functions or hot paths when they are first encountered or called frequently.
        *   **Tiered Compilation:** Start with interpretation, and once a function/path shows high usage, compile it. Later, potentially re-compile with more aggressive optimizations.
        *   **Intermediate Representation:** The VM might use its own internal IR (different from the compiler's SSA IR) to represent bytecode before JITting to machine code.
        *   **Machine Code Generation:** Use OS-specific APIs to allocate executable memory and emit native machine code.
        *   **Function Cache:** Store pointers to compiled native code.
    *   **Optimizations:** Leverage VM-level optimizations like inlining (if applicable), efficient register allocation for native code, and potentially profiling-guided JIT optimizations.
*   **Memory Management:**
    *   The VM will interface with the `RuntimeMemoryManager` for heap allocations.
    *   Manage the VM's execution stack.
*   **Extensibility:**
    *   Design the instruction set and VM architecture to allow adding new instructions or features.
    *   Allow plugging in different JIT backends or optimization strategies.
*   **Error Handling:**
    *   **VM Execution Errors:** Detect and report runtime errors during bytecode execution (e.g., stack overflow, division by zero, invalid memory access, type errors).
    *   **JIT Compilation Errors:** Handle failures during the JIT compilation of bytecode.
*   **Logging:** Log VM execution flow, function calls, errors, and JIT compilation events.
*   **Profiling:**
    *   **Instrumentation:** Add hooks in the bytecode interpreter and JIT compiler to record execution counts, time spent in functions/basic blocks, branch prediction accuracy, etc.
    *   **Data Collection:** Store profiling data.
    *   **Analysis:** Provide tools or interfaces to analyze profiling data to guide optimizations (e.g., for recompilation in tiered JIT).

## Embedding and API

*   The `CoraEngine` will manage the `BytecodeGenerator` and the `VirtualMachine`.
*   Provide methods to:
    *   Compile Cora to bytecode.
    *   Load bytecode into the VM.
    *   Execute bytecode functions.
    *   Access profiling data.
    *   Configure VM execution (e.g., interpreter vs. JIT mode).
    *   Retrieve compilation and execution logs/errors.
*   **API Design Principles:**
    *   **Simplicity:** Offer high-level functions for common tasks (e.g., `compileAndRun(source_code)`).
    *   **Flexibility:** Allow lower-level access for advanced users (e.g., direct bytecode loading, VM configuration).
    *   **Clarity:** Use clear naming conventions and provide comprehensive documentation (as detailed in `architecture.md`).
    *   **Safety:** Integrate robust error handling and logging.

## Modularity and Extensibility

*   Each optimization pass is a distinct module.
*   The Bytecode Generator can be adapted for different IRs or bytecode formats.
*   The VM's instruction set, interpreter, and JIT compiler are designed for extensibility. New opcodes, host functions, or JIT strategies can be added.
*   The `CoraEngine` can be extended to manage multiple compiled modules or different VM configurations.

## Testing and Benchmarking Strategy

*   **Unit Tests:** Comprehensive unit tests for each component:
    *   **Lexer/Parser:** Test grammar rules, tokenization, and error reporting.
    *   **Semantic Analyzer:** Test type checking, name resolution, scope rules, and error reporting.
    *   **IR Generator:** Test translation from AST to IR for various Cora constructs.
    *   **Optimization Passes:** Test each pass individually on representative IR snippets.
    *   **Bytecode Generator:** Test IR to bytecode translation.
    *   **VM Interpreter:** Test all opcodes and control flow.
    *   **VM JIT Compiler (Conceptual):** Test JIT compilation of bytecode to native code.
*   **Integration Tests:** Test the entire pipeline from Cora source to VM execution.
*   **Error Handling Tests:** Verify that all types of errors (lexical, syntax, semantic, VM execution, JIT errors) are correctly detected, reported, and logged.
*   **Performance Benchmarks:**
    *   Measure execution time of compiled Cora code using the VM (interpreter vs. JIT).
    *   Benchmark different optimization levels.
    *   Compare performance against native compilation or other JIT solutions where applicable.
    *   Profile frequently executed code paths to guide further JIT optimizations.
    *   Use standard benchmarking libraries or custom timers.
