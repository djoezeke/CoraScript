#ifndef CORA_IR_IRBUILDER_H
#define CORA_IR_IRBUILDER_H

#include "IRInstruction.hpp"

#include "../AST/ASTExpr.hpp"
#include "../AST/ASTStmt.hpp"

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cora::ir
{
    class IRBuilder final
    {
    public:
        IRBuilder();
        ~IRBuilder();

        BasicBlock *Build(const std::deque<cora::ast::Statement *> &program);

        const std::vector<BasicBlock *> &GetBlocks() const;
        BasicBlock *GetEntryBlock() const;

    private:
        Instruction *EmitStatement(ast::Statement *stmt);
        Value *EmitExpression(ast::Expression *expr);
        Value *EmitBinaryExpr(ast::BinaryExpr *expr);
        Value *EmitUnaryExpr(ast::UnaryExpr *expr);
        Value *EmitAssignExpr(ast::AssignExpr *expr);
        Value *EmitFuncCallExpr(ast::FuncCallExpr *expr);
        Value *EmitLiteral(ast::Expression *expr);
        Value *EmitIdentifier(ast::IdentifierExpr *expr, bool load);
        void EmitBlock(ast::BlockStmt *block);
        void EmitIf(ast::IfStmt *stmt);
        void EmitWhile(ast::WhileStmt *stmt);
        void EmitFor(ast::ForStmt *stmt);
        void EmitForIn(ast::ForInStmt *stmt);
        void EmitVarDecl(ast::VarDeclStmt *stmt);
        void EmitFuncDecl(ast::FuncDeclStmt *stmt);
        void EmitSwitch(ast::SwitchStmt *stmt);
        void EmitImport(ast::ImportStmt *stmt);
        void EmitClass(ast::ClassDecl *stmt);
        void EmitStruct(ast::StructDecl *stmt);
        void EmitEnum(ast::EnumDecl *stmt);
        void EmitTryCatch(ast::TryCatchStmt *stmt);
        Value *EmitTernaryExpr(ast::TernaryExpr *expr);

        BasicBlock *CreateBlock(const std::string &name);
        Value *MakeConstant(runtime::value value, const std::string &name = "");
        template <typename T, typename... Args>
        T *MakeValue(Args &&...args)
        {
            if (m_currentBlock && !m_currentBlock->insts.empty())
            {
                Instruction::Opcode lastOp = m_currentBlock->insts.back()->opcode;
                if (lastOp == Instruction::Opcode::Br || lastOp == Instruction::Opcode::Ret || lastOp == Instruction::Opcode::Jump)
                {
                    // Block already terminated.
                    // To keep things simple, we create a dead block to soak up unreachable instructions.
                    m_currentBlock = CreateBlock("dead");
                }
            }
            auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
            T *raw = ptr.get();
            m_ownedValues.push_back(std::move(ptr));
            return raw;
        }

        void Reset();
        Value *lookupVariable(const std::string &name) const;
        void assignVariable(const std::string &name, Value *value);
        std::string makeTempName(const std::string &prefix = "t");

    private:
        std::vector<std::unique_ptr<BasicBlock>> m_ownedBlocks;
        std::vector<BasicBlock *> m_blocks;
        std::vector<std::unique_ptr<Value>> m_ownedValues;
        std::unordered_map<std::string, Value *> m_variables;
        std::vector<BasicBlock *> m_breakStack;
        std::vector<BasicBlock *> m_continueStack;
        BasicBlock *m_entry{nullptr};
        BasicBlock *m_currentBlock{nullptr};
        std::size_t m_tempIndex{0};
    };

} // namespace cora::ir

#endif // CORA_IR_IRBUILDER_H
