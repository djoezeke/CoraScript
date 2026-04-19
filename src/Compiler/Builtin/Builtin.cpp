#include "Cora/Compiler/Builtin/Builtin.hpp"
#include "Cora/Compiler/Builtin/Class.hpp"
#include "Cora/Compiler/Builtin/Module.hpp"

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

#include "Modules.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_set>

namespace cora::compiler::builtin
{
    std::shared_ptr<runtime::Object> MakeObject(const std::string &className)
    {
        return std::make_shared<runtime::Object>(className);
    };

    std::shared_ptr<runtime::Method> MakeMethod(const std::shared_ptr<runtime::Object> &object, std::string name, runtime::Method::Func function)
    {
        return std::make_shared<runtime::Method>(object, std::move(name), std::move(function));
    };

    std::shared_ptr<runtime::Function> MakeFunction(std::string name, runtime::Function::Func function)
    {
        return std::make_shared<runtime::Function>(std::move(name), std::move(function));
    };

    void RegisterClass(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant)
    {
        scope.SetVariableValue(name, new runtime::Value(object), constant);
    };

    void RegisterModule(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant)
    {
        scope.SetVariableValue(name, new runtime::Value(object), constant);
    };

    void RegisterFunction(runtime::Scope &scope, const std::string &name, std::function<runtime::Value(const std::vector<runtime::Value> &)> function, bool constant)
    {
        auto callable = MakeFunction(name, std::move(function));
        scope.SetVariableValue(name, new runtime::Value(std::static_pointer_cast<runtime::Callable>(callable)), constant);
    };

    void RegisterVariable(runtime::Scope &scope, const std::string &name, runtime::Value value, bool constant)
    {
        scope.SetVariableValue(name, new runtime::Value(std::move(value)), constant);
    };

    void Builtins(runtime::Scope &scope)
    {
        RegisterModule(scope, "io", CoraGetIoModuleObject());
    };

} // namespace cora::compiler::builtin