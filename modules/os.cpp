
#include <Cora/Compiler/Builtin/Builtin.hpp>
#include <Cora/Compiler/Builtin/Class.hpp>
#include <Cora/Compiler/Builtin/Module.hpp>
#include <Cora/Compiler/Runtime/Scope.hpp>
#include <Cora/Compiler/Runtime/Value.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace cora
{
    using namespace compiler::runtime;
    using namespace compiler::builtin;

    namespace modules::os
    {
        using namespace compiler;

        //////////////////////////////////////////////////
        //// MODULE VARIABLES
        //////////////////////////////////////////////////

        //////////////////////////////////////////////////
        //// MODULE FUNCTIONS
        //////////////////////////////////////////////////

        //////////////////////////////////////////////////
        //// MODULE CLASSES
        //////////////////////////////////////////////////

        //////////////////////////////////////////////////
        //// CREATE MODULE
        //////////////////////////////////////////////////

        builtin::Module::Classes os_classes = { };

        builtin::Module::Functions os_functions = {
        };

        builtin::Module::Variables os_variables = {
        };

        CORA_NEW_MODULE(os_module, os_classes, os_functions, os_variables);

    } // namespace os

} // namespace cora::modules

std::shared_ptr<cora::compiler::runtime::Object> CoraGetOSModuleObject()
{
    return cora::modules::os::os_module.Object();
}
