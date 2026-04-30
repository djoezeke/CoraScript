Designing a complete interpreted language with a register-based Virtual Machine (VM), Static Single Assignment (SSA) Intermediate Representation (IR), and a Just-In-Time (JIT) compiler is a massive and incredibly rewarding engineering undertaking.

To achieve this in modern C++, you will want to heavily leverage tools like std::variant (for dynamic typing/AST nodes), std::string_view (for zero-copy lexing), and smart pointers for memory management within the compiler itself.

Here is a comprehensive architectural blueprint to guide your development.

# 1. The Pipeline Architecture
Your language will execute in distinct phases, allowing you to halt, serialize, or resume the pipeline at multiple points (Source, IR, or Bytecode).Source Code (.src) $\rightarrow$ Lexer $\rightarrow$ std::vector<Token>Tokens $\rightarrow$ Parser $\rightarrow$ Abstract Syntax Tree (AST)AST $\rightarrow$ Semantic Analyzer $\rightarrow$ Annotated AST (Scope, Type/Variable resolution)Annotated AST $\rightarrow$ IR Builder $\rightarrow$ SSA IR (In-memory representation)SSA IR $\rightarrow$ IR Writer/Reader $\rightarrow$ .ir file (Human-readable or binary IR)SSA IR $\rightarrow$ Bytecode Builder $\rightarrow$ Flat array of Bytecode InstructionsBytecode $\rightarrow$ Bytecode Writer/Reader $\rightarrow$ .bc file (Binary executable)Bytecode $\rightarrow$ Decoder $\rightarrow$ VM (Interpreter) OR JIT Compiler $\rightarrow$ Machine Code execution.

# 2. Frontend Core (Source to AST)
Lexer (Lexer): Scans a std::string_view of the source code. It emits a stream of Token structs containing the token type, line/column number, and lexeme slice.

Parser (Parser): A Recursive Descent parser is recommended. It consumes tokens and builds the AST.

Design: Use a base ASTNode class, with derived classes for BinaryExpr, LetStmt, CallExpr, etc. Use std::unique_ptr<ASTNode> to manage tree ownership.

Semantic Analyzer (SemanticAnalyzer): Walks the AST using the Visitor pattern. It builds symbol tables, checks for undefined variables, and verifies scoping rules before IR generation.

# 3. Middle-End Core (SSA IR)
SSA form means every variable is assigned exactly once, making optimizations (like constant folding and dead code elimination) trivial.

IR Architecture:

Module: Contains multiple Functions.

Function: Contains multiple BasicBlocks.

BasicBlock: Contains a sequential list of Instructions and ends with a terminator (branch/return).

Instruction: Represents operations (e.g., Add, Phi, Load, Store).

IRBuilder: Provides a clean API (builder.CreateAdd(lhs, rhs)) to generate these structures while visiting the AST. It automatically handles basic block insertion.

IRWriter / IRReader: Serializes the in-memory SSA graph to a text format (similar to LLVM IR) so you can save it to an .ir file, modify it by hand, and load it back into memory.

# 4. Backend Core (Bytecode & VM)
A register-based VM is faster than a stack-based VM because it reduces instruction dispatch overhead, though the instructions are larger.

Bytecode Format (Instruction): Use a 32-bit or 64-bit integer for each instruction.

Example 32-bit layout: [Opcode: 8 bits] [Reg A: 8 bits] [Reg B: 8 bits] [Reg C/Immediate: 8 bits].

Operation: ADD r1, r2, r3 (adds r2 and r3, stores in r1).

BytecodeBuilder: Flattens the SSA IR. It performs Register Allocation (mapping infinite SSA variables to a finite set of VM registers, e.g., 256 per frame).

Encoder / Decoder: Translates logical instruction objects into the 32-bit integer array and vice versa.

BytecodeWriter / BytecodeReader: Dumps the 32-bit integers and the constant pool (strings, numbers) to a .bc file.

The VM Core (RegisterVM):

CallFrame: Contains the Instruction Pointer (IP), a pointer to the current function's constant pool, and a window into the register array.

Value: The fundamental data type. Use NaN-boxing (using the bit representation of double-precision floats to pack pointers/integers) or a std::variant<double, int64_t, bool, Object*> for safety.

Dispatch Loop: A giant switch statement over the decoded opcode, or a computed goto table for maximum performance.

# 5. Memory Management (Garbage Collector)
Since you are writing an interpreter, you need a GC for dynamic memory (strings, arrays, objects).

