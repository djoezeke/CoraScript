#include <Cora/Compiler/Builtin/Builtin.hpp>
#include <Cora/Compiler/Builtin/Class.hpp>
#include <Cora/Compiler/Builtin/Module.hpp>
#include <Cora/Compiler/Runtime/Scope.hpp>
#include <Cora/Compiler/Runtime/Value.hpp>

namespace cora
{
    using namespace compiler::runtime;
    using namespace compiler::builtin;

    namespace modules::exception
    {
        using namespace compiler;

        static builtin::Class MakeExceptionClass(const std::string &name, const std::string &bases, const std::string &doc)
        {
            builtin::Class klass(name, {}, {}, doc);
            klass.WithField("__bases__", runtime::Value(bases));

            klass.WithMethod("__init__", [name](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty() || !arguments.front().IsObject())
                                 {
                                     return runtime::Value(nullptr);
                                 }

                                 auto self = arguments.front().AsObject();
                                 const std::string message = arguments.size() > 1 ? arguments[1].AsString() : std::string();
                                 self->fields["message"] = runtime::Value(message);
                                 self->fields["__cause__"] = runtime::Value(name);
                                 return runtime::Value(nullptr); });

            klass.WithMethod("__str__", [name](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty() || !arguments.front().IsObject())
                                 {
                                     return runtime::Value(name);
                                 }

                                 auto self = arguments.front().AsObject();
                                 auto it = self->fields.find("message");
                                 if (it == self->fields.end())
                                 {
                                     return runtime::Value(name);
                                 }

                                 const std::string message = it->second.AsString();
                                 if (message.empty())
                                 {
                                     return runtime::Value(name);
                                 }

                                 return runtime::Value(name + ": " + message); });

            klass.WithMethod("__repr__", [name](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty() || !arguments.front().IsObject())
                                 {
                                     return runtime::Value("<" + name + ">");
                                 }

                                 auto self = arguments.front().AsObject();
                                 auto it = self->fields.find("message");
                                 const std::string message = it == self->fields.end() ? std::string() : it->second.AsString();
                                 return runtime::Value("<" + name + " message='" + message + "'>"); });

            return klass;
        }

        builtin::Module::Classes exception_classes = {
            {"BaseException", MakeExceptionClass("BaseException", "", "Root exception type.")},
            {"Exception", MakeExceptionClass("Exception", "BaseException", "Base class for all user-level exceptions.")},
            {"RuntimeError", MakeExceptionClass("RuntimeError", "Exception,BaseException", "Raised for runtime evaluation errors.")},
            {"TypeError", MakeExceptionClass("TypeError", "Exception,BaseException", "Raised for invalid type usage.")},
            {"ValueError", MakeExceptionClass("ValueError", "Exception,BaseException", "Raised for invalid values.")},
        };

        builtin::Module::Functions exception_functions = {};

        builtin::Module::Variables exception_variables = {
            {"module_name", runtime::Value("exception")},
        };

        CORA_NEW_MODULE(exception_module, exception_classes, exception_functions, exception_variables);

    } // namespace exception

} // namespace cora::modules

std::shared_ptr<cora::compiler::runtime::Object> CoraGetExceptionModuleObject()
{
    return cora::modules::exception::exception_module.Object();
}
