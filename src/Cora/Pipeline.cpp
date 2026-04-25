#include "Pipeline.hpp"

#include "../IRGen/IRBuilder.hpp"
#include "../IRGen/IROptimizer.hpp"
#include "../IRGen/IRPrinter.hpp"
#include "../IRGen/IRVerifier.hpp"
#include "../Parser/Parser.hpp"
#include "../Semantic/Validator.hpp"
#include "../VMachine/BytecodeEmitter.hpp"
#include "../VMachine/BytecodeReader.hpp"
#include "../VMachine/BytecodeWriter.hpp"

#include <deque>
#include <memory>
#include <utility>

namespace cora::tooling
{
    namespace
    {
        class ProgramGuard
        {
        public:
            explicit ProgramGuard(std::deque<cora::compiler::ast::Statement *> statements)
                : m_Statements(std::move(statements))
            {
            }

            ProgramGuard(const ProgramGuard &) = delete;
            ProgramGuard &operator=(const ProgramGuard &) = delete;

            ~ProgramGuard()
            {
                for (cora::compiler::ast::Statement *statement : m_Statements)
                {
                    delete statement;
                }
            }

            const std::deque<cora::compiler::ast::Statement *> &Get() const
            {
                return m_Statements;
            }

        private:
            std::deque<cora::compiler::ast::Statement *> m_Statements;
        };
    } // namespace

    FrontendResult CompileToBytecode(const std::string &source,
                                     const std::string &fileName,
                                     const FrontendOptions &options,
                                     std::ostream *irOut,
                                     std::ostream *bytecodeOut)
    {
        cora::compiler::parser::Parser parser;
        parser.SetFileName(fileName);
        parser.SetModuleName(fileName);

        ProgramGuard program(parser.ParseProgram(source));

        cora::compiler::semantic::ValidateProgram(program.Get(), fileName, fileName);

        cora::ir::IRBuilder builder;
        builder.Build(program.Get());
        std::vector<cora::ir::BasicBlock *> blocks = builder.GetBlocks();

        if (options.verify)
        {
            cora::ir::IRVerifier verifier;
            verifier.Verify(blocks);
        }

        if (options.optimize)
        {
            cora::ir::IROptimizer optimizer;
            optimizer.Optimize(blocks);
        }

        if (options.verify)
        {
            cora::ir::IRVerifier verifier;
            verifier.Verify(blocks);
        }

        FrontendResult result;

        if (options.printIR || irOut != nullptr)
        {
            cora::ir::IRPrinter printer;
            result.irText = printer.Print(blocks);
            if (options.printIR && irOut != nullptr)
            {
                *irOut << result.irText;
            }
        }

        cora::vmachine::BytecodeEmitter emitter;
        result.program = emitter.Emit(blocks);

        if (options.printBytecode && bytecodeOut != nullptr)
        {
            PrintBytecodeProgram(result.program, *bytecodeOut);
        }

        if (!options.bytecodeOutputFile.empty())
        {
            SaveBytecodeFile(options.bytecodeOutputFile, result.program);
        }

        return result;
    }

    cora::vmachine::BytecodeProgram LoadBytecodeFile(const std::string &filePath)
    {
        cora::vmachine::BytecodeReader reader;
        reader.Read(filePath);
        return reader.GetProgram();
    }

    void SaveBytecodeFile(const std::string &filePath, const cora::vmachine::BytecodeProgram &program)
    {
        cora::vmachine::BytecodeWriter writer;
        writer.Write(filePath, program);
    }

    void PrintBytecodeProgram(const cora::vmachine::BytecodeProgram &program, std::ostream &out)
    {
        out << "constants(" << program.constants.size() << ")\n";
        for (std::size_t index = 0; index < program.constants.size(); ++index)
        {
            out << "  [" << index << "] " << program.constants[index] << '\n';
        }

        out << "names(" << program.names.size() << ")\n";
        for (std::size_t index = 0; index < program.names.size(); ++index)
        {
            out << "  [" << index << "] " << program.names[index] << '\n';
        }

        out << "code(" << program.code.size() << ")\n";
        for (std::size_t ip = 0; ip < program.code.size(); ++ip)
        {
            const cora::vmachine::Instruction &instruction = program.code[ip];
            out << "  " << ip << ": "
                << cora::vmachine::ToString(instruction.op)
                << " a=" << instruction.a
                << " b=" << instruction.b
                << '\n';
        }
    }

} // namespace cora::tooling