Implementation: A simple Mark-and-Sweep GC is best to start.

Mechanics: All heap-allocated objects inherit from a GCObject struct containing a marked boolean.

Trigger: When allocations hit a threshold, pause execution.

Mark: Start at the VM's roots (current registers in all active call frames, globals) and recursively set marked = true on reachable objects.

Sweep: Iterate through an internal linked list of all allocated objects; free those where marked == false.

# 6. FFI & Dynamic Libraries in C++
To easily bind C++ functions and load .so / .dll files:

FFI Registry: Create a BuiltinRegistry map matching strings to C++ function pointers (e.g., std::function<Value(std::vector<Value>&)>).

Dynamic Loading: Provide a standard library function in your language (e.g., import_lib("math.so")).

Under the hood, use POSIX dlopen() and dlsym() (or Windows LoadLibrary() and GetProcAddress()) to load a C-ABI exported initialization function from the shared library.

This init function will take a pointer to your VM's BuiltinRegistry and register its own functions into the VM's global scope.

# 7. The JIT Compiler
Translating your bytecode to machine code at runtime yields massive performance gains.

Library choice: Do not write x86/ARM emitters by hand initially. Use AsmJit (a lightweight C++ JIT library) or LLVM (heavy, but industry standard).

Execution: Instead of the VM interpreting the bytecode block, the JIT iterates over the bytecode (or SSA IR) and emits native machine instructions to a memory page marked with execution privileges (mprotect). It then casts that memory to a C++ function pointer and executes it.

```
mylang/
├── .clang-format               # Rules for consistent code style (e.g., LLVM or Google style)
├── .clang-tidy                 # Static analysis rules to catch bugs and enforce modern C++
├── .gitignore                  # Standard ignore file for build/ and IDE folders
├── CMakeLists.txt              # Top-level CMake build script
├── README.md                   # Project documentation
│
├── external/                   # Third-party dependencies (git submodules or FetchContent)
│   ├── asmjit/                 # JIT compilation library
│   ├── fmt/                    # Modern string formatting (std::format polyfill)
│   └── cxxopts/                # Command-line argument parser
│
├── include/                    # Public headers (if exposing your VM as a library)
│   └── mylang/                 
│       ├── Compiler.h          # API to compile strings/files to Bytecode
│       └── VM.h                # API to initialize and embed the VM in other C++ apps
│
├── src/                        # Internal implementation and private headers
│   ├── CMakeLists.txt          # Library build targets
│   │
│   ├── frontend/               # Phase 1 & 2: Source to AST
│   │   ├── Lexer.h / .cpp
│   │   ├── Parser.h / .cpp
│   │   ├── AST.h               # AST Node definitions (usually header-only or mostly headers)
│   │   └── Semantic.h / .cpp   # Scope resolution and type checking
│   │
│   ├── middle/                 # Phase 3: AST to SSA IR
│   │   ├── IR.h / .cpp         # BasicBlocks, Instructions, Functions
│   │   ├── IRBuilder.h / .cpp  # API to construct the SSA graph
│   │   └── IRIO.h / .cpp       # IR Reader/Writer (.ir files)
│   │
│   ├── backend/                # Phase 4: SSA IR to Bytecode/JIT
│   │   ├── Bytecode.h          # Opcode enums and Instruction struct
│   │   ├── BytecodeGen.h / .cpp# Flattens IR to Bytecode (Register Allocation)
│   │   ├── BytecodeIO.h / .cpp # .bc file Reader/Writer
│   │   └── JIT.h / .cpp        # Translates Bytecode/IR to Machine Code via AsmJit
│   │
│   └── runtime/                # Phase 5: Execution
│       ├── VM.h / .cpp         # The main execution loop (Dispatch)
│       ├── Value.h / .cpp      # The core data type (NaN-boxing or std::variant)
│       ├── GC.h / .cpp         # Mark-and-sweep Garbage Collector
│       ├── Builtins.h / .cpp   # Standard library functions (print, math)
│       └── FFI.h / .cpp        # Dynamic library loading (.so/.dll)
│
├── tools/                      # Executables that link against your library
│   ├── CMakeLists.txt
│   ├── cli.cpp                 # Main executable (e.g., `mylang run script.src`)
│   └── repl.cpp                # Interactive Read-Eval-Print-Loop
│
├── tests/                      # Unit and integration tests
│   ├── CMakeLists.txt          # Test build targets
│   ├── frontend_test.cpp       # Catch2 or GTest for Lexer/Parser
│   ├── ir_test.cpp             # Tests for SSA generation
│   ├── vm_test.cpp             # Tests for bytecode execution
│   └── test_scripts/           # Folder containing .src files with expected outputs
│
└── docs/                       # Language spec, architecture diagrams, Doxygen config
    └── language_spec.md
```

