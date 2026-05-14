#include "Pipeline.hpp"

#include "../IR/IRBuilder.hpp"
#include "../IR/IRPrinter.hpp"
#include "../IR/IRVerifier.hpp"
#include "../Parser/Parser.hpp"
#include "../VMachine/BytecodeEmitter.hpp"
#include "../VMachine/BytecodeReader.hpp"
#include "../VMachine/BytecodeWriter.hpp"

#include <deque>
#include <memory>
#include <utility>
#include <vector>

namespace cora::tooling
{
    namespace
    {
        class ProgramGuard
        {
        public:
            explicit ProgramGuard(std::deque<cora::ast::Statement *> statements)
                : m_Statements(std::move(statements))
            {
            }

            ProgramGuard(const ProgramGuard &) = delete;
            ProgramGuard &operator=(const ProgramGuard &) = delete;

            ~ProgramGuard()
            {
                for (cora::ast::Statement *statement : m_Statements)
                {
                    delete statement;
                }
            }

            const std::deque<cora::ast::Statement *> &Get() const
            {
                return m_Statements;
            }

        private:
            std::deque<cora::ast::Statement *> m_Statements;
        };
    } // namespace

    FrontendResult CompileToBytecode(const std::string &source,
                                     const std::string &fileName,
                                     const FrontendOptions &options,
                                     std::ostream *irOut,
                                     std::ostream *bytecodeOut)
    {
        cora::SourceManager sm;
        cora::DiagnosticEngine de;
        de.addEmitter(std::make_unique<cora::ConsoleEmitter>(sm));

        uint32_t fileID = sm.addFile(fileName, source);

        cora::parser::Parser parser(sm, de);
        parser.SetFileID(fileID);
        parser.SetModuleName(fileName);

        std::vector<cora::ast::Statement *> parsed;
        try
        {
            parsed = parser.Parse();
        }
        catch (const std::exception &e)
        {
            // Error already reported via DiagnosticEngine
            return {};
        }

        std::deque<cora::ast::Statement *> programQueue(parsed.begin(), parsed.end());
        ProgramGuard program(std::move(programQueue));

        // cora::semantic::ValidateProgram(program.Get(), fileName, fileName);

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
