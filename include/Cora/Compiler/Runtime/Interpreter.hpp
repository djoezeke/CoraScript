#ifndef CORA_COMPILER_RUNTIME_INTERPRETER_H
#define CORA_COMPILER_RUNTIME_INTERPRETER_H

#include "Cora/Compiler/Parser/Parser.hpp"
#include "Cora/Compiler/Parser/Token.hpp"

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"
#include "Cora/Compiler/Runtime/Variable.hpp"

#include <deque>
#include <optional>
#include <string>
#include <unordered_map>

namespace cora::compiler
{
    namespace runtime
    {
        class Interpreter
        {
        public:
            using Token = parser::Token;
            using Statement = ast::Statement;
            using Expression = ast::Expression;
            using BlockStmt = ast::BlockStmt;
            using TokenType = parser::TokenType;

            void Run(const std::string &source);
            void RunFile(const std::string &path);

        private:
            void Execute(const std::deque<Statement *> &program);

            runtime::Value EvalExpr(Expression *expr);
            void ExecStmt(Statement *stmt);
            void ExecBlock(BlockStmt *block);

            bool IsTruthy(const runtime::Value &value) const;
            double AsNumber(const runtime::Value &value) const;
            std::string ToString(const runtime::Value &value) const;
            bool ValuesEqual(const runtime::Value &lhs, const runtime::Value &rhs) const;
            runtime::Value ApplyBinary(TokenType op, const runtime::Value &lhs, const runtime::Value &rhs) const;
            runtime::Value ApplyUnary(TokenType op, const runtime::Value &rhs) const;
            void CheckTypeCompatibility(const std::optional<std::string> &declaredType, const runtime::Value &value, const std::string &name) const;

            runtime::Scope m_GlobalScope;
            runtime::Scope *m_CurrentScope{&m_GlobalScope};
        };

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_INTERPRETER_H
