#include "../VMachine/Interpreter.hpp"
#include "Pipeline.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

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

    struct Args
    {
        std::string source;
        std::string sourceName{"<memory>"};
        std::string bytecodeInputFile;
        cora::tooling::FrontendOptions options;
    };

    Args ParseArgs(int argc, char **argv)
    {
        Args args;
        for (int i = 1; i < argc; ++i)
        {
            const std::string current = argv[i];
            if (current == "-c")
            {
                if (i + 1 >= argc)
                {
                    throw std::runtime_error("Missing source after -c");
                }
                args.source = argv[++i];
                args.sourceName = "<command>";
                continue;
            }

            if (current == "--load-bc")
            {
                if (i + 1 >= argc)
                {
                    throw std::runtime_error("Missing file after --load-bc");
                }
                args.bytecodeInputFile = argv[++i];
                continue;
            }

            if (current == "--save-bc")
            {
                if (i + 1 >= argc)
                {
                    throw std::runtime_error("Missing file after --save-bc");
                }
                args.options.bytecodeOutputFile = argv[++i];
                continue;
            }

            if (current == "--print-ir")
            {
                args.options.printIR = true;
                continue;
            }

            if (current == "--print-bytecode")
            {
                args.options.printBytecode = true;
                continue;
            }

            if (current == "--no-opt")
            {
                args.options.optimize = false;
                continue;
            }

            if (args.source.empty())
            {
                const std::filesystem::path inputPath(current);
                if (!std::filesystem::exists(inputPath))
                {
                    throw std::runtime_error("File not found: " + inputPath.string());
                }

                args.source = ReadFile(inputPath);
                args.sourceName = inputPath.string();
                continue;
            }

            throw std::runtime_error("Unexpected argument: " + current);
        }

        return args;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: cora [--print-ir] [--print-bytecode] [--save-bc out.cbc] [--no-opt] <script-file>\n";
        std::cerr << "       cora [--print-ir] [--print-bytecode] [--save-bc out.cbc] [--no-opt] -c \"<source>\"\n";
        std::cerr << "       cora --load-bc file.cbc\n";
        return 1;
    }

    try
    {
        const Args args = ParseArgs(argc, argv);

        cora::vmachine::BytecodeProgram program;
        if (!args.bytecodeInputFile.empty())
        {
            program = cora::tooling::LoadBytecodeFile(args.bytecodeInputFile);
        }
        else
        {
            if (args.source.empty())
            {
                throw std::runtime_error("No input source provided");
            }

            // std::cerr << "Compiling to bytecode...\n";
            const cora::tooling::FrontendResult result = cora::tooling::CompileToBytecode(
                args.source,
                args.sourceName,
                args.options,
                &std::cout,
                &std::cout);
            // std::cerr << "Compilation finished.\n";
            program = result.program;
        }

        // std::cerr << "Running interpreter...\n";
        cora::vmachine::Interpreter interpreter(&std::cout);
        const int rc = interpreter.Run(program);
        // std::cerr << "Interpreter finished with code: " << rc << "\n";
        if (rc != 0)
        {
            std::cerr << interpreter.LastError() << '\n';
        }
        return rc;
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
