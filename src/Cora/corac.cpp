
#include "Cora/Basic/Error.hpp"
#include "Pipeline.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

            if (current == "-o" || current == "--emit-bc")
            {
                if (i + 1 >= argc)
                {
                    throw std::runtime_error("Missing output file after -o/--emit-bc");
                }

                args.options.bytecodeOutputFile = argv[++i];
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

        if (args.source.empty())
        {
            throw std::runtime_error("No input source provided");
        }

        if (args.options.bytecodeOutputFile.empty() && args.sourceName != "<command>")
        {
            std::filesystem::path srcPath(args.sourceName);
            args.options.bytecodeOutputFile = srcPath.replace_extension(".cbc").string();
        }

        return args;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: corac [--print-ir] [--print-bytecode] [--no-opt] [-o out.cbc] <script-file>\n";
        std::cerr << "       corac [--print-ir] [--print-bytecode] [--no-opt] [-o out.cbc] -c \"<source>\"\n";
        return 1;
    }

    try
    {
        const Args args = ParseArgs(argc, argv);
        const cora::tooling::FrontendResult result = cora::tooling::CompileToBytecode(
            args.source,
            args.sourceName,
            args.options,
            &std::cout,
            &std::cout);

        std::cout << "Bytecode compile successful\n";
        std::cout << "  instructions: " << result.program.code.size() << "\n";
        std::cout << "  constants:    " << result.program.constants.size() << "\n";
        std::cout << "  names:        " << result.program.names.size() << "\n";

        if (!args.options.bytecodeOutputFile.empty())
        {
            std::cout << "  wrote:        " << args.options.bytecodeOutputFile << "\n";
        }

        return 0;
    }
    catch (const cora::error::Error &error)
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
