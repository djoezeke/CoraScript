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
        void EmitVarDecl(ast::VarDeclStmt *stmt);
        void EmitFuncDecl(ast::FuncDeclStmt *stmt);

        BasicBlock *CreateBlock(const std::string &name);
        Value *MakeConstant(runtime::value value, const std::string &name = "");
        template <typename T, typename... Args>
        T *MakeValue(Args &&...args)
        {
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
        BasicBlock *m_entry{nullptr};
        BasicBlock *m_currentBlock{nullptr};
        std::size_t m_tempIndex{0};
    };

} // namespace cora::ir

#endif // CORA_IR_IRBUILDER_H
