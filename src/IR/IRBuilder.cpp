#include "IRBuilder.hpp"

#include "../Parser/Token.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace cora::ir
{

    IRBuilder::IRBuilder() = default;

    BasicBlock *IRBuilder::Build(const std::deque<cora::ast::Statement *> &program)
    {
        Reset();

        auto entry = std::make_unique<BasicBlock>("entry");
        // entry->name = "entry";
        m_entry = entry.get();
        m_currentBlock = m_entry;
        m_blocks.push_back(m_entry);
        m_ownedBlocks.push_back(std::move(entry));

        for (ast::Statement *stmt : program)
        {
            EmitStatement(stmt);
        }

        return m_entry;
    }

    Instruction *IRBuilder::EmitStatement(ast::Statement *stmt)
    {
        if (stmt == nullptr)
        {
            return nullptr;
        }

        if (auto *exprStmt = dynamic_cast<ast::ExprStmt *>(stmt))
        {
            EmitExpression(exprStmt->expr);
            return nullptr;
        }
        if (auto *blockStmt = dynamic_cast<ast::BlockStmt *>(stmt))
        {
            EmitBlock(blockStmt);
            return nullptr;
        }
        if (auto *ifStmt = dynamic_cast<ast::IfStmt *>(stmt))
        {
            EmitIf(ifStmt);
            return nullptr;
        }
        if (auto *whileStmt = dynamic_cast<ast::WhileStmt *>(stmt))
        {
            EmitWhile(whileStmt);
            return nullptr;
        }
        if (auto *forStmt = dynamic_cast<ast::ForStmt *>(stmt))
        {
            EmitFor(forStmt);
            return nullptr;
        }
        if (auto *varDecl = dynamic_cast<ast::VarDeclStmt *>(stmt))
        {
            EmitVarDecl(varDecl);
            return nullptr;
        }
        if (auto *funcDecl = dynamic_cast<ast::FuncDeclStmt *>(stmt))
        {
            EmitFuncDecl(funcDecl);
            return nullptr;
        }

        return nullptr;
    }

    void IRBuilder::EmitBlock(ast::BlockStmt *block)
    {
        if (block == nullptr)
        {
            return;
        }

        for (ast::Statement *stmt : block->stmts)
        {
            EmitStatement(stmt);
        }
    }

    void IRBuilder::EmitIf(ast::IfStmt *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }

        Value *condition = EmitExpression(stmt->cond);
        if (condition == nullptr)
        {
            condition = MakeConstant(runtime::value(true));
        }

        BasicBlock *thenBlock = CreateBlock("if.then");
        BasicBlock *elseBlock = stmt->false_block != nullptr ? CreateBlock("if.else") : nullptr;
        BasicBlock *mergeBlock = CreateBlock("if.merge");

        if (elseBlock)
        {
            MakeValue<BranchInstruction>(condition, thenBlock, elseBlock, m_currentBlock);
        }
        else
        {
            MakeValue<BranchInstruction>(condition, thenBlock, mergeBlock, m_currentBlock);
        }

        m_currentBlock = thenBlock;
        EmitBlock(stmt->true_block);
        MakeValue<JumpInstruction>(mergeBlock, m_currentBlock);

        if (elseBlock)
        {
            m_currentBlock = elseBlock;
            EmitBlock(stmt->false_block);
            MakeValue<JumpInstruction>(mergeBlock, m_currentBlock);
        }

        m_currentBlock = mergeBlock;
    }

    void IRBuilder::EmitWhile(ast::WhileStmt *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }

        BasicBlock *condBlock = CreateBlock("while.cond");
        BasicBlock *bodyBlock = CreateBlock("while.body");
        BasicBlock *exitBlock = CreateBlock("while.exit");

        MakeValue<JumpInstruction>(condBlock, m_currentBlock);

        m_currentBlock = condBlock;
        Value *condition = EmitExpression(stmt->condition);
        if (condition == nullptr)
        {
            condition = MakeConstant(runtime::value(true));
        }
        MakeValue<BranchInstruction>(condition, bodyBlock, exitBlock, m_currentBlock);

        m_currentBlock = bodyBlock;
        EmitBlock(stmt->block);
        MakeValue<JumpInstruction>(condBlock, m_currentBlock);

        m_currentBlock = exitBlock;
    }

    void IRBuilder::EmitFor(ast::ForStmt *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }

        EmitStatement(stmt->init);

        BasicBlock *condBlock = CreateBlock("for.cond");
        BasicBlock *bodyBlock = CreateBlock("for.body");
        BasicBlock *updateBlock = CreateBlock("for.update");
        BasicBlock *exitBlock = CreateBlock("for.exit");

        MakeValue<JumpInstruction>(condBlock, m_currentBlock);

        m_currentBlock = condBlock;
        Value *condition = EmitExpression(stmt->condition);
        if (condition == nullptr)
        {
            condition = MakeConstant(runtime::value(true));
        }
        MakeValue<BranchInstruction>(condition, bodyBlock, exitBlock, m_currentBlock);

        m_currentBlock = bodyBlock;
        EmitBlock(stmt->block);
        MakeValue<JumpInstruction>(updateBlock, m_currentBlock);

        m_currentBlock = updateBlock;
        EmitStatement(stmt->update);
        MakeValue<JumpInstruction>(condBlock, m_currentBlock);

        m_currentBlock = exitBlock;
    }

    void IRBuilder::EmitVarDecl(ast::VarDeclStmt *stmt)
    {
        if (stmt == nullptr || stmt->name == nullptr)
        {
            return;
        }

        const std::string &name = stmt->name->name;
        Value *slot = lookupVariable(name);
        if (slot == nullptr)
        {
            slot = MakeValue<AllocaInstruction>(Type::Pointer(Type::Int()), name, m_currentBlock);
            assignVariable(name, slot);
        }

        if (stmt->value != nullptr)
        {
            if (auto *exprStmt = dynamic_cast<ast::ExprStmt *>(stmt->value))
            {
                Value *rhs = EmitExpression(exprStmt->expr);
                if (rhs != nullptr)
                {
                    MakeValue<StoreInstruction>(rhs, slot, m_currentBlock);
                }
            }
        }
    }

    void IRBuilder::EmitFuncDecl(ast::FuncDeclStmt *stmt)
    {
        if (stmt == nullptr || stmt->name == nullptr)
        {
            return;
        }

        const std::string &name = stmt->name->name;
        BasicBlock *savedBlock = m_currentBlock;
        auto savedVars = m_variables;

        BasicBlock *funcEntry = CreateBlock("fn." + name);
        m_currentBlock = funcEntry;
        m_variables.clear();

        for (ast::ParamExpr *param : stmt->params)
        {
            if (param == nullptr || param->name == nullptr)
            {
                continue;
            }
            Value *slot = MakeValue<AllocaInstruction>(Type::Pointer(Type::Int()), param->name->name, m_currentBlock);
            assignVariable(param->name->name, slot);
        }

        EmitBlock(stmt->block);
        MakeValue<ReturnInstruction>(m_currentBlock, nullptr);

        auto *funcValue = MakeValue<FunctionValue>(name, funcEntry, static_cast<int>(stmt->params.size()));
        savedVars[name] = funcValue;

        m_variables = std::move(savedVars);
        m_currentBlock = savedBlock;
    }

    Value *IRBuilder::EmitExpression(ast::Expression *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        if (auto *literal = EmitLiteral(expr))
        {
            return literal;
        }

        if (auto *identifier = dynamic_cast<ast::IdentifierExpr *>(expr))
        {
            return EmitIdentifier(identifier, true);
        }

        if (auto *assign = dynamic_cast<ast::AssignExpr *>(expr))
        {
            return EmitAssignExpr(assign);
        }

        if (auto *binary = dynamic_cast<ast::BinaryExpr *>(expr))
        {
            return EmitBinaryExpr(binary);
        }

        if (auto *unary = dynamic_cast<ast::UnaryExpr *>(expr))
        {
            return EmitUnaryExpr(unary);
        }

        if (auto *call = dynamic_cast<ast::FuncCallExpr *>(expr))
        {
            return EmitFuncCallExpr(call);
        }

        return nullptr;
    }

    Value *IRBuilder::EmitLiteral(ast::Expression *expr)
    {
        if (auto *literal = dynamic_cast<ast::IntegerExpr *>(expr))
        {
            return MakeConstant(runtime::value(literal->value));
        }
        if (auto *literal = dynamic_cast<ast::FloatExpr *>(expr))
        {
            return MakeConstant(runtime::value(literal->value));
        }
        if (auto *literal = dynamic_cast<ast::StringExpr *>(expr))
        {
            return MakeConstant(runtime::value(literal->value));
        }
        if (auto *literal = dynamic_cast<ast::BoolExpr *>(expr))
        {
            return MakeConstant(runtime::value(literal->value));
        }
        if (dynamic_cast<ast::NullExpr *>(expr))
        {
            return MakeConstant(runtime::value(nullptr));
        }

        return nullptr;
    }

    Value *IRBuilder::EmitIdentifier(ast::IdentifierExpr *expr, bool load)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        Value *value = lookupVariable(expr->name);
        if (value == nullptr)
        {
            return nullptr;
        }

        if (!load)
        {
            return value;
        }

        if (dynamic_cast<FunctionValue *>(value) != nullptr)
        {
            return value;
        }

        return MakeValue<LoadInstruction>(value, makeTempName("load"), m_currentBlock);
    }

    Value *IRBuilder::EmitAssignExpr(ast::AssignExpr *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        auto *identifier = dynamic_cast<ast::IdentifierExpr *>(expr->left);
        if (identifier == nullptr)
        {
            return nullptr;
        }

        Value *slot = EmitIdentifier(identifier, false);
        if (slot == nullptr)
        {
            slot = MakeValue<AllocaInstruction>(Type::Pointer(Type::Int()), identifier->name, m_currentBlock);
            assignVariable(identifier->name, slot);
        }

        Value *rhs = EmitExpression(expr->right);
        if (rhs == nullptr)
        {
            return nullptr;
        }

        MakeValue<StoreInstruction>(rhs, slot, m_currentBlock);
        return rhs;
    }

    Value *IRBuilder::EmitBinaryExpr(ast::BinaryExpr *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        Value *lhs = EmitExpression(expr->left);
        Value *rhs = EmitExpression(expr->right);
        if (lhs == nullptr || rhs == nullptr)
        {
            return nullptr;
        }

        Instruction::Opcode opcode = Instruction::Opcode::Add;
        switch (expr->op)
        {
        case parser::TokenType::Plus:
            opcode = Instruction::Opcode::Add;
            break;
        case parser::TokenType::Minus:
            opcode = Instruction::Opcode::Sub;
            break;
        case parser::TokenType::Star:
            opcode = Instruction::Opcode::Mul;
            break;
        case parser::TokenType::Slash:
            opcode = Instruction::Opcode::Div;
            break;
        case parser::TokenType::Equal:
            opcode = Instruction::Opcode::Eq;
            break;
        case parser::TokenType::NotEqual:
            opcode = Instruction::Opcode::Ne;
            break;
        case parser::TokenType::Less:
            opcode = Instruction::Opcode::Lt;
            break;
        case parser::TokenType::LessEqual:
            opcode = Instruction::Opcode::Le;
            break;
        case parser::TokenType::Greater:
            opcode = Instruction::Opcode::Gt;
            break;
        case parser::TokenType::GreaterEqual:
            opcode = Instruction::Opcode::Ge;
            break;
        default:
            opcode = Instruction::Opcode::Add;
            break;
        }

        return MakeValue<BinaryInstruction>(opcode, lhs, rhs, makeTempName(), m_currentBlock);
    }

    Value *IRBuilder::EmitUnaryExpr(ast::UnaryExpr *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        Value *value = EmitExpression(expr->expr);
        if (value == nullptr)
        {
            return nullptr;
        }

        Instruction::Opcode opcode = Instruction::Opcode::Sub;
        if (expr->op == parser::TokenType::Minus)
        {
            opcode = Instruction::Opcode::Sub;
        }
        return MakeValue<UnaryInstruction>(opcode, value, makeTempName(), m_currentBlock);
    }

    Value *IRBuilder::EmitFuncCallExpr(ast::FuncCallExpr *expr)
    {
        if (expr == nullptr || expr->name == nullptr)
        {
            return nullptr;
        }

        auto *callee = EmitIdentifier(expr->name, true);
        if (callee == nullptr)
        {
            return nullptr;
        }

        std::vector<Value *> args;
        args.reserve(expr->args.size());
        for (ast::Statement *argStmt : expr->args)
        {
            auto *exprStmt = dynamic_cast<ast::ExprStmt *>(argStmt);
            if (exprStmt == nullptr)
            {
                continue;
            }
            Value *argValue = EmitExpression(exprStmt->expr);
            if (argValue != nullptr)
            {
                args.push_back(argValue);
            }
        }

        return MakeValue<CallInstruction>(callee, args, makeTempName("call"), m_currentBlock);
    }

    BasicBlock *IRBuilder::CreateBlock(const std::string &name)
    {
        auto block = std::make_unique<BasicBlock>(name);
        BasicBlock *raw = block.get();
        m_blocks.push_back(raw);
        m_ownedBlocks.push_back(std::move(block));
        return raw;
    }

    Value *IRBuilder::MakeConstant(runtime::value value, const std::string &name)
    {
        return MakeValue<ConstantValue>(std::move(value), name);
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

    Value *IRBuilder::lookupVariable(const std::string &name) const
    {
        const auto found = m_variables.find(name);
        if (found == m_variables.end())
        {
            return nullptr;
        }
        return found->second;
    };

    void IRBuilder::assignVariable(const std::string &name, Value *value)
    {
        m_variables[name] = value;
    };

    std::string IRBuilder::makeTempName(const std::string &prefix)
    {
        std::ostringstream out;
        out << '%' << prefix << m_tempIndex++;
        return out.str();
    };

    IRBuilder::~IRBuilder() {};

} // namespace cora::ir
