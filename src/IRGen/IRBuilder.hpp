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

        BasicBlock *Build(const std::deque<cora::compiler::ast::Statement *> &program);
        void Reset();

        const std::vector<BasicBlock *> &GetBlocks() const;
        BasicBlock *GetEntryBlock() const;

        ~IRBuilder();

    private:
        Value *emitExpression(cora::compiler::ast::Expression *expr);
        void emitStatement(cora::compiler::ast::Statement *stmt);
        Instruction *emitBinary(Instruction::OpKind kind, Value *lhs, Value *rhs, const std::string &name, bool sideEffect = false);
        Instruction *emitInstruction(Instruction::OpKind kind, const std::string &name, bool sideEffect = false);
        ConstantInt *makeConstant(int value);
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
