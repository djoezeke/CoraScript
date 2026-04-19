#ifndef CORA_COMPILER_BUILTIN_CLASS_H
#define CORA_COMPILER_BUILTIN_CLASS_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Cora/Compiler/Runtime/Value.hpp"

namespace cora::compiler
{

    namespace runtime
    {
        class Scope;
    } // namespace runtime

    namespace builtin
    {

#define CORA_NEW_CLASS(name, methods, fields) builtin::Class name = builtin::Class(#name, methods, fields)
#define CORA_CLASS_METOD(object, name, value, isPrivate) Class::AddMethod(object, name, value, isPrivate)
#define CORA_CLASS_FIELD(object, name, value, isPrivate) Class::AddField(object, name, value, isPrivate)

        class Class
        {
            using Obj = std::shared_ptr<runtime::Object>;
            using Field = runtime::Value;
            using Method = std::shared_ptr<runtime::Callable>;

        public:
            using Fields = std::unordered_map<std::string, Field>;
            using Methods = std::unordered_map<std::string, Method>;

        public:
            explicit Class(std::string name = {});
            Class(std::string name, const Methods methods, const Fields fields);

            const std::string &Name() const;
            std::shared_ptr<runtime::Scope> Scope() const;
            std::shared_ptr<runtime::Object> Object() const;

            Class &WithField(std::string name, Field field, bool isPrivate = false);
            Class &WithMethod(std::string name, runtime::Function::Func function, bool isPrivate = false);
            Class &WithMethod(std::string name, Method method, bool isPrivate = false);

            static void AddField(const Obj &object, const std::string &name, Field field, bool isPrivate = false);
            static void AddMethod(const Obj &object, const std::string &name, Method method, bool isPrivate = false);

            ~Class();

        private:
            std::string m_Name;
            Fields m_Fields;
            Methods m_Methods;
            std::unordered_set<std::string> m_PrivateMembers;
        };

    } // namespace builtin

} // namespace cora::compiler

#endif // CORA_COMPILER_BUILTIN_CLASS_H