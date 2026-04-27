#ifndef CORA_IR_IRBUILDER_H
#define CORA_IR_IRBUILDER_H

#include "IRInstruction.hpp"

#include "../AST/ASTExpr.hpp"
#include "../AST/ASTStmt.hpp"

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
        IRBuilder(const std::vector<cora::ast::Statement *> &program);

        std::vector<BasicBlock *> Build();
        void Build(const std::vector<cora::ast::Statement *> &program);

    private:
        Instruction *EmitStatement(ast::Statement *stmt);
        Instruction *EmitExprStmt(ast::Statement *stmt);
        Instruction *EmitBlockStmt(ast::Statement *stmt);
        Instruction *EmitIfStmt(ast::Statement *stmt);
        Instruction *EmitForStmt(ast::Statement *stmt);
        Instruction *EmitWhileStmt(ast::Statement *stmt);
        Instruction *EmitSwitchStmt(ast::Statement *stmt);
        Instruction *EmitMatchStmt(ast::Statement *stmt);
        Value *EmitFuncDeclStmt(ast::Statement *stmt);
        Value *EmitVarDeclStmt(ast::Statement *stmt);
        Instruction *EmitPassStmt(ast::Statement *stmt);
        Instruction *EmitBreakStmt(ast::Statement *stmt);
        Instruction *EmitContinueStmt(ast::Statement *stmt);
        Instruction *EmitThrowStmt(ast::Statement *stmt);

        Instruction *EmitExpression(ast::Expression *expr);
        Instruction *EmitGroupExpr(ast::Expression *expr);
        Instruction *EmitArrayExpr(ast::Expression *expr);
        Instruction *EmitArrayIdExpr(ast::Expression *expr);
        Value EmitParamExpr(ast::Expression *expr);
        Instruction *EmitUnaryExpr(ast::Expression *expr);
        Instruction *EmitPrefixUnaryExpr(ast::Expression *expr);
        Instruction *EmitPostfixUnaryExpr(ast::Expression *expr);
        Instruction *EmitBinaryExpr(ast::Expression *expr);
        Instruction *EmitAssignExpr(ast::Expression *expr);
        Instruction *EmitFuncCallExpr(ast::Expression *expr);

    private:
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
