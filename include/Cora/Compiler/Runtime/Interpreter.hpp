#ifndef CORA_COMPILER_RUNTIME_INTERPRETER_H
#define CORA_COMPILER_RUNTIME_INTERPRETER_H

#include "Cora/Compiler/Parser/Parser.hpp"
#include "Cora/Compiler/Parser/Token.hpp"
#include "Cora/Compiler/Runtime/Scope.hpp"

#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace cora::compiler
{
    namespace runtime
    {

        class Scope;
        class Object;
        class Value;
        class Variable;

        class Interpreter
        {
        public:
            Interpreter();

            using Token = parser::Token;
            using TokenType = parser::TokenType;

            using BlockStmt = ast::BlockStmt;
            using Statement = ast::Statement;
            using Expression = ast::Expression;

            using Value = runtime::Value;
            using Scope = runtime::Scope;

            void Run(const std::string &source);
            void RunFile(const std::string &path);

        private:
            void RegisterBuiltins();

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
            void InvokeConstructor(const std::shared_ptr<Object> &object, const std::vector<Value> &arguments);
            void InvokeDestructor(const Value &value);
            void CheckTypeCompatibility(const std::optional<std::string> &declaredType, const runtime::Value &value, const std::string &name) const;

        private:
            runtime::Scope m_GlobalScope;
            runtime::Scope *m_CurrentScope{&m_GlobalScope};
        };

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_INTERPRETER_H
