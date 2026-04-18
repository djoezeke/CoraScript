#ifndef CORA_COMPILER_BUILTIN_BUILTIN_H
#define CORA_COMPILER_BUILTIN_BUILTIN_H

namespace cora::compiler
{
    namespace runtime
    {
        class Scope;
    }

    namespace builtin
    {
        void RegisterBuiltinFunctions(runtime::Scope &scope);
    } // namespace builtin

} // namespace cora::compiler

#endif // CORA_COMPILER_BUILTIN_BUILTIN_H