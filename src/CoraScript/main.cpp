#include "Cora/Compiler/Error/Error.hpp"
#include "Cora/Compiler/Runtime/Interpreter.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

using namespace cora::compiler::runtime;
using namespace cora::compiler::error;

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        std::cerr << "Usage: corascript <script-file>\n";
        return 1;
    }

    try
    {
        std::filesystem::path scriptPath(argv[1]);
        Interpreter interpreter;
        interpreter.SetFileName(std::string(argv[1]));
        interpreter.RunFile(argv[1]);
        return 0;
    }
    catch (const Error &error)
    {
        std::cerr << error.Format();
        return 2;
    }
    catch (const std::exception &error)
    {
        std::cerr << "exception";
        DiagnosticContext context;
        context.fileName = argv[1];
        RuntimeError wrapped(error.what(), context);
        std::cerr << wrapped.Format();
        return 2;
    }
}
