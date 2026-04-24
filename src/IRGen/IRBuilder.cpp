#include "IRBuilder.hpp"

#include "../Parser/Token.hpp"

#include <stdexcept>
#include <string>

namespace cora::embed::internal
{
    using namespace cora::compiler;

    BytecodeProgram BytecodeCompiler::Compile(const std::deque<cora::compiler::ast::Statement *> &program)
    {
        BytecodeProgram out;

        for (cora::compiler::ast::Statement *statement : program)
        {
            CompileStatement(statement, out);
        }

        out.Emit(OpCode::PushNull);
        out.Emit(OpCode::Halt);
        return out;
    }

    void BytecodeCompiler::CompileStatement(cora::compiler::ast::Statement *statement, BytecodeProgram &out)
    {
        if (!statement)
        {
            return;
        }

        if (auto *var = dynamic_cast<cora::compiler::ast::VarDeclStmt *>(statement))
        {
            CompileExpression(var->GetExpression(), out);
            out.Emit(OpCode::StoreGlobal, out.AddName(var->GetName()));
            out.Emit(OpCode::Pop);
            return;
        }

        if (auto *var = dynamic_cast<cora::compiler::ast::VarDeclaration *>(statement))
        {
            CompileExpression(var->GetExpression(), out);
            out.Emit(OpCode::StoreGlobal, out.AddName(var->GetName()));
            out.Emit(OpCode::Pop);
            return;
        }

        if (auto *assign = dynamic_cast<cora::compiler::ast::AssignStmt *>(statement))
        {
            CompileAssignment(assign->GetTarget(), assign->GetValue(), out);
            out.Emit(OpCode::Pop);
            return;
        }

        if (auto *assign = dynamic_cast<cora::compiler::ast::Assignment *>(statement))
        {
            CompileAssignment(assign->GetTarget(), assign->GetValue(), out);
            out.Emit(OpCode::Pop);
            return;
        }

        if (auto *expr = dynamic_cast<cora::compiler::ast::ExprStmt *>(statement))
        {
            CompileExpression(expr->GetExpression(), out);
            out.Emit(OpCode::Pop);
            return;
        }

        if (auto *print = dynamic_cast<cora::compiler::ast::PrintStmt *>(statement))
        {
            out.Emit(OpCode::LoadGlobal, out.AddName("print"));
            for (cora::compiler::ast::Expression *expr : print->expressions)
            {
                CompileExpression(expr, out);
            }
            out.Emit(OpCode::Call, static_cast<std::int32_t>(print->expressions.size()));
            out.Emit(OpCode::Pop);
            return;
        }

        if (auto *block = dynamic_cast<cora::compiler::ast::BlockStmt *>(statement))
        {
            CompileBlock(block, out);
            return;
        }

        if (auto *ifStmt = dynamic_cast<cora::compiler::ast::IfStmt *>(statement))
        {
            std::vector<std::int32_t> branchEndJumps;
            for (const auto &branch : ifStmt->branches)
            {
                CompileExpression(branch.first, out);
                const std::int32_t jumpIfFalse = out.Emit(OpCode::JumpIfFalse, -1);
                out.Emit(OpCode::Pop);
                CompileBlock(branch.second, out);
                branchEndJumps.push_back(out.Emit(OpCode::Jump, -1));
                out.code[static_cast<std::size_t>(jumpIfFalse)].a = static_cast<std::int32_t>(out.code.size());
                out.Emit(OpCode::Pop);
            }

            if (ifStmt->elseBlock)
            {
                CompileBlock(ifStmt->elseBlock, out);
            }

            const std::int32_t branchEnd = static_cast<std::int32_t>(out.code.size());
            for (std::int32_t jump : branchEndJumps)
            {
                out.code[static_cast<std::size_t>(jump)].a = branchEnd;
            }
            return;
        }

        if (auto *whileStmt = dynamic_cast<cora::compiler::ast::WhileStmt *>(statement))
        {
            const std::int32_t loopStart = static_cast<std::int32_t>(out.code.size());
            CompileExpression(whileStmt->condition, out);
            const std::int32_t jumpIfFalse = out.Emit(OpCode::JumpIfFalse, -1);
            out.Emit(OpCode::Pop);
            CompileBlock(whileStmt->block, out);
            out.Emit(OpCode::Jump, loopStart);
            out.code[static_cast<std::size_t>(jumpIfFalse)].a = static_cast<std::int32_t>(out.code.size());
            out.Emit(OpCode::Pop);
            return;
        }

        if (auto *returnStmt = dynamic_cast<cora::compiler::ast::ReturnStmt *>(statement))
        {
            if (returnStmt->GetValue())
            {
                CompileExpression(returnStmt->GetValue(), out);
            }
            else
            {
                out.Emit(OpCode::PushNull);
            }
            out.Emit(OpCode::Return);
            return;
        }

        if (dynamic_cast<cora::compiler::ast::PassStmt *>(statement))
        {
            return;
        }

        Unsupported("statement");
    }

