#ifndef CORA_VMACHINE_SHAREDLIBRARY_H
#define CORA_VMACHINE_SHAREDLIBRARY_H

#include <Cora/Detail/Common.hpp>

#if defined(CORA_OS_WINDOWS)
#include <Windows.h>
#elif defined(CORA_OS_LINUX)
#include <dlfcn.h>
#else
#error "Can not identify the platform"
#endif

#include <string>
#include <system_error>

namespace cora::vmachine
{
    class SharedLibrary
    {
    public:
        SharedLibrary();

        SharedLibrary(const SharedLibrary &) = delete;

        SharedLibrary &operator=(const SharedLibrary &) = delete;

        SharedLibrary(SharedLibrary &&other) noexcept;

        SharedLibrary &operator=(SharedLibrary &&other) noexcept;

        explicit SharedLibrary(std::string path);

        void Load(const std::string &path);

        void Unload() const;

        void *Resolve(const char *symbolName) const;

        template <typename T>
        T Get(const std::string &procname)
        {
            T funcptr;

#if defined(CORA_OS_WINDOWS)
            if (NULL == (funcptr = reinterpret_cast<T>(GetProcAddress(m_handle, procname.c_str()))))
            {
                throw std::system_error(
                    std::error_code(::GetLastError(), std::system_category()), std::string("PluginError: Couldn't find ") + procname);
            }
#elif defined(CORA_OS_LINUX)
            if (NULL == (funcptr = reinterpret_cast<T>(dlsym(m_handle, procname.c_str()))))
            {
                throw std::system_error(
                    std::error_code(errno, std::system_category()), std::string("SharedLibraryError: Couldn't find {}" + procname + ", " + dlerror()));
            }
#endif
            return funcptr;
        }

        [[nodiscard]] const std::string &path() const { return m_path; }

        ~SharedLibrary();

    private:
#if defined(CORA_OS_WINDOWS)
        HMODULE m_handle;
#elif defined(CORA_OS_LINUX)
        void *m_handle;
#endif
        std::string m_path;
        bool m_loaded;
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_SHAREDLIBRARY_H
