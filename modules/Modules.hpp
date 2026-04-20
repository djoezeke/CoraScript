#ifndef CORA_COMPILER_MODULES_MODULES_H
#define CORA_COMPILER_MODULES_MODULES_H

#include <Cora/Compiler/Builtin/Module.hpp>

std::shared_ptr<cora::compiler::runtime::Object> CoraGetIOModuleObject();
std::shared_ptr<cora::compiler::runtime::Object> CoraGetOSModuleObject();
std::shared_ptr<cora::compiler::runtime::Object> CoraGetMathModuleObject();
std::shared_ptr<cora::compiler::runtime::Object> CoraGetExceptionModuleObject();

#endif // CORA_COMPILER_MODULES_MODULES_H