    void BytecodeCompiler::CompileBlock(cora::compiler::ast::BlockStmt *block, BytecodeProgram &out)
    {
        if (!block)
        {
            return;
        }

        for (cora::compiler::ast::Statement *statement : block->statements)
        {
            CompileStatement(statement, out);
        }
    }

    void BytecodeCompiler::CompileExpression(cora::compiler::ast::Expression *expression, BytecodeProgram &out)
    {
        if (!expression)
        {
            out.Emit(OpCode::PushNull);
            return;
        }

        if (auto *literal = dynamic_cast<cora::compiler::ast::LiteralExpr *>(expression))
        {
            const auto &value = literal->GetValue();
            if (std::holds_alternative<std::monostate>(value))
            {
                out.Emit(OpCode::PushNull);
            }
            else if (std::holds_alternative<bool>(value))
            {
                out.Emit(OpCode::PushConst, out.AddConstant(runtime::Value(std::get<bool>(value))));
            }
            else if (std::holds_alternative<double>(value))
            {
                out.Emit(OpCode::PushConst, out.AddConstant(runtime::Value(std::get<double>(value))));
            }
            else if (std::holds_alternative<std::string>(value))
            {
                out.Emit(OpCode::PushConst, out.AddConstant(runtime::Value(std::get<std::string>(value))));
            }
            else
            {
                out.Emit(OpCode::PushNull);
            }
            return;
        }

        if (auto *variable = dynamic_cast<cora::compiler::ast::VariableExpr *>(expression))
        {
            out.Emit(OpCode::LoadGlobal, out.AddName(variable->GetName()));
            return;
        }

        if (auto *unary = dynamic_cast<cora::compiler::ast::UnaryExpr *>(expression))
        {
            CompileExpression(unary->GetRhs(), out);
            switch (unary->GetOperator())
            {
            case parser::TokenType::Minus:
                out.Emit(OpCode::Negate);
                break;
            case parser::TokenType::Not:
                out.Emit(OpCode::LogicalNot);
                break;
            case parser::TokenType::Plus:
                break;
            default:
                Unsupported("unary operator");
            }
            return;
        }

        if (auto *binary = dynamic_cast<cora::compiler::ast::BinaryExpr *>(expression))
        {
            CompileExpression(binary->GetLeft(), out);
            CompileExpression(binary->GetRight(), out);
            switch (binary->GetOperator())
            {
            case parser::TokenType::Plus:
                out.Emit(OpCode::Add);
                break;
            case parser::TokenType::Minus:
                out.Emit(OpCode::Sub);
                break;
            case parser::TokenType::Star:
                out.Emit(OpCode::Mul);
                break;
            case parser::TokenType::Slash:
                out.Emit(OpCode::Div);
                break;
            case parser::TokenType::Percent:
                out.Emit(OpCode::Mod);
                break;
            case parser::TokenType::Equal:
                out.Emit(OpCode::Equal);
                break;
            case parser::TokenType::NotEqual:
                out.Emit(OpCode::NotEqual);
                break;
            case parser::TokenType::Less:
                out.Emit(OpCode::Less);
                break;
            case parser::TokenType::LessEqual:
                out.Emit(OpCode::LessEqual);
                break;
            case parser::TokenType::Greater:
                out.Emit(OpCode::Greater);
                break;
            case parser::TokenType::GreaterEqual:
                out.Emit(OpCode::GreaterEqual);
                break;
            default:
                Unsupported("binary operator");
            }
            return;
        }

        if (auto *call = dynamic_cast<cora::compiler::ast::CallExpr *>(expression))
        {
            CompileExpression(call->GetCallee(), out);
            for (cora::compiler::ast::Expression *argument : call->GetArguments())
            {
                CompileExpression(argument, out);
            }
            out.Emit(OpCode::Call, static_cast<std::int32_t>(call->GetArguments().size()));
            return;
        }

        Unsupported("expression");
    }

    void BytecodeCompiler::CompileAssignment(cora::compiler::ast::Expression *target, cora::compiler::ast::Expression *value, BytecodeProgram &out)
    {
        auto *variable = dynamic_cast<cora::compiler::ast::VariableExpr *>(target);
        if (!variable)
        {
            Unsupported("assignment target");
        }

        CompileExpression(value, out);
        out.Emit(OpCode::StoreGlobal, out.AddName(variable->GetName()));
    }

    [[noreturn]] void BytecodeCompiler::Unsupported(const char *what) const
    {
        throw std::runtime_error(std::string("VM bytecode compiler does not support this ") + what);
    }
}
