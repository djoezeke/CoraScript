#include "../IRGen/IRBuilder.hpp"

#include "../IRGen/Bytecode.hpp"

#include "../Parser/Parser.hpp"
#include "../Semantic/Validator.hpp"
#include "Cora/Basic/Error.hpp"

#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using cora::compiler::ast::Statement;
using cora::embed::internal::BytecodeCompiler;
using cora::embed::internal::BytecodeProgram;

namespace
{
    std::string ReadFile(const std::filesystem::path &path)
    {
        std::ifstream input(path);
        if (!input)
        {
            throw std::runtime_error("Unable to open file: " + path.string());
        }

        std::stringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    }

    int CompileSource(const std::string &source, const std::string &fileName)
    {
        cora::compiler::parser::Parser parser;
        parser.SetFileName(fileName);
        parser.SetModuleName(std::filesystem::path(fileName).stem().string());

        std::deque<Statement *> program = parser.ParseProgram(source);

        try
        {
            cora::compiler::semantic::ValidateProgram(
                program,
                fileName,
                std::filesystem::path(fileName).stem().string());

            BytecodeCompiler compiler;
            BytecodeProgram bytecode = compiler.Compile(program);

            std::cout << "Bytecode compile successful\n";
            std::cout << "  instructions: " << bytecode.code.size() << "\n";
            std::cout << "  constants:    " << bytecode.constants.size() << "\n";
            std::cout << "  names:        " << bytecode.names.size() << "\n";
        }
        catch (...)
        {
            for (Statement *statement : program)
            {
                delete statement;
            }
            throw;
        }

        for (Statement *statement : program)
        {
            delete statement;
        }

        return 0;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: corac <script-file>\n";
        std::cerr << "       corac -c \"<source>\"\n";
        return 1;
    }

    try
    {
        if (std::string(argv[1]) == "-c")
        {
            if (argc < 3)
            {
                std::cerr << "Missing source after -c\n";
                return 1;
            }

            return CompileSource(argv[2], "<command>");
        }

        const std::filesystem::path filePath(argv[1]);
        if (!std::filesystem::exists(filePath))
        {
            std::cerr << "File not found: " << filePath.string() << '\n';
            return 1;
        }

        return CompileSource(ReadFile(filePath), filePath.string());
    }
    catch (const cora::compiler::error::Error &error)
    {
        std::cerr << error.Format() << '\n';
        return 1;
    }
    catch (const std::runtime_error &error)
    {
        std::cerr << error.what() << '\n';
        return 2;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