# 1. Project Directory Structure
```
mylang/
├── CMakeLists.txt              # Root build configuration
├── include/                    # Public API for embedding
│   └── mylang/
│       ├── compiler.hpp        # Main compilation interface
│       └── vm.hpp              # Main execution interface
├── src/                        # Private Implementation
│   ├── frontend/               # Lexing, Parsing, AST
│   ├── middle/                 # SSA IR and Optimizations
│   ├── backend/                # Bytecode and JIT
│   ├── runtime/                # VM, GC, and Types
│   └── util/                   # Common logging and buffers
├── tools/
│   ├── mylang_cli/             # Command line driver
│   └── mylang_repl/            # Interactive REPL
├── tests/                      # Unit and Integration tests
└── external/                   # AsmJit, fmt, etc.
```

# 2. Pipeline Architecture
The data flows linearly through these stages. Each stage is designed to be independently testable.

Source Code (std::string_view)

Lexer: Produces a stream of Token objects.

Parser: Consumes Tokens, produces an AST.

Semantic Analyzer: Validates the AST, resolves symbols, and checks types.

IR Builder: Lowers the AST into a linear SSA IR graph.

Optimizer (Optional): Operates on the SSA IR (e.g., Dead Code Elimination).

Bytecode Generator: Maps SSA variables to registers and flattens instructions.

VM Dispatcher: Executes the bytecode instructions.

JIT Engine (Optional): Compiles hot bytecode paths into machine code using AsmJit.

# 3. Class Hierarchy & Core Components
A. Frontend (Source to AST)
Lexer: Scans source code; maintains line/column state.

Parser: Implements recursive descent or Pratt parsing.

ASTNode (Base Class):

ExprAST: Base for LiteralExpr, BinaryExpr, VariableExpr, CallExpr.

StmtAST: Base for LetStmt, IfStmt, ReturnStmt, FunctionStmt.

B. Middle-End (SSA IR)
IRValue: The base for everything in the IR (Instructions, Constants, Arguments).

Instruction: Base for SSA operations.

BinaryInst, BranchInst, PhiInst, LoadInst.

BasicBlock: A container for a sequence of Instructions ending in a terminator.

Function: A collection of BasicBlocks representing a callable unit.

IRModule: The top-level container for global variables and functions.

C. Backend (Bytecode & VM)
BytecodeBuilder: Converts IRModule to a stream of 64-bit opcodes.

RegisterVM: The execution engine.

CallFrame: Manages the stack of registers and return pointers for a function call.

Decoder: Interprets raw bytecode into internal VM actions.

GarbageCollector: A MarkAndSweep manager for heap-allocated Value types.

D. FFI & Integration
FFIManager: Handles dlopen/dlsym and maps C++ function pointers to language-level symbols.

NativeFunction: A specialized Value type that wraps a C++ lambda or function pointer.

# 4. The Data Model (The Value Type)
For the VM to be fast, we use a union-based or NaN-boxed Value type.

```cpp
// src/runtime/value.hpp
struct Value {
    enum class Type { Nil, Bool, Int, Float, Object };
    Type type;
    union {
        bool b;
        int64_t i;
        double f;
        GCObject* obj;
    } as;
};
```

# 5. Persistence (Readers & Writers)
The project includes three distinct formats for storage:FormatClassPurposeSourceLexer/ParserHuman-written .mylang files.
IRIRWriter/IRReaderHuman-readable .ir text for debugging optimizations.
BytecodeBCWriter/BCReaderBinary .mbc files for fast loading and execution.

# 6. Extension API (FFI)
To allow C++ libraries to work with the language:

Define C++ Function:
```cpp
Value MyCppFunc(VM& vm, std::span<Value> args) {
    return Value(args[0].as.i + 10);
}
```

Register with VM:
```cpp
vm.registerFunction("addTen", MyCppFunc);
```

Dynamic Loading:
The FFIManager will look for .so or .dll files containing an extern "C" void Init(VM* vm) function to register multiple bindings at once.

