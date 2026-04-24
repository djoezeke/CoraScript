#include "../JITCom/JITEngine.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: coravm [--no-jit] [--hot N] <script-file>\n";
        std::cerr << "       coravm [--no-jit] [--hot N] -c \"<source>\"\n";
        return 1;
    }

    cora::embed::EngineConfig config{};
    std::string scriptArg;
    std::string commandSource;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--no-jit")
        {
            config.jit.enabled = false;
            continue;
        }

        if (arg == "--hot")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing value for --hot\n";
                return 1;
            }

            ++i;
            config.jit.hotLoopThreshold = static_cast<std::uint32_t>(std::stoul(argv[i]));
            continue;
        }

        if (arg == "-c")
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing source after -c\n";
                return 1;
            }

            ++i;
            commandSource = argv[i];
            continue;
        }

        scriptArg = arg;
    }

    cora::embed::Engine engine(config);

    if (!commandSource.empty())
    {
        const int rc = engine.RunString(commandSource, "<command>");
        if (rc != 0)
        {
            std::cerr << engine.LastError() << '\n';
        }
        return rc;
    }

    if (scriptArg.empty())
    {
        std::cerr << "Missing script file\n";
        return 1;
    }

    const std::filesystem::path scriptPath(scriptArg);
    if (!std::filesystem::exists(scriptPath))
    {
        std::cerr << "File not found: " << scriptPath.string() << '\n';
        return 1;
    }

    const int rc = engine.RunFile(scriptPath.string());
    if (rc != 0)
    {
        std::cerr << engine.LastError() << '\n';
    }

    return rc;
}
