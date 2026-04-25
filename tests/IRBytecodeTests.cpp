#include "../src/AST/ASTExpr.hpp"
#include "../src/AST/ASTStmt.hpp"
#include "../src/IRGen/IRBuilder.hpp"
#include "../src/IRGen/IROptimizer.hpp"
#include "../src/IRGen/IRPrinter.hpp"
#include "../src/IRGen/IRVerifier.hpp"
#include "../src/VMachine/Bytecode.hpp"
#include "../src/VMachine/BytecodeEmitter.hpp"
#include "../src/VMachine/BytecodeReader.hpp"
#include "../src/VMachine/BytecodeWriter.hpp"
#include "../src/VMachine/VMachine.hpp"

#include <deque>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace
{
    using namespace cora::compiler::ast;

    std::deque<Statement *> BuildSampleProgram()
    {
        auto *expr = new BinaryExpr(new Integer(2), cora::compiler::parser::TokenType::Plus, new Integer(3));

        std::deque<Statement *> program;
        program.push_back(new VarDeclStmt("x", std::nullopt, expr));
        program.push_back(new ReturnStmt(new Identifier("x")));
        return program;
    }

    void FreeProgram(std::deque<Statement *> &program)
    {
        for (Statement *stmt : program)
        {
            delete stmt;
        }
        program.clear();
    }

    void Assert(bool condition, const std::string &message)
    {
        if (!condition)
        {
            throw std::runtime_error(message);
        }
    }
}

int main()
{
    auto program = BuildSampleProgram();

    try
    {
        cora::ir::IRBuilder builder;
        builder.Build(program);
        auto blocks = builder.GetBlocks();

        cora::ir::IRVerifier verifier;
        verifier.Verify(blocks);

        cora::ir::IROptimizer optimizer;
        optimizer.Optimize(blocks);
        verifier.Verify(blocks);

        cora::ir::IRPrinter printer;
        const std::string irText = printer.Print(blocks);
        Assert(!irText.empty(), "Expected non-empty IR print output");

        cora::vmachine::BytecodeEmitter emitter;
        const cora::vmachine::BytecodeProgram bytecode = emitter.Emit(blocks);
        Assert(!bytecode.code.empty(), "Expected emitted bytecode instructions");

        cora::vmachine::BytecodeWriter writer;
        const auto raw = writer.Write(bytecode);

        cora::vmachine::BytecodeReader reader;
        reader.Read(raw);

        const auto &roundtrip = reader.GetProgram();
        Assert(roundtrip.code.size() == bytecode.code.size(), "Roundtrip instruction count mismatch");

        cora::vmachine::VMachine vm;
        const int runResult = vm.Run(roundtrip);
        Assert(runResult == 0, "Expected VM run to succeed");
        Assert(vm.GetReturnValue().AsNumber() == 5.0, "Expected return value 5");

        std::cout << "IR/bytecode tests passed\n";
        std::cout << irText << "\n";

        FreeProgram(program);
        return 0;
    }
    catch (...)
    {
        FreeProgram(program);
        throw;
    }
}