# Boilerplate

## 1. Frontend: Lexer & Parser
We use std::string_view for the Lexer to avoid unnecessary allocations and std::variant for the AST to keep nodes lightweight.

```cpp
#include <iostream>
#include <vector>
#include <string_view>
#include <variant>
#include <memory>

// --- Lexer Components ---
enum class TokenType { Let, Identifier, Equal, Number, Plus, Semicolon, EOF_TK };

struct Token {
    TokenType type;
    std::string_view lexeme;
};

class Lexer {
    std::string_view source;
    size_t cursor = 0;
public:
    explicit Lexer(std::string_view src) : source(src) {}
    
    Token nextToken() {
        skipWhitespace();
        if (cursor >= source.size()) return {TokenType::EOF_TK, ""};
        
        char c = source[cursor];
        if (isdigit(c)) return consumeNumber();
        if (isalpha(c)) return consumeIdentifier();
        
        cursor++;
        switch (c) {
            case '=': return {TokenType::Equal, "="};
            case '+': return {TokenType::Plus, "+"};
            case ';': return {TokenType::Semicolon, ";"};
            default:  return nextToken(); // Simplified error handling
        }
    }
private:
    void skipWhitespace() { while (cursor < source.size() && isspace(source[cursor])) cursor++; }
    Token consumeNumber() {
        size_t start = cursor;
        while (cursor < source.size() && isdigit(source[cursor])) cursor++;
        return {TokenType::Number, source.substr(start, cursor - start)};
    }
    Token consumeIdentifier() {
        size_t start = cursor;
        while (cursor < source.size() && isalnum(source[cursor])) cursor++;
        auto sub = source.substr(start, cursor - start);
        if (sub == "let") return {TokenType::Let, sub};
        return {TokenType::Identifier, sub};
    }
};

// --- AST Components ---
struct BinaryExpr;
struct LiteralExpr;

using Expression = std::variant<std::unique_ptr<BinaryExpr>, std::unique_ptr<LiteralExpr>>;

struct LiteralExpr { double value; };
struct BinaryExpr {
    Expression lhs;
    TokenType op;
    Expression rhs;
};
```

## 2. Middle-End: SSA IR Builder
In SSA, every assignment creates a new versioned variable. The IRBuilder manages these versioned virtual registers.

```cpp
#include <map>
#include <string>

enum class IROp { Add, Assign, Load, Store };

struct IRInstruction {
    IROp op;
    uint32_t dest;   // SSA Virtual Register
    uint32_t src1;
    uint32_t src2;
};

class IRBuilder {
    std::vector<IRInstruction> instructions;
    uint32_t nextReg = 0;
    std::map<std::string, uint32_t> ssaMapping;

public:
    uint32_t createAdd(uint32_t lhs, uint32_t rhs) {
        uint32_t reg = nextReg++;
        instructions.push_back({IROp::Add, reg, lhs, rhs});
        return reg;
    }

    void createStore(const std::string& name, uint32_t valueReg) {
        ssaMapping[name] = valueReg; // Map var name to the latest SSA register
    }

    const std::vector<IRInstruction>& getInstructions() const { return instructions; }
};
```

## 3. Backend: Register-Based VM Loop
This VM uses a flat array of Value objects as registers. For performance, the dispatch loop uses a switch (which compilers often optimize into jump tables).

```cpp
#include <array>

union Value {
    double f64;
    int64_t i64;
};

enum class OpCode : uint8_t {
    OP_ADD,     // [Op][Dest][Src1][Src2]
    OP_LOAD_K,  // [Op][Dest][ConstIdx]
    OP_HALT
};

struct BytecodeInst {
    OpCode op;
    uint8_t a, b, c;
};

class RegisterVM {
    std::array<Value, 256> registers; // Register file for the current frame
    std::vector<Value> constants;
    std::vector<BytecodeInst> code;
    size_t ip = 0;

public:
    void run() {
        while (true) {
            const auto& inst = code[ip++];
            switch (inst.op) {
                case OpCode::OP_ADD:
                    registers[inst.a].f64 = registers[inst.b].f64 + registers[inst.c].f64;
                    break;
                case OpCode::OP_LOAD_K:
                    registers[inst.a] = constants[inst.b];
                    break;
                case OpCode::OP_HALT:
                    return;
            }
        }
    }

    void loadBytecode(std::vector<BytecodeInst> bc, std::vector<Value> k) {
        code = std::move(bc);
        constants = std::move(k);
    }
};
```
# Integrating a Foreign Function Interface (FFI)

