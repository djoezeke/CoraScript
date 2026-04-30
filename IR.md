# 1. Textual IR Serialization (LLVM-style)
```cpp
// Example output:
// define @my_func {
// entry:
//   %0 = load_const 10.5
//   %1 = load_const 2.0
//   %2 = add %0, %1
//   br label %exit
// exit:
//   ret %2
// }

class IRTextWriter {
public:
    static std::string dumpModule(const IRModule& module) {
        std::stringstream ss;
        for (const auto& func : module.functions) {
            ss << "define @" << func.name << " {\n";
            for (const auto& bb : func.blocks) {
                ss << bb.name << ":\n";
                for (const auto& inst : bb.instructions) {
                    ss << "  " << dumpInstruction(inst) << "\n";
                }
            }
            ss << "}\n\n";
        }
        return ss.str();
    }

private:
    static std::string dumpInstruction(const IRInstruction& inst) {
        std::string out = "%" + std::to_string(inst.dest) + " = ";
        switch (inst.op) {
            case IROp::Add:  return out + "add %" + std::to_string(inst.src1) + ", %" + std::to_string(inst.src2);
            case IROp::Load: return out + "load_k " + std::to_string(inst.src1);
            case IROp::Phi:  return out + "phi [" + std::to_string(inst.src1) + ", " + std::to_string(inst.src2) + "]";
            default: return "unknown";
        }
    }
};
```
```cpp
class IRTextEncoder {
public:
    static std::string encode(const std::vector<IRInstruction>& program) {
        std::string output;
        for (const auto& inst : program) {
            output += encodeInstruction(inst) + "\n";
        }
        return output;
    }

private:
    static std::string encodeInstruction(const IRInstruction& inst) {
        std::string res = "%" + std::to_string(inst.dest) + " = ";
        switch (inst.op) {
            case IROp::Add: 
                return res + "add i64 %" + std::to_string(inst.src1) + ", %" + std::to_string(inst.src2);
            case IROp::LoadConst: 
                return res + "load_k [" + std::to_string(inst.src1) + "]";
            case IROp::Phi:
                return res + "phi [%" + std::to_string(inst.src1) + ", block_" + std::to_string(inst.src2) + "]";
            case IROp::Ret:
                return "ret %" + std::to_string(inst.src1);
            default: return "unknown_op";
        }
    }
};
```

# 2. Binary IR Serialization
Binary serialization focuses on safety and speed. You should use a "Magic Number" to identify the file type and versioning to handle breaking changes in your IR definition.

The File Schema
Header: Magic (0x4D594952), Version, Flags.

String Table: Unique names for functions/labels.

Instruction Stream: Packed IRInstruction structs.
```cpp
struct IRFileHeader {
    uint32_t magic = 0x4D594952; // "MYIR"
    uint32_t version = 1;
    uint64_t instructionCount;
};

class IRBinarySerializer {
public:
    static void serialize(const std::string& filename, const std::vector<IRInstruction>& instructions) {
        std::ofstream file(filename, std::ios::binary);
        
        IRFileHeader header;
        header.instructionCount = instructions.size();
        
        // Write Header
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        
        // Write Instructions
        // Note: Using a single write call for the entire buffer is extremely fast
        file.write(reinterpret_cast<const char*>(instructions.data()), 
                   instructions.size() * sizeof(IRInstruction));
    }

    static std::vector<IRInstruction> deserialize(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        
        IRFileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(header));
        
        if (header.magic != 0x4D594952) throw std::runtime_error("Invalid IR file");

        std::vector<IRInstruction> instructions(header.instructionCount);
        file.read(reinterpret_cast<char*>(instructions.data()), 
                  header.instructionCount * sizeof(IRInstruction));
        
        return instructions;
    }
};
```
```cpp
#include <fstream>

class IRBinaryCodec {
public:
    static void encodeToFile(const std::string& path, const std::vector<IRInstruction>& insts) {
        std::ofstream file(path, std::ios::binary);
        
        uint32_t magic = 0x4D594952;
        uint32_t version = 1;
        uint64_t count = insts.size();

        file.write(reinterpret_cast<char*>(&magic), 4);
        file.write(reinterpret_cast<char*>(&version), 4);
        file.write(reinterpret_cast<char*>(&count), 8);
        
        // Write entire buffer in one call
        file.write(reinterpret_cast<const char*>(insts.data()), count * sizeof(IRInstruction));
    }

    static std::vector<IRInstruction> decodeFromFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        
        uint32_t magic, version;
        uint64_t count;
        
        file.read(reinterpret_cast<char*>(&magic), 4);
        if (magic != 0x4D594952) throw std::runtime_error("Not a MyLang IR file");
        
        file.read(reinterpret_cast<char*>(&version), 4);
        file.read(reinterpret_cast<char*>(&count), 8);

        std::vector<IRInstruction> insts(count);
        file.read(reinterpret_cast<char*>(insts.data()), count * sizeof(IRInstruction));
        
        return insts;
    }
};
```

# 3. Key Considerations for SSA IR Serialization
Instruction Width: Keep your IRInstruction struct 64-bit aligned. Modern CPUs process aligned memory significantly faster.
```cpp
struct alignas(8) IRInstruction {
    uint8_t op;      // 1 byte
    uint8_t _pad;    // 1 byte
    uint32_t dest;   // 4 bytes
    uint32_t src1;   // 4 bytes
    uint32_t src2;   // 4 bytes
    // Total 14 bytes + 2 bytes padding = 16 bytes
};
```
The Phi Node Problem: In text format, Phi nodes are easy to represent. In binary, src1 and src2 often refer to the value and the incoming basic block ID. Ensure your binary decoder can map these block IDs back to actual pointers in memory.

Symbolic References: When serializing to file, you cannot store C++ pointers. You must convert pointers (to BasicBlocks or Functions) into integer indices (IDs) during encoding and reconstruct the pointers during decoding (swizzling).
