#ifndef CORA_COMPILER_RUNTIME_INTERPRETER_H
#define CORA_COMPILER_RUNTIME_INTERPRETER_H

#include "Cora/Compiler/Error/Error.hpp"
#include "Cora/Compiler/Parser/Parser.hpp"
#include "Cora/Compiler/Parser/Token.hpp"
#include "Cora/Compiler/Runtime/Scope.hpp"

#include <deque>
#include <filesystem>
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
        class Callable;

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
            void SetFileName(std::string fileName);

        private:
            void RegisterBuiltins();
            std::shared_ptr<Object> ImportModule(const std::string &modulePath);
            std::shared_ptr<Object> LoadModuleFromFile(const std::string &modulePath, const std::filesystem::path &path);
            Scope *PushTransientScope(ScopeKind kind, Scope *parent = nullptr);
            Scope *PushTransientScope(std::unique_ptr<Scope> scope);
            void PopTransientScope();

            error::DiagnosticContext MakeContext(unsigned int line = 0, unsigned int column = 0) const;
            [[noreturn]] void RaiseRuntimeError(const std::string &message, unsigned int line = 0, unsigned int column = 0) const;

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
            void CheckTypeCompatibility(const std::string &type, const std::string &name, const runtime::Value &value) const;
            std::shared_ptr<Callable> FindBestMethodOverload(const std::shared_ptr<Object> &object, const std::string &methodName, std::size_t argCount) const;
            bool CanAccessMember(const std::shared_ptr<Object> &object) const;
            Value ResolveMemberValue(const std::shared_ptr<Object> &object, const std::string &memberName);

        private:
            runtime::Scope m_GlobalScope;
            runtime::Scope *m_CurrentScope{&m_GlobalScope};
            std::filesystem::path m_WorkingDirectory;
            std::unordered_map<std::string, std::shared_ptr<Object>> m_ModuleCache;
            std::vector<std::shared_ptr<Scope>> m_PersistentScopes;
            std::vector<std::unique_ptr<Scope>> m_TransientScopes;
            std::string m_FileName{"<memory>"};
            std::string m_ModuleName;
            std::vector<std::string> m_NamespaceStack;
            std::vector<std::string> m_ClassStack;
            std::vector<std::string> m_FunctionStack;
        };

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_INTERPRETER_H
