#ifndef CORA_COMPILER_BUILTIN_CLASS_H
#define CORA_COMPILER_BUILTIN_CLASS_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

namespace cora::compiler
{
    namespace builtin
    {

        class Class
        {
        public:
            Class(std::string name, std::string fields, std::string methods);

            std::shared_ptr<runtime::Object> Build() const;

            ~Class();

        private:
            const std::string m_Name;
            std::shared_ptr<runtime::Object> m_Object;
        };

    } // namespace builtin

} // namespace cora::compiler

#endif // CORA_COMPILER_BUILTIN_CLASS_H