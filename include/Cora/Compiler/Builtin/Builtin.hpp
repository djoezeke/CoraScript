#ifndef CORA_COMPILER_BUILTIN_BUILTIN_H
#define CORA_COMPILER_BUILTIN_BUILTIN_H

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

namespace cora::compiler
{
    namespace runtime
    {
        class Object;
        class Method;
        class Callable;
        class Function;
    }

    namespace builtin
    {

#define CORA_NEW_OBJECT(name) ::cora::compiler::builtin::MakeObject(name)
#define CORA_NEW_VARIABLE(name, value) ::cora::compiler::runtime::Value name = ::cora::compiler::runtime::Value(value)

#define CORA_NEW_METHOD(name)                                                                                                        \
    ::cora::compiler::runtime::Value fun_##name(const std::vector<::cora::compiler::runtime::Value> &arguments);                     \
    std::shared_ptr<::cora::compiler::runtime::Method> name = ::cora::compiler::builtin::MakeMethod(classObject, #name, fun_##name); \
    runtime::Value fun_##name(const std::vector<runtime::Value> &arguments)

#define CORA_NEW_FUNCTION(name)                                                                                             \
    ::cora::compiler::runtime::Value fun_##name(const std::vector<::cora::compiler::runtime::Value> &arguments);            \
    std::shared_ptr<::cora::compiler::runtime::Function> name = ::cora::compiler::builtin::MakeFunction(#name, fun_##name); \
    ::cora::compiler::runtime::Value fun_##name(const std::vector<::cora::compiler::runtime::Value> &arguments)

        std::shared_ptr<runtime::Object> MakeObject(const std::string &className);
        std::shared_ptr<runtime::Method> MakeMethod(const std::shared_ptr<runtime::Object> &object, std::string name, runtime::Method::Func method);
        std::shared_ptr<runtime::Function> MakeFunction(std::string name, runtime::Function::Func function);

        void RegisterClass(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant = true);
        void RegisterModule(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant = true);
        void RegisterFunction(runtime::Scope &scope, const std::string &name, std::function<runtime::Value(const std::vector<runtime::Value> &)> function, bool constant = true);
        void RegisterVariable(runtime::Scope &scope, const std::string &name, runtime::Value value, bool constant = true);

#define CORA_REGISTER_CLASS(scope, name, object) ::cora::compiler::builtin::RegisterClass((scope), (name), (object))
#define CORA_REGISTER_MODULE(scope, name, object) ::cora::compiler::builtin::RegisterModule((scope), (name), (object))
#define CORA_REGISTER_VARIABLE(scope, name, value) ::cora::compiler::builtin::RegisterVariable((scope), (name), (value))
#define CORA_REGISTER_FUNCTION(scope, name, function) ::cora::compiler::builtin::RegisterFunction((scope), (name), (function))

        void Builtins(runtime::Scope &scope);

    } // namespace builtin

} // namespace cora::compiler

#endif // CORA_COMPILER_BUILTIN_BUILTIN_H