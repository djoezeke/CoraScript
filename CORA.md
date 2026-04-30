## 1 Compiler Architecture

## 1.1 Pipeline

```text
Source
  -> Lexer (tokens)
  -> Parser (AST)
  -> Semantic passes (symbol/type checks)
  -> IR Lowering
  -> IR Optimization
  -> Bytecode Generation
  -> Bytecode Optimization
  -> VM runtime execution
```

## 1.2 Frontend (`src/Parser` + `src/AST`)

- Lexer: tokenizes source, tracks line/column/file offsets.
- Parser: builds AST, expression precedence, statement blocks, declarations.
- AST: immutable/enforced ownership model for nodes where practical.

### Suggested units

- `Lexer.*`, `Token.*`, `Parser.*`
- `ASTExpr.*`, `ASTStmt.*`, `ASTode.*`

## 1.3 Semantic (`src/Semantic`)

- Symbol table and scope nesting.
- Type inference/checking rules.
- Control-flow semantic checks.
- Produces typed AST or directly annotated IR inputs.

## 1.4 IR (`src/IR`)

Structure: Every instruction is both a Value and a User.
Key Classes: Function, BasicBlock, and instructions like Binary, Load, Store, Call, Branch, and Phi.
SSA Property: Use-def chains allow every Value to track exactly who uses it via an intrusive use_list.

**Files:**
- `src/IR/IRInstruction.hpp` 

DCE: Deletes instructions with no users and no side effects.
GVN/CSE: Uses a Dominator Tree and Expression Hashing to eliminate redundant calculations.
LICM: Hoists loop-invariant code to a pre-header block. 

**Files:**
- `src/IR/IROptimizer.hpp` 

Build Instructions from AST.

**Files:**
- `src/IR/IRBuilder.*` 

#### Encoding/decoding

Write SSA IR Optimized Instructions to stream or file.
**Files:**
- `src/IR/IRWriter.*` 

Read/Load SSA IR Instructions from IR Code to stream or file.
- `src/IR/IRReader.*` 

## 1.4 Bytecode (`src/Bytecode`)

Structure Every instruction.
Key Classes: Function, BasicBlock, and instructions like Binary, Load, Store, Call, Convert, and unary.

### Key Operations:
- Arithmetic: ADD, SUB, MUL, DIV, MOD
- Comparisons: EQ, NE, LT, LE, GT, GE
- Control flow: JUMP, JUMP_IF_FALSE, JUMP_IF_TRUE
- Memory: LOAD_GLOBAL, STORE_GLOBAL, LOAD_FAST, STORE_FAST
- Objects: LOAD_ATTR, STORE_ATTR, BUILD_OBJECT

- `src/Bytecode/BCInstruction.hpp` 

Register Allocation: Linear Scan maps infinite SSA virtual registers to a finite set of VM registers 
PHI Lowering: Converts PhiNode logic into Move instructions in predecessor blocks before register allocation.
Bytecode Encoder: Flattens the Control Flow Graph (CFG) and replaces pointers with relative byte offsets

Build Bytecode Instructions from SSA IR.

**Files:**
- `src/Bytecode/BCBuilder.*` 

#### Encoding/decoding

Write Bytecode Instructions to stream or file.
**Files:**
- `src/Bytecode/BCWriter.*` 

Read/Load Bytecode Instructions from Bytecode bytes to stream or file.
- `src/Bytecode/BCReader.*` 

Bytecode should include:

- Arithmetic, comparisons, branches, loads/stores.
- Function calls/returns.
- Object/array/string ops.


## 2 Runtime & VM Architecture

## 2.1 Runtime Core (`src/Runtime`)

- Tagged value representation (`Value`) for numbers/integers/refs.
- Heap object model for strings, arrays, functions, closures.
- GC interface and allocation APIs.
- Runtime services used by both VM code.

### 2.1.1 Garbage Collector (GC)

The `GarbageCollector` uses a mark-and-sweep algorithm for memory management:

#### Features:
- **Automatic memory management** for Objects, Strings, and Functions
- **Reference counting** for immediate cleanup of short-lived objects
- **Mark phase** identifies reachable objects from root set
- **Sweep phase** recycles unreachable memory
- **Configurable thresholds** for GC triggering

## 2.2 VM (`src/VM`)

Execution: A Fetch-Decode-Execute loop using a byte-offset Program Counter (
).
Memory: Features a shared Heap for global data and a Call Stack for local function frames.
Native Hooks: Provides a bridge for the VM to call C++ functions (like printf) by symbol name.

- 256 CPU-like registers for efficient value storage
- 30+ optimized instructions (LOAD, STORE, BINARY_OP, COMPARE, JUMP, etc.)
- Direct memory-mapped execution model
- O(1) register access with zero stack frame overhead
- Fast interpreter loop over bytecode.
- Call frame stack and upvalue/closure handling.
- Hotness tracking per loop header/call target.
- On threshold hit, invoke trace recorder.

## 3 Executable (`exe`) Architecture

Create three binaries (or two plus mode switch):

1. `corac` (compiler tool)
   - Parse + semantic + IR + bytecode generation.
   - Emits IR/bytecode/module artifacts.

2. `coravm` (runtime tool)
   - Loads IR/bytecode/modules.
   - Executes with interpreter.

3. `cora` (optional unified driver)
   - Subcommands: `build`, `run`,  `disasm`.

<!-- enhance lexer, parser and ast class to efficiently tokenize, parse ad emit ast for sample code in sample.cora. Enhace IRBuilder to constrct ir (Building Value ,User and Instruction classes),  perform Analysis , Building Dominator Trees and Liveness maps and IROptimizer for DCE, GVN, LICM, and Loop Unrolling. Enhace IRWriter and IRReader to write/read ir instrctio fro and to a stream or file.  -->