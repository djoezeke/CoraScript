#include "IRBuilder.hpp"

#include "../Parser/Token.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace cora::ir
{
    namespace
    {
        using cora::compiler::ast::Assignment;
        using cora::compiler::ast::BinaryExpr;
        using cora::compiler::ast::BlockStmt;
        using cora::compiler::ast::Bool;
        using cora::compiler::ast::Expression;
        using cora::compiler::ast::ExprStmt;
        using cora::compiler::ast::Float;
        using cora::compiler::ast::Identifier;
        using cora::compiler::ast::IfStmt;
        using cora::compiler::ast::Integer;
        using cora::compiler::ast::LiteralExpr;
        using cora::compiler::ast::PassStmt;
        using cora::compiler::ast::PrintStmt;
        using cora::compiler::ast::ReturnStmt;
        using cora::compiler::ast::Statement;
        using cora::compiler::ast::String;
        using cora::compiler::ast::VarDeclStmt;
        using cora::compiler::ast::WhileStmt;

        static Instruction::OpKind toOp(cora::compiler::parser::TokenType tokenType)
        {
            switch (tokenType)
            {
            case cora::compiler::parser::TokenType::Plus:
            case cora::compiler::parser::TokenType::T_PLUS:
                return Instruction::ADD;
            case cora::compiler::parser::TokenType::Star:
            case cora::compiler::parser::TokenType::T_STAR:
                return Instruction::MUL;
            case cora::compiler::parser::TokenType::Minus:
            case cora::compiler::parser::TokenType::T_MINUS:
                return Instruction::SUB;
            case cora::compiler::parser::TokenType::Slash:
            case cora::compiler::parser::TokenType::T_SLASH:
                return Instruction::DIV;
            default:
                throw std::runtime_error("IRBuilder: unsupported binary operator");
            }
        }

        static int literalToInt(const LiteralExpr *literal)
        {
            const auto &value = literal->GetValue();
            if (std::holds_alternative<double>(value))
            {
                return static_cast<int>(std::get<double>(value));
            }
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value) ? 1 : 0;
            }
            if (std::holds_alternative<std::string>(value))
            {
                return static_cast<int>(std::get<std::string>(value).size());
            }
            return 0;
        }
    }

    IRBuilder::IRBuilder() = default;

    BasicBlock *IRBuilder::Build(const std::deque<cora::compiler::ast::Statement *> &program)
    {
        Reset();

        auto entry = std::make_unique<BasicBlock>();
        entry->label = "entry";
        m_entry = entry.get();
        m_currentBlock = m_entry;
        m_blocks.push_back(m_entry);
        m_ownedBlocks.push_back(std::move(entry));

        for (Statement *stmt : program)
        {
            emitStatement(stmt);
        }

        return m_entry;
    }

    void IRBuilder::Reset()
    {
        m_ownedBlocks.clear();
        m_blocks.clear();
        m_ownedValues.clear();
        m_variables.clear();
        m_entry = nullptr;
        m_currentBlock = nullptr;
        m_tempIndex = 0;
    }

    const std::vector<BasicBlock *> &IRBuilder::GetBlocks() const
    {
        return m_blocks;
    }

    BasicBlock *IRBuilder::GetEntryBlock() const
    {
        return m_entry;
    }

    IRBuilder::~IRBuilder() = default;

    Value *IRBuilder::emitExpression(Expression *expr)
    {
        if (expr == nullptr)
        {
            return makeConstant(0);
        }

        if (auto *integer = dynamic_cast<Integer *>(expr))
        {
            return makeConstant(integer->GetValue());
        }

        if (auto *literal = dynamic_cast<LiteralExpr *>(expr))
        {
            return makeConstant(literalToInt(literal));
        }

        if (auto *boolean = dynamic_cast<Bool *>(expr))
        {
            return makeConstant(boolean->GetValue() ? 1 : 0);
        }

        if (auto *floating = dynamic_cast<Float *>(expr))
        {
            return makeConstant(static_cast<int>(floating->GetValue()));
        }

        if (auto *identifier = dynamic_cast<Identifier *>(expr))
        {
            Value *existing = lookupVariable(identifier->GetName());
            if (existing == nullptr)
            {
                throw std::runtime_error("IRBuilder: undefined variable '" + identifier->GetName() + "'");
            }
            return existing;
        }

        if (auto *binary = dynamic_cast<BinaryExpr *>(expr))
        {
            Value *lhs = emitExpression(binary->GetLeft());
            Value *rhs = emitExpression(binary->GetRight());
            return emitBinary(toOp(binary->GetOperator()), lhs, rhs, makeTempName("tmp"));
        }

        if (auto *stringExpr = dynamic_cast<String *>(expr))
        {
            return makeConstant(static_cast<int>(stringExpr->GetValue().size()));
        }

        throw std::runtime_error("IRBuilder: unsupported expression type");
    }

    void IRBuilder::emitStatement(Statement *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }

        if (auto *block = dynamic_cast<BlockStmt *>(stmt))
        {
            for (Statement *nested : block->statements)
            {
                emitStatement(nested);
            }
            return;
        }

        if (auto *decl = dynamic_cast<VarDeclStmt *>(stmt))
        {
            Value *value = emitExpression(decl->GetExpression());
            auto *store = emitInstruction(Instruction::STORE, decl->GetName(), true);
            store->addOperand(value);
            assignVariable(decl->GetName(), value);
            return;
        }

        if (auto *assign = dynamic_cast<Assignment *>(stmt))
        {
            Value *value = emitExpression(assign->GetValue());
            auto *target = dynamic_cast<Identifier *>(assign->GetTarget());
            if (target == nullptr)
            {
                throw std::runtime_error("IRBuilder: only identifier assignments are supported");
            }

            auto *store = emitInstruction(Instruction::STORE, target->GetName(), true);
            store->addOperand(value);
            assignVariable(target->GetName(), value);
            return;
        }

        if (auto *exprStmt = dynamic_cast<ExprStmt *>(stmt))
        {
            (void)emitExpression(exprStmt->GetExpression());
            return;
        }

        if (auto *printStmt = dynamic_cast<PrintStmt *>(stmt))
        {
            for (Expression *expression : printStmt->expressions)
            {
                Value *value = emitExpression(expression);
                auto *printInst = emitInstruction(Instruction::PRINT, makeTempName("print"), true);
                printInst->addOperand(value);
            }
            return;
        }

        if (auto *ret = dynamic_cast<ReturnStmt *>(stmt))
        {
            Value *value = emitExpression(ret->GetValue());
            auto *retInst = emitInstruction(Instruction::RETURN, makeTempName("ret"), true);
            retInst->addOperand(value);
            return;
        }

        if (auto *ifStmt = dynamic_cast<IfStmt *>(stmt))
        {
            for (const auto &branch : ifStmt->branches)
            {
                (void)emitExpression(branch.first);
                if (branch.second != nullptr)
                {
                    for (Statement *branchStmt : branch.second->statements)
                    {
                        emitStatement(branchStmt);
                    }
                }
            }

            if (ifStmt->elseBlock != nullptr)
            {
                for (Statement *branchStmt : ifStmt->elseBlock->statements)
                {
                    emitStatement(branchStmt);
                }
            }
            return;
        }

        if (auto *whileStmt = dynamic_cast<WhileStmt *>(stmt))
        {
            (void)emitExpression(whileStmt->condition);
            if (whileStmt->block != nullptr)
            {
                for (Statement *loopStmt : whileStmt->block->statements)
                {
                    emitStatement(loopStmt);
                }
            }
            return;
        }

        if (dynamic_cast<PassStmt *>(stmt) != nullptr)
        {
            return;
        }

        throw std::runtime_error("IRBuilder: unsupported statement type");
    }

    Instruction *IRBuilder::emitBinary(Instruction::OpKind kind, Value *lhs, Value *rhs, const std::string &name, bool sideEffect)
    {
        auto *inst = emitInstruction(kind, name, sideEffect);
        inst->addOperand(lhs);
        inst->addOperand(rhs);
        return inst;
    }

    Instruction *IRBuilder::emitInstruction(Instruction::OpKind kind, const std::string &name, bool sideEffect)
    {
        auto instruction = std::make_unique<Instruction>(kind, name, sideEffect);
        Instruction *raw = instruction.get();
        m_ownedValues.push_back(std::move(instruction));
        m_currentBlock->addInstruction(raw);
        return raw;
    }

    ConstantInt *IRBuilder::makeConstant(int value)
    {
        auto constant = std::make_unique<ConstantInt>(value);
        ConstantInt *raw = constant.get();
        m_ownedValues.push_back(std::move(constant));
        return raw;
    }

    Value *IRBuilder::lookupVariable(const std::string &name) const
    {
        const auto found = m_variables.find(name);
        if (found == m_variables.end())
        {
            return nullptr;
        }
        return found->second;
    }

    void IRBuilder::assignVariable(const std::string &name, Value *value)
    {
        m_variables[name] = value;
    }

    std::string IRBuilder::makeTempName(const std::string &prefix)
    {
        std::ostringstream out;
        out << '%' << prefix << m_tempIndex++;
        return out.str();
    }

} // namespace cora::ir
