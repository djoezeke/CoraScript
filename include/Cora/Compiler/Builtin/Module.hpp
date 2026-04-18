#ifndef CORA_COMPILER_BUILTIN_MODULE_H
#define CORA_COMPILER_BUILTIN_MODULE_H

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

namespace cora::compiler
{

    namespace builtin
    {
        class Module
        {
        public:
            Module(std::string name, std::string fields, std::string functions, , std::string classes);

            std::shared_ptr<runtime::Object> Build() const;

            ~Module();

        private:
            const std::string m_Name;
            std::shared_ptr<runtime::Object> m_Object;
        };


    } // namespace builtin

} // namespace cora::compiler

#endif // CORA_COMPILER_BUILTIN_MODULE_H