#include <Cora/Compiler/Builtin/Builtin.hpp>
#include <Cora/Compiler/Builtin/Class.hpp>
#include <Cora/Compiler/Builtin/Module.hpp>
#include <Cora/Compiler/Runtime/Scope.hpp>
#include <Cora/Compiler/Runtime/Value.hpp>

#include <cmath>

namespace cora
{
    using namespace compiler::runtime;
    using namespace compiler::builtin;

    namespace modules::math
    {
        using namespace compiler;

        //////////////////////////////////////////////////
        //// MODULE VARIABLES
        //////////////////////////////////////////////////

        //////////////////////////////////////////////////
        //// MODULE FUNCTIONS
        //////////////////////////////////////////////////

#define ADD_MATH_FUNCTION(func) {#func, std::static_pointer_cast<runtime::Callable>(func)}

        CORA_NEW_FUNCTION(abs)
        {
            return runtime::Value(std::abs(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(acos)
        {
            return runtime::Value(std::acos(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(asin)
        {
            return runtime::Value(std::asin(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(atan)
        {
            return runtime::Value(std::atan(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(atan2)
        {
            return runtime::Value(std::atan2(arguments.front().AsNumber(), arguments.back().AsNumber()));
        };

        CORA_NEW_FUNCTION(cos)
        {
            return runtime::Value(std::cos(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(sin)
        {
            return runtime::Value(std::sin(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(tan)
        {
            return runtime::Value(std::tan(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(cosh)
        {
            return runtime::Value(std::cosh(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(sinh)
        {
            return runtime::Value(std::sinh(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(tanh)
        {
            return runtime::Value(std::tanh(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(exp)
        {
            return runtime::Value(std::exp(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(frexp)
        {
            // return runtime::Value(std::frexp(arguments.front().AsNumber()));
            return runtime::Value(0.0);
        };

        CORA_NEW_FUNCTION(ldexp)
        {
            return runtime::Value(std::ldexp(arguments.front().AsNumber(), arguments.back().AsNumber()));
        };

        CORA_NEW_FUNCTION(log)
        {
            return runtime::Value(std::log(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(log10)
        {
            return runtime::Value(std::log10(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(modf)
        {
            // return runtime::Value(std::modf(arguments.front().AsNumber()));
            return runtime::Value(0.0);
        };

        CORA_NEW_FUNCTION(pow)
        {
            return runtime::Value(std::pow(arguments.front().AsNumber(), arguments.back().AsNumber()));
        };

        CORA_NEW_FUNCTION(sqrt)
        {
            return runtime::Value(std::sqrt(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(ceil)
        {
            return runtime::Value(std::ceil(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(fabs)
        {
            return runtime::Value(std::fabs(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(floor)
        {
            return runtime::Value(std::floor(arguments.front().AsNumber()));
        };

        CORA_NEW_FUNCTION(fmod)
        {
            return runtime::Value(std::fmod(arguments.front().AsNumber(), arguments.back().AsNumber()));
        };

        //////////////////////////////////////////////////
        //// MODULE CLASSES
        //////////////////////////////////////////////////

        //////////////////////////////////////////////////
        //// CREATE MODULE
        //////////////////////////////////////////////////

        builtin::Module::Classes math_classes = {};

        builtin::Module::Functions math_functions = {
            ADD_MATH_FUNCTION(abs),
            ADD_MATH_FUNCTION(acos),
            ADD_MATH_FUNCTION(asin),
            ADD_MATH_FUNCTION(atan),
            ADD_MATH_FUNCTION(atan2),
            ADD_MATH_FUNCTION(cos),
            ADD_MATH_FUNCTION(sin),
            ADD_MATH_FUNCTION(tan),
            ADD_MATH_FUNCTION(cosh),
            ADD_MATH_FUNCTION(sinh),
            ADD_MATH_FUNCTION(tanh),
            ADD_MATH_FUNCTION(exp),
            ADD_MATH_FUNCTION(frexp),
            ADD_MATH_FUNCTION(ldexp),
            ADD_MATH_FUNCTION(log),
            ADD_MATH_FUNCTION(log10),
            ADD_MATH_FUNCTION(modf),
            ADD_MATH_FUNCTION(pow),
            ADD_MATH_FUNCTION(sqrt),
            ADD_MATH_FUNCTION(ceil),
            ADD_MATH_FUNCTION(fabs),
            ADD_MATH_FUNCTION(floor),
            ADD_MATH_FUNCTION(fmod),
        };

        builtin::Module::Variables math_variables = {
            {"e", runtime::Value(std::exp(1.0))},
            {"pi", runtime::Value(std::acos(-1.0))},
            {"inf", runtime::Value(1.0)},
            {"nan", runtime::Value(1.0)},
            {"tau", runtime::Value(2.0 * std::acos(-1.0))},
        };

        CORA_NEW_MODULE(math_module, math_classes, math_functions, math_variables);

    } // namespace math

} // namespace cora::modules

std::shared_ptr<cora::compiler::runtime::Object> CoraGetMathModuleObject()
{
    return cora::modules::math::math_module.Object();
}
