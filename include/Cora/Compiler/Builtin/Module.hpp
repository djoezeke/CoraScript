#ifndef CORA_COMPILER_BUILTIN_MODULE_H
#define CORA_COMPILER_BUILTIN_MODULE_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "Cora/Compiler/Builtin/Class.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

namespace cora::compiler
{

    namespace runtime
    {
        class Scope;
    } // namespace runtime

    namespace builtin
    {

#define CORA_NEW_MODULE(name, classes, functions, variables) builtin::Module name = builtin::Module(#name, classes, functions, variables)

        class Module
        {
            using Obj = std::shared_ptr<runtime::Object>;
            using Class = builtin::Class;
            using Variable = runtime::Value;
            using Function = std::shared_ptr<runtime::Callable>;

        public:
            using Func = std::function<runtime::Value(const std::vector<runtime::Value> &)>;

            using Classes = std::unordered_map<std::string, Class>;
            using Functions = std::unordered_map<std::string, Function>;
            using Variables = std::unordered_map<std::string, Variable>;

        public:
            Module(std::string name, Classes classes, Functions functions, const Variables variables, std::string doc= {});

            const std::string &Name() const;
            const std::string &Doc() const;

            std::shared_ptr<runtime::Scope> Scope() const;
            std::shared_ptr<runtime::Object> Object() const;

            Module &WithClass(std::string name, Class cls);
            Module &WithFunction(std::string name, runtime::Function::Func function);
            Module &WithFunction(std::string name, Function function);
            Module &WithVariable(std::string name, Variable variable);

            static void AddClass(const Obj &object, const std::string &name, Class cls);
            static void AddFunction(const Obj &object, const std::string &name, Function function);
            static void AddVariable(const Obj &object, const std::string &name, Variable variable);

            ~Module();

        private:
            std::string m_Doc;
            std::string m_Name;
            Classes m_Classes;
            Functions m_Functions;
            Variables m_Variables;
        };

    } // namespace builtin

} // namespace cora::compiler

#endif // CORA_COMPILER_BUILTIN_MODULE_H