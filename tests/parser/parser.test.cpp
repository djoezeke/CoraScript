#include "Cora/Basic/Error.hpp"

#include "Parser.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    struct ParserOptions
    {
        bool print = false;
    };

    struct Args
    {
        std::string source;
        std::string sourceName{"<memory>"};
        std::string file;
        ParserOptions options;
    };

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

            if (current == "--print")
            {
                args.options.print = true;
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
        std::cerr << "Usage: parser [--print] <script-file>\n";
        std::cerr << "       parser [--print] <script-file>\n";
        return 0;
    }

    try
    {
        const Args args = ParseArgs(argc, argv);

        cora::parser::Lexer lexer(args.source);
        if (!args.file.empty())
        {
            lexer = cora::parser::Lexer(ReadFile(args.file));
        }
        else
        {
            if (args.source.empty())
            {
                throw std::runtime_error("No input source provided");
            }
        }

        cora::parser::Parser parser(lexer.Tokenize());
        auto statements = parser.Parse();

        for (auto statement : statements)
        {
            std::cout << statement;
        };

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