Integrating a Foreign Function Interface (FFI) allows your language to call C or C++ functions and vice versa. In a register-based VM, this is usually achieved by mapping a special opcode or a specific Value type to a native function pointer.

## 1. The Native Function Interface
To make C++ functions callable from your VM, define a standard signature. Using std::span (C++20) is ideal for passing a slice of the VM registers as arguments.

```cpp
#include <span>
#include <functional>

// Standard signature for a native function
// Returns a Value, takes a view of the registers starting at the first argument
using NativeFn = Value(*)(RegisterVM& vm, std::span<Value> args);

struct NativeFunction {
    std::string name;
    NativeFn func;
    int arity; // Number of expected arguments
};
```

## 2. Extending the Value System
The Value union must be updated to store pointers to these native functions. This allows your language to treat native functions as "first-class citizens" (you can pass them to other functions).
```cpp
struct GCObject {
    enum class ObjType { String, NativeFunc, UserFunc };
    ObjType type;
    bool marked; // For GC
};

struct NativeFuncObj : public GCObject {
    NativeFn func;
    int arity;
    
    NativeFuncObj(NativeFn f, int a) {
        type = ObjType::NativeFunc;
        func = f;
        arity = a;
    }
};

// Inside Value union/variant:
// GCObject* obj;
```

## 3. The VM Call Logic
When the VM encounters a CALL opcode, it checks if the object is a NativeFuncObj. If it is, it bypasses the bytecode dispatcher and calls the C++ function directly.
```cpp
case OpCode::OP_CALL: {
    uint8_t destReg = inst.a;
    uint8_t funcReg = inst.b;
    uint8_t argStart = inst.c;

    Value val = registers[funcReg];
    if (val.isObjType(ObjType::NativeFunc)) {
        auto* native = static_cast<NativeFuncObj*>(val.as.obj);
        
        // Pass a slice of registers as arguments
        std::span<Value> args(&registers[argStart], native->arity);
        
        // Execute C++ logic and store result back in register
        registers[destReg] = native->func(*this, args);
    }
    break;
}
```
## 4. Dynamic Library Loading (FFI Manager)
To load .so or .dll files at runtime, you use an FFIManager class. This allows your language to extend itself without recompiling the core VM.
```cpp
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

class FFIManager {
    std::vector<void*> handles;

public:
    void loadLibrary(const std::string& path, RegisterVM& vm) {
#ifdef _WIN32
        void* handle = LoadLibraryA(path.c_str());
        auto initFunc = (void(*)(RegisterVM&))GetProcAddress((HMODULE)handle, "InitPlugin");
#else
        void* handle = dlopen(path.c_str(), RTLD_NOW);
        auto initFunc = (void(*)(RegisterVM&))dlsym(handle, "InitPlugin");
#endif
        if (initFunc) {
            initFunc(vm); // The library registers its own functions into the VM
            handles.push_back(handle);
        }
    }

    ~FFIManager() {
        for (void* h : handles) {
#ifdef _WIN32
            FreeLibrary((HMODULE)h);
#else
            dlclose(h);
#endif
        }
    }
};
```

## 5. Example C++ Plugin
This is how a developer would write an extension for your language in a separate C++ project:
```cpp
// math_plugin.cpp
extern "C" void InitPlugin(RegisterVM& vm) {
    auto sinWrapper = [](RegisterVM& vm, std::span<Value> args) -> Value {
        return Value(std::sin(args[0].as.f64));
    };
    
    vm.defineGlobal("sin", new NativeFuncObj(sinWrapper, 1));
}
```

