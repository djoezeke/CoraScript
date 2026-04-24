#include "../JITCom/JITEngine.hpp"

#include <filesystem>
#include <iostream>

using cora::embed::Engine;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: cora <script-file>\n";
        std::cerr << "       cora -c \"<source>\"\n";
        return 1;
    }

    Engine engine;

    if (std::string(argv[1]) == "-c")
    {
        if (argc < 3)
        {
            std::cerr << "Missing source after -c\n";
            return 1;
        }

        const int rc = engine.RunString(argv[2], "<command>");
        if (rc != 0)
        {
            std::cerr << engine.LastError() << '\n';
        }
        return rc;
    }

    const std::filesystem::path scriptPath(argv[1]);
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
