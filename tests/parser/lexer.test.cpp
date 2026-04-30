#include "Cora/Basic/Error.hpp"

#include "Cora/Basic/Location.hpp"

#include "Lexer.hpp"

#include "Token.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    struct LexerOptions
    {
        bool print = false;
    };

    struct Args
    {
        std::string source;
        std::string sourceName{"<memory>"};
        std::string file;
        LexerOptions options;
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
        std::cerr << "Usage: lexer [--print-ir] <script-file>\n";
        std::cerr << "       lexer [--print-ir] <script-file>\n";
        return 1;
    }

    try
    {
        const Args args = ParseArgs(argc, argv);

        cora::parser::Lexer lexer(args.source);
        if (!args.file.empty())
        {
            lexer = cora::parser::Lexer(ReadFile(args.file));
            lexer.SetFileName(args.sourceName);
            lexer.SetModuleName(args.sourceName);
        }
        else
        {
            if (args.source.empty())
            {
                throw std::runtime_error("No input source provided");
            }
        }

        auto tokens = lexer.Tokenize();

        for (auto token : tokens)
        {
            std::cout << token;
            std::cout << ", ";
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
