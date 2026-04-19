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
    namespace modules::io
    {
        using namespace compiler;

        //////////////////////////////////////////////////
        //// MODULE VARIABLES
        //////////////////////////////////////////////////

        CORA_NEW_VARIABLE(modulename, "io");

        //////////////////////////////////////////////////
        //// MODULE FUNCTIONS
        //////////////////////////////////////////////////

        static runtime::Value print_impl(const std::vector<runtime::Value> &arguments, bool append_newline)
        {
            bool first = true;
            for (const runtime::Value &argument : arguments)
            {
                if (!first)
                {
                    std::cout << ' ';
                }
                std::cout << argument.AsString();
                first = false;
            }

            if (append_newline)
            {
                std::cout << '\n';
            }

            return runtime::Value(nullptr);
        }

        CORA_NEW_FUNCTION(print)
        {
            return print_impl(arguments, false);
        };

        CORA_NEW_FUNCTION(println)
        {
            return print_impl(arguments, true);
        };

        CORA_NEW_FUNCTION(exists)
        {
            if (arguments.empty())
            {
                return runtime::Value(false);
            }

            std::ifstream input(arguments[0].AsString(), std::ios::binary);
            return runtime::Value(static_cast<bool>(input));
        };

        CORA_NEW_FUNCTION(read_file)
        {
            if (arguments.empty())
            {
                return runtime::Value("");
            }

            std::ifstream input(arguments[0].AsString(), std::ios::binary);
            if (!input)
            {
                return runtime::Value("");
            }

            std::ostringstream buffer;
            buffer << input.rdbuf();
            return runtime::Value(buffer.str());
        };

        CORA_NEW_FUNCTION(write_file)
        {
            if (arguments.size() < 2)
            {
                return runtime::Value(false);
            }

            std::ofstream output(arguments[0].AsString(), std::ios::binary | std::ios::trunc);
            if (!output)
            {
                return runtime::Value(false);
            }

            output << arguments[1].AsString();
            return runtime::Value(static_cast<bool>(output));
        };

        CORA_NEW_FUNCTION(append_file)
        {
            if (arguments.size() < 2)
            {
                return runtime::Value(false);
            }

            std::ofstream output(arguments[0].AsString(), std::ios::binary | std::ios::app);
            if (!output)
            {
                return runtime::Value(false);
            }

            output << arguments[1].AsString();
            return runtime::Value(static_cast<bool>(output));
        };

        CORA_NEW_FUNCTION(remove_file)
        {
            if (arguments.empty())
            {
                return runtime::Value(false);
            }

            std::error_code error;
            const bool removed = std::filesystem::remove(arguments[0].AsString(), error);
            if (error)
            {
                return runtime::Value(false);
            }
            return runtime::Value(removed);
        };

        CORA_NEW_FUNCTION(file_size)
        {
            if (arguments.empty())
            {
                return runtime::Value(0.0);
            }

            std::error_code error;
            const std::uintmax_t size = std::filesystem::file_size(arguments[0].AsString(), error);
            if (error)
            {
                return runtime::Value(0.0);
            }

            return runtime::Value(static_cast<double>(size));
        };

        CORA_NEW_FUNCTION(cwd)
        {
            std::error_code error;
            const auto path = std::filesystem::current_path(error);
            if (error)
            {
                return runtime::Value("");
            }

            return runtime::Value(path.string());
        };

        CORA_NEW_FUNCTION(input)
        {
            if (!arguments.empty())
            {
                std::cout << arguments[0].AsString();
                std::cout.flush();
            }

            std::string line;
            std::getline(std::cin, line);
            return runtime::Value(line);
        };

        CORA_NEW_FUNCTION(greet)
        {
            return runtime::Value("hello from io.creator");
        };

        CORA_NEW_FUNCTION(program)
        {
            return runtime::Value("CoraScript");
        };

        CORA_NEW_FUNCTION(describe)
        {
            return runtime::Value("builtin io creator object");
        };

        CORA_NEW_FUNCTION(new_creator)
        {
            auto object = CORA_NEW_OBJECT("creator");
            builtin::Class::AddMethod(object, "greet", std::static_pointer_cast<runtime::Callable>(greet));
            builtin::Class::AddMethod(object, "program", std::static_pointer_cast<runtime::Callable>(program));
            builtin::Class::AddMethod(object, "describe", std::static_pointer_cast<runtime::Callable>(describe));

            return runtime::Value(std::move(object));
        };

        CORA_NEW_FUNCTION(module_name)
        {
            return runtime::Value("io");
        };

        CORA_NEW_FUNCTION(version)
        {
            return runtime::Value("1.0.0");
        };

        //////////////////////////////////////////////////
        //// MODULE CLASSES
        //////////////////////////////////////////////////

        builtin::Class::Methods creator_methods = {
            {"greet", std::static_pointer_cast<runtime::Callable>(greet)},
            {"program", std::static_pointer_cast<runtime::Callable>(program)},
            {"describe", std::static_pointer_cast<runtime::Callable>(describe)},
        };

        builtin::Class::Fields creator_fields = {
            {"module", runtime::Value("io")},
        };

        CORA_NEW_CLASS(creator, creator_methods, creator_fields);

        //////////////////////////////////////////////////
        //// CREATE MODULE
        //////////////////////////////////////////////////

        builtin::Module::Classes io_classes = {
            {"creator", creator},
        };

        builtin::Module::Functions io_functions = {
            {"print", std::static_pointer_cast<runtime::Callable>(print)},
            {"println", std::static_pointer_cast<runtime::Callable>(println)},
            {"exists", std::static_pointer_cast<runtime::Callable>(exists)},
            {"read_file", std::static_pointer_cast<runtime::Callable>(read_file)},
            {"write_file", std::static_pointer_cast<runtime::Callable>(write_file)},
            {"append_file", std::static_pointer_cast<runtime::Callable>(append_file)},
            {"remove_file", std::static_pointer_cast<runtime::Callable>(remove_file)},
            {"file_size", std::static_pointer_cast<runtime::Callable>(file_size)},
            {"cwd", std::static_pointer_cast<runtime::Callable>(cwd)},
            {"input", std::static_pointer_cast<runtime::Callable>(input)},
            {"new_creator", std::static_pointer_cast<runtime::Callable>(new_creator)},
            {"module", std::static_pointer_cast<runtime::Callable>(module_name)},
            {"version", std::static_pointer_cast<runtime::Callable>(version)},
        };

        builtin::Module::Variables io_variables = {
            {"__module__", modulename},
            {"__version__", runtime::Value("1.0.0")},
        };

        CORA_NEW_MODULE(io_module, io_classes, io_functions, io_variables);

    } // namespace io

} // namespace cora::modules

std::shared_ptr<cora::compiler::runtime::Object> CoraGetIoModuleObject()
{
    return cora::modules::io::io_module.Object();
}
