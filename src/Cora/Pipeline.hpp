#ifndef CORA_CORA_PIPELINE_HPP
#define CORA_CORA_PIPELINE_HPP

#include "../VMachine/Bytecode.hpp"

#include <iosfwd>
#include <string>

namespace cora::tooling
{
    struct FrontendOptions
    {
        bool optimize{true};
        bool verify{true};
        bool printIR{false};
        bool printBytecode{false};
        std::string bytecodeOutputFile;
    };

    struct FrontendResult
    {
        cora::vmachine::BytecodeProgram program;
        std::string irText;
    };

    FrontendResult CompileToBytecode(const std::string &source,
                                     const std::string &fileName,
                                     const FrontendOptions &options,
                                     std::ostream *irOut = nullptr,
                                     std::ostream *bytecodeOut = nullptr);

    cora::vmachine::BytecodeProgram LoadBytecodeFile(const std::string &filePath);
    void SaveBytecodeFile(const std::string &filePath, const cora::vmachine::BytecodeProgram &program);
    void PrintBytecodeProgram(const cora::vmachine::BytecodeProgram &program, std::ostream &out);

} // namespace cora::tooling

#endif // CORA_CORA_PIPELINE_HPP