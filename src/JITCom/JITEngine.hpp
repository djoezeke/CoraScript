#ifndef CORA_EMBED_ENGINE_HPP
#define CORA_EMBED_ENGINE_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace cora::compiler::runtime
{
    class Object;
}

namespace cora::embed
{
    struct JitConfig
    {
        bool enabled{true};
        bool tracing{true};
        std::uint32_t hotLoopThreshold{56};
    };

    struct EngineConfig
    {
        JitConfig jit{};
        bool autoloadBuiltinModules{true};
        std::vector<std::string> moduleSearchPaths{};
    };

    class Engine
    {
    public:
        using ModuleFactory = std::function<std::shared_ptr<cora::compiler::runtime::Object>()>;

        explicit Engine(EngineConfig config = {});
        ~Engine();

        Engine(const Engine &) = delete;
        Engine &operator=(const Engine &) = delete;
        Engine(Engine &&) noexcept;
        Engine &operator=(Engine &&) noexcept;

        int RunFile(const std::string &path);
        int RunString(const std::string &source, const std::string &virtualFile = "<memory>");

        void AddModuleSearchPath(const std::string &path);
        bool LoadModuleLibrary(const std::string &path);

        bool RegisterBuiltinModule(const std::string &name, ModuleFactory moduleFactory);
        void SetBuiltinModuleAutoload(bool enabled);

        const EngineConfig &Config() const;
        const std::string &LastError() const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_Impl;
    };
}

#endif
