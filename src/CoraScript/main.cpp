#include "Cora/Compiler/Runtime/Interpreter.hpp"

#include <exception>
#include <iostream>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: corascript <script-file>\n";
        return 1;
    }

    try
    {
        cora::script::Interpreter interpreter;
        interpreter.RunFile(argv[1]);
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return 2;
    }
}