## 6. Key Considerations
Marshaling: Moving data between your VM and C++ (e.g., converting your language's String object to a std::string) can be expensive. For high-performance FFI, minimize data copying.

Safety: Native functions can crash the VM if they access invalid memory. In a clean codebase, wrap native calls in a try-catch block if your VM supports language-level exceptions.

ABI: Using extern "C" is mandatory for the loading entry point to prevent C++ name mangling, ensuring dlsym can find your "InitPlugin" function.

# Serialization/Deserialization

To bridge the gap between high-level SSA IR and execution-ready Bytecode, you need robust serialization (encoding) and deserialization (decoding) logic. This allows you to save the state of your compilation at any point.

## 1. IR Serialization (SSA IR ↔ File)
Since SSA IR is typically for debugging and optimization, it’s best to support a Textual Format (similar to LLVM IR) and a Binary Format for speed.
```cpp
#include <iostream>
#include <vector>
#include <sstream>

// --- IR Instruction (SSA) ---
struct IRInstruction {
    IROp op;
    uint32_t dest;   // Target Virtual Register
    uint32_t src1;   // Operand 1
    uint32_t src2;   // Operand 2 (optional)
};

class IRCodec {
public:
    // ENCODE: To Human-Readable String
    static std::string toString(const IRInstruction& inst) {
        std::stringstream ss;
        ss << "%" << inst.dest << " = ";
        switch(inst.op) {
            case IROp::Add: ss << "add %" << inst.src1 << ", %" << inst.src2; break;
            case IROp::Load: ss << "load_k [" << inst.src1 << "]"; break;
            // ...
        }
        return ss.str();
    }

    // ENCODE: To Binary File
    static void writeBinary(std::ostream& os, const std::vector<IRInstruction>& instructions) {
        size_t size = instructions.size();
        os.write(reinterpret_cast<const char*>(&size), sizeof(size));
        os.write(reinterpret_cast<const char*>(instructions.data()), size * sizeof(IRInstruction));
    }

    // DECODE: From Binary File
    static std::vector<IRInstruction> readBinary(std::istream& is) {
        size_t size;
        is.read(reinterpret_cast<char*>(&size), sizeof(size));
        std::vector<IRInstruction> insts(size);
        is.read(reinterpret_cast<char*>(insts.data()), size * sizeof(IRInstruction));
        return insts;
    }
};
```

## 2. Bytecode Serialization (VM ↔ File)
Bytecode needs to be compact. We use a fixed-width 32-bit or 64-bit word (the "Instruction") where bits are partitioned for the Opcode and Register addresses.

Instruction Layout (32-bit example)
00-07: Opcode (256 possible ops)

08-15: Register A (Destination)

16-23: Register B (Source 1)

24-31: Register C (Source 2 or Small Immediate)

```cpp
#include <cstdint>

// Compact 32-bit representation
using BCWord = uint32_t;

struct BCInstruction {
    OpCode op;
    uint8_t a, b, c;

    // PACK: Convert struct to 32-bit word
    BCWord encode() const {
        return (static_cast<uint32_t>(op))       |
               (static_cast<uint32_t>(a) << 8)  |
               (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(c) << 24);
    }

    // UNPACK: Convert 32-bit word back to struct
    static BCInstruction decode(BCWord word) {
        return {
            static_cast<OpCode>(word & 0xFF),
            static_cast<uint8_t>((word >> 8) & 0xFF),
            static_cast<uint8_t>((word >> 16) & 0xFF),
            static_cast<uint8_t>((word >> 24) & 0xFF)
        };
    }
};
```

## 3. The Full Bytecode Module (Binary File Structure)
A .bc file isn't just instructions; it needs a Header (magic number/version) and a Constant Pool (strings and numbers used in the code).

```cpp
struct BCHeader {
    uint32_t magic = 0x4D594C42; // "MYLB" (MyLang Bytecode)
    uint32_t version = 1;
    uint32_t instructionCount;
    uint32_t constantCount;
};

class BytecodeModule {
    std::vector<BCWord> code;
    std::vector<Value> constants;

public:
    void writeToFile(const std::string& path) {
        std::ofstream ofs(path, std::ios::binary);
        
        BCHeader header{0x4D594C42, 1, (uint32_t)code.size(), (uint32_t)constants.size()};
        ofs.write((char*)&header, sizeof(header));

        // 1. Write Instructions
        ofs.write((char*)code.data(), code.size() * sizeof(BCWord));

        // 2. Write Constants (Simplified: assuming all doubles for now)
        for (const auto& val : constants) {
            ofs.write((char*)&val.as.f64, sizeof(double));
        }
    }

    void loadFromFile(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        BCHeader header;
        ifs.read((char*)&header, sizeof(header));

        code.resize(header.instructionCount);
        ifs.read((char*)code.data(), header.instructionCount * sizeof(BCWord));

        constants.resize(header.constantCount);
        for (uint32_t i = 0; i < header.constantCount; ++i) {
            double d;
            ifs.read((char*)&d, sizeof(double));
            constants[i] = Value(d);
        }
    }
};
```

## Summary of the Encoder/Decoder Pipeline
IRBuilder creates IRInstruction structs.
IRCodec can save these to .ir (text/binary) for debugging.
BytecodeGen converts IRInstruction $\rightarrow$ BCInstruction.
BCInstruction::encode() packs the fields into a 32-bit BCWord.
BytecodeModule writes the header, BCWord array, and constant pool to a .bc file.

# Mapping infinite SSA registers
Mapping infinite SSA registers to a finite set of VM registers is the most complex part of the backend. Since your VM uses a register file (e.g., 256 registers), you must reuse indices efficiently to avoid "stack overflow" in your register array.

## 1. Register Allocation (Linear Scan)
For an interpreter, Linear Scan Register Allocation is the standard approach. 
It’s faster than Graph Coloring and produces sufficiently good results.
Compute Liveness: Determine the "Live Interval" for each SSA variable (from the first instruction that defines it to the last instruction that uses it).
Iterate and Assign: Walk through the instructions. 
When a variable’s life begins, assign it the lowest available physical register index ($0-255$).Expire: When a variable’s life ends, return that index to the pool of "available" registers.
```cpp
#include <set>
#include <algorithm>

struct LiveInterval {
    uint32_t ssaReg;
    size_t start;
    size_t end;
    uint8_t physicalReg = 0;
};

class RegisterAllocator {
    uint8_t maxRegisters = 255;
    std::set<uint8_t> freePool;

public:
    RegisterAllocator() {
        for (int i = 0; i < maxRegisters; ++i) freePool.insert(i);
    }

    std::map<uint32_t, uint8_t> allocate(std::vector<LiveInterval>& intervals) {
        std::map<uint32_t, uint8_t> mapping;
        
        // Sort intervals by start point
        std::sort(intervals.begin(), intervals.end(), [](auto& a, auto& b) {
            return a.start < b.start;
        });

        std::vector<LiveInterval*> active;

        for (auto& interval : intervals) {
            // 1. Expire old intervals
            auto it = active.begin();
            while (it != active.end()) {
                if ((*it)->end < interval.start) {
                    freePool.insert((*it)->physicalReg);
                    it = active.erase(it);
                } else {
                    ++it;
                }
            }

            // 2. Assign from pool
            if (freePool.empty()) {
                // Handle "Spilling" (moving a register to the stack)
                throw std::runtime_error("Out of physical registers!");
            }

            uint8_t reg = *freePool.begin();
            freePool.erase(freePool.begin());
            
            interval.physicalReg = reg;
            mapping[interval.ssaReg] = reg;
            active.push_back(&interval);
        }
        return mapping;
    }
};
```

## 2. The Bytecode Generator (Lowering IR)
The BytecodeGen class uses the mapping provided by the allocator to translate IRInstruction into BCInstruction.
```cpp
class BytecodeGenerator {
public:
    std::vector<BCWord> generate(const std::vector<IRInstruction>& irInstructions) {
        // Step 1: Run Liveness Analysis (simplified)
        auto intervals = computeLiveIntervals(irInstructions);
        
        // Step 2: Run Allocator
        RegisterAllocator allocator;
        auto regMap = allocator.allocate(intervals);

        // Step 3: Emit Bytecode
        std::vector<BCWord> bytecode;
        for (const auto& ir : irInstructions) {
            BCInstruction bc;
            bc.op = translateOp(ir.op);
            bc.a = regMap[ir.dest];
            bc.b = regMap[ir.src1];
            bc.c = regMap[ir.src2];
            
            bytecode.push_back(bc.encode());
        }
        return bytecode;
    }

private:
    OpCode translateOp(IROp irOp) {
        switch (irOp) {
            case IROp::Add: return OpCode::OP_ADD;
            // ...
        }
    }
};
```
## 3. The VM Execution Pipeline 
SummaryNow that all parts are defined, your system functions as a cohesive unit:Frontend: Parser $\rightarrow$ AST.Middle: IRBuilder $\rightarrow$ SSA IR (Infinite registers, safe for optimization).Backend (Gen): RegisterAllocator $\rightarrow$ Map SSA to 8-bit Register IDs.Backend (Emit): BytecodeGenerator $\rightarrow$ std::vector<BCWord>.Persistence: BytecodeModule $\rightarrow$ Disk (.bc).Runtime: RegisterVM $\rightarrow$ Load .bc, decode BCWord, execute in the high-speed switch loop.