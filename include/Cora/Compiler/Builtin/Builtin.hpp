#ifndef CORA_COMPILER_BUILTIN_BUILTIN_H
#define CORA_COMPILER_BUILTIN_BUILTIN_H

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
        class NativeFunction;
    }

    namespace builtin
    {

        class BuiltinClassBuilder
        {
        public:
            explicit BuiltinClassBuilder(std::string className);

            BuiltinClassBuilder &Field(const std::string &name, runtime::Value value, bool isPrivate = false);
            BuiltinClassBuilder &Method(const std::string &name, std::function<runtime::Value(const std::vector<runtime::Value> &)> function, bool isPrivate = false);

            std::shared_ptr<runtime::Object> Build() const;

        private:
            std::shared_ptr<runtime::Object> m_Object;
        };

        std::shared_ptr<runtime::NativeFunction> MakeBuiltinFunction(std::string name, std::function<runtime::Value(const std::vector<runtime::Value> &)> function);
        std::shared_ptr<runtime::Object> MakeBuiltinObject(const std::string &className);

        void AddBuiltinField(const std::shared_ptr<runtime::Object> &object, const std::string &name, runtime::Value value, bool isPrivate = false);
        void AddBuiltinMethod(const std::shared_ptr<runtime::Object> &object, const std::string &name, std::function<runtime::Value(const std::vector<runtime::Value> &)> function, bool isPrivate = false);

        void RegisterBuiltinFunction(runtime::Scope &scope, const std::string &name, std::function<runtime::Value(const std::vector<runtime::Value> &)> function, bool constant = true);
        void RegisterBuiltinValue(runtime::Scope &scope, const std::string &name, runtime::Value value, bool constant = true);
        void RegisterBuiltinObject(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant = true);
        void RegisterStandardModules(runtime::Scope &scope);

        BuiltinClassBuilder MakeBuiltinClass(std::string className);

#define CORA_BUILTIN_FUNCTION(scope, name, function) ::cora::compiler::builtin::RegisterBuiltinFunction((scope), #name, (function))
#define CORA_BUILTIN_VALUE(scope, name, value) ::cora::compiler::builtin::RegisterBuiltinValue((scope), #name, (value))
#define CORA_BUILTIN_OBJECT(scope, name, object) ::cora::compiler::builtin::RegisterBuiltinObject((scope), #name, (object))
#define CORA_BUILTIN_CLASS(name) ::cora::compiler::builtin::MakeBuiltinClass(name)
#define CORA_BUILTIN_FIELD(object, name, value) ::cora::compiler::builtin::AddBuiltinField((object), #name, (value))
#define CORA_BUILTIN_METHOD(object, name, function) ::cora::compiler::builtin::AddBuiltinMethod((object), #name, (function))

        void Builtins(runtime::Scope &scope);

    } // namespace builtin

} // namespace cora::compiler

#endif // CORA_COMPILER_BUILTIN_BUILTIN_H