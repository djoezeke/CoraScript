#ifndef CORA_COMPILER_BUILTIN_BUILTIN_H
#define CORA_COMPILER_BUILTIN_BUILTIN_H

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../Runtime/Scope.hpp"
#include "../Runtime/Value.hpp"

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
#define CORA_NEW_VARIABLE(name, _value) ::cora::compiler::runtime::value name = ::cora::compiler::runtime::value(_value)

#define CORA_NEW_METHOD(name)                                                                                                        \
    ::cora::compiler::runtime::value fun_##name(const std::vector<::cora::compiler::runtime::value> &arguments);                     \
    std::shared_ptr<::cora::compiler::runtime::Method> name = ::cora::compiler::builtin::MakeMethod(classObject, #name, fun_##name); \
    runtime::value fun_##name(const std::vector<runtime::value> &arguments)

#define CORA_NEW_FUNCTION(name)                                                                                             \
    ::cora::compiler::runtime::value fun_##name(const std::vector<::cora::compiler::runtime::value> &arguments);            \
    std::shared_ptr<::cora::compiler::runtime::Function> name = ::cora::compiler::builtin::MakeFunction(#name, fun_##name); \
    ::cora::compiler::runtime::value fun_##name(const std::vector<::cora::compiler::runtime::value> &arguments)

        std::shared_ptr<runtime::Object> MakeObject(const std::string &className);
        std::shared_ptr<runtime::Method> MakeMethod(const std::shared_ptr<runtime::Object> &object, std::string name, runtime::Method::Func method);
        std::shared_ptr<runtime::Function> MakeFunction(std::string name, runtime::Function::Func function);

        void RegisterClass(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant = true);
        void RegisterModule(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant = true);
        void RegisterFunction(runtime::Scope &scope, const std::string &name, std::function<runtime::value(const std::vector<runtime::value> &)> function, bool constant = true);
        void RegisterVariable(runtime::Scope &scope, const std::string &name, runtime::value value, bool constant = true);

#define CORA_REGISTER_CLASS(scope, name, object) ::cora::compiler::builtin::RegisterClass((scope), (name), (object))
#define CORA_REGISTER_MODULE(scope, name, object) ::cora::compiler::builtin::RegisterModule((scope), (name), (object))
#define CORA_REGISTER_VARIABLE(scope, name, value) ::cora::compiler::builtin::RegisterVariable((scope), (name), (value))
#define CORA_REGISTER_FUNCTION(scope, name, function) ::cora::compiler::builtin::RegisterFunction((scope), (name), (function))

        void Builtins(runtime::Scope &scope);

    } // namespace builtin

} // namespace cora::compiler

#endif // CORA_COMPILER_BUILTIN_BUILTIN_H