#include "JITEngine.hpp"
#include "JITPipeline.hpp"

#include "../IRGen/IRBuilder.hpp"
#include "../VMachine/VMachine.hpp"

#include "../Parser/Parser.hpp"
#include "../Semantic/Validator.hpp"
#include "../VMachine/Interpreter.hpp"
#include "Cora/Basic/Error.hpp"

#if defined(CORA_WITH_BUILTIN_MODULES)
#include "Modules.hpp"
#endif

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

namespace cora::embed
{
    namespace
    {
        class DynamicLibrary
        {
        public:
            explicit DynamicLibrary(const std::string &path)
                : m_Path(path)
            {
#if defined(_WIN32)
                m_Handle = LoadLibraryA(path.c_str());
#else
                m_Handle = dlopen(path.c_str(), RTLD_NOW);
#endif
                if (!m_Handle)
                {
                    throw std::runtime_error("Failed to load module library: " + path);
                }
            }

            DynamicLibrary(const DynamicLibrary &) = delete;
            DynamicLibrary &operator=(const DynamicLibrary &) = delete;

            DynamicLibrary(DynamicLibrary &&other) noexcept
                : m_Path(std::move(other.m_Path)),
                  m_Handle(other.m_Handle)
            {
                other.m_Handle = nullptr;
            }

            DynamicLibrary &operator=(DynamicLibrary &&other) noexcept
            {
                if (this == &other)
                {
                    return *this;
                }

                Unload();
                m_Path = std::move(other.m_Path);
                m_Handle = other.m_Handle;
                other.m_Handle = nullptr;
                return *this;
            }

            ~DynamicLibrary()
            {
                Unload();
            }

            void *Resolve(const char *symbolName) const
            {
                if (!m_Handle)
                {
                    return nullptr;
                }

#if defined(_WIN32)
                return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(m_Handle), symbolName));
#else
                return dlsym(m_Handle, symbolName);
#endif
            }

        private:
            void Unload()
            {
                if (!m_Handle)
                {
                    return;
                }

#if defined(_WIN32)
                FreeLibrary(static_cast<HMODULE>(m_Handle));
#else
                dlclose(m_Handle);
#endif

                m_Handle = nullptr;
            }

        private:
            std::string m_Path;
            void *m_Handle{nullptr};
        };
    } // namespace

    class Engine::Impl
    {
    public:
        explicit Impl(EngineConfig config)
            : m_Config(std::move(config)), m_Jit(m_Config.jit), m_Vm(&m_Jit)
        {
            for (const std::string &searchPath : m_Config.moduleSearchPaths)
            {
                if (!searchPath.empty())
                {
                    m_ModuleSearchPaths.push_back(searchPath);
                }
            }

            if (m_Config.autoloadBuiltinModules)
            {
                RegisterDefaultBuiltinModules();
                LoadRegisteredBuiltinModules();
            }
        }

        int RunFile(const std::string &path)
        {
            try
            {
                const std::filesystem::path fullPath(path);
                const auto parent = fullPath.parent_path();
                if (!parent.empty())
                {
                    AddModuleSearchPath(parent.string());
                }

                std::ifstream input(path);
                if (!input)
                {
                    throw std::runtime_error("Unable to open file: " + path);
                }

                std::stringstream buffer;
                buffer << input.rdbuf();
                RunSource(buffer.str(), path);
                m_LastError.clear();
                return 0;
            }
            catch (const compiler::error::Error &error)
            {
                m_LastError = error.Format();
            }
            catch (const std::exception &error)
            {
                m_LastError = error.what();
            }

            return 1;
        }

        int RunString(const std::string &source, const std::string &virtualFile)
        {
            try
            {
                RunSource(source, virtualFile);
                m_LastError.clear();
                return 0;
            }
            catch (const compiler::error::Error &error)
            {
                m_LastError = error.Format();
            }
            catch (const std::exception &error)
            {
                m_LastError = error.what();
            }

            return 1;
        }

        void AddModuleSearchPath(const std::string &path)
        {
            if (path.empty())
            {
                return;
            }

            m_ModuleSearchPaths.push_back(path);
        }

        bool LoadModuleLibrary(Engine &engine, const std::string &path)
        {
            try
            {
                DynamicLibrary library(path);

                m_LoadedLibraries.push_back(std::move(library));
                m_LastError.clear();
                return true;
            }
            catch (const std::exception &error)
            {
                m_LastError = error.what();
                return false;
            }
        }

        bool RegisterBuiltinModule(const std::string &name, ModuleFactory moduleFactory)
        {
            if (name.empty() || !moduleFactory)
            {
                return false;
            }

            m_BuiltinModules[name] = std::move(moduleFactory);
            return true;
        }

        void SetBuiltinModuleAutoload(bool enabled)
        {
            m_Config.autoloadBuiltinModules = enabled;
            if (enabled)
            {
                LoadRegisteredBuiltinModules();
            }
        }

        void LoadRegisteredBuiltinModules()
        {
            for (const auto &entry : m_BuiltinModules)
            {
                const auto moduleObject = entry.second();
                if (!moduleObject)
                {
                    continue;
                }

                try
                {
                    m_Interpreter.RegisterModule(entry.first, moduleObject, true);
                }
                catch (const std::exception &)
                {
                }

                m_Vm.SetGlobal(entry.first, compiler::runtime::Value(moduleObject));
            }
        }

        const EngineConfig &Config() const
        {
            return m_Config;
        }

        const std::string &LastError() const
        {
            return m_LastError;
        }

    private:
        void RunSource(const std::string &source, const std::string &fileName)
        {
            compiler::parser::Parser parser;
            parser.SetFileName(fileName);
            parser.SetModuleName(std::filesystem::path(fileName).stem().string());

            std::deque<compiler::ast::Statement *> program = parser.ParseProgram(source);
            compiler::semantic::ValidateProgram(program, fileName, std::filesystem::path(fileName).stem().string());

            try
            {
                internal::BytecodeProgram bytecode;
                try
                {
                    bytecode = m_Compiler.Compile(program);
                }
                catch (const std::runtime_error &error)
                {
                    const std::string message = error.what();
                    if (message.find("VM bytecode compiler does not support") != std::string::npos)
                    {
                        m_Interpreter.SetFileName(fileName);
                        m_Interpreter.Run(source);
                        return;
                    }
                    throw;
                }

                (void)m_Vm.Execute(bytecode);
            }
            catch (...)
            {
                for (compiler::ast::Statement *statement : program)
                {
                    delete statement;
                }
                throw;
            }

            for (compiler::ast::Statement *statement : program)
            {
                delete statement;
            }
        }

        void RegisterDefaultBuiltinModules()
        {
#if defined(CORA_WITH_BUILTIN_MODULES)
            RegisterBuiltinModule("io", []
                                  { return CoraGetIOModuleObject(); });
            RegisterBuiltinModule("os", []
                                  { return CoraGetOSModuleObject(); });
            RegisterBuiltinModule("math", []
                                  { return CoraGetMathModuleObject(); });
            RegisterBuiltinModule("exception", []
                                  { return CoraGetExceptionModuleObject(); });
#endif
        }

    private:
        EngineConfig m_Config;
        internal::JitPipeline m_Jit;
        internal::BytecodeCompiler m_Compiler;
        internal::BytecodeVm m_Vm;
        compiler::runtime::Interpreter m_Interpreter;
        std::string m_LastError;
        std::vector<std::string> m_ModuleSearchPaths;
        std::vector<DynamicLibrary> m_LoadedLibraries;
        std::unordered_map<std::string, ModuleFactory> m_BuiltinModules;
    };

    Engine::Engine(EngineConfig config)
        : m_Impl(std::make_unique<Impl>(std::move(config)))
    {
    }

    Engine::~Engine() = default;

    Engine::Engine(Engine &&) noexcept = default;

    Engine &Engine::operator=(Engine &&) noexcept = default;

    int Engine::RunFile(const std::string &path)
    {
        return m_Impl->RunFile(path);
    }

    int Engine::RunString(const std::string &source, const std::string &virtualFile)
    {
        return m_Impl->RunString(source, virtualFile);
    }

    void Engine::AddModuleSearchPath(const std::string &path)
    {
        m_Impl->AddModuleSearchPath(path);
    }

    bool Engine::LoadModuleLibrary(const std::string &path)
    {
        return m_Impl->LoadModuleLibrary(*this, path);
    }

    bool Engine::RegisterBuiltinModule(const std::string &name, ModuleFactory moduleFactory)
    {
        const bool registered = m_Impl->RegisterBuiltinModule(name, std::move(moduleFactory));
        if (registered && m_Impl->Config().autoloadBuiltinModules)
        {
            m_Impl->LoadRegisteredBuiltinModules();
        }
        return registered;
    }

    void Engine::SetBuiltinModuleAutoload(bool enabled)
    {
        m_Impl->SetBuiltinModuleAutoload(enabled);
    }

    const EngineConfig &Engine::Config() const
    {
        return m_Impl->Config();
    }

    const std::string &Engine::LastError() const
    {
        return m_Impl->LastError();
    }
}
