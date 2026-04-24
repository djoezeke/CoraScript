#ifndef CORA_CORE_INTERNAL_BYTECODECOMPILER_HPP
#define CORA_CORE_INTERNAL_BYTECODECOMPILER_HPP

#include "Bytecode.hpp"

#include "../AST/ASTExpr.hpp"
#include "../AST/ASTStmt.hpp"

#include <deque>

namespace cora::embed::internal
{
    class BytecodeCompiler
    {
    public:
        BytecodeProgram Compile(const std::deque<cora::compiler::ast::Statement *> &program);

    private:
        void CompileStatement(cora::compiler::ast::Statement *statement, BytecodeProgram &out);
        void CompileBlock(cora::compiler::ast::BlockStmt *block, BytecodeProgram &out);
        void CompileExpression(cora::compiler::ast::Expression *expression, BytecodeProgram &out);
        void CompileAssignment(cora::compiler::ast::Expression *target, cora::compiler::ast::Expression *value, BytecodeProgram &out);
        [[noreturn]] void Unsupported(const char *what) const;
    };
}

#endif
