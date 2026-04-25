#include "SharedLibrary.hpp"

#include <iostream>
#include <sstream>

namespace cora::vmachine
{
    SharedLibrary::SharedLibrary()
        : m_handle(nullptr), m_loaded(false) {};

    SharedLibrary::SharedLibrary(std::string path)
        : m_handle(nullptr), m_path(std::move(path)), m_loaded(false)
    {
        Load(m_path);
    }

    SharedLibrary::~SharedLibrary()
    {
        Unload();
    }

    void SharedLibrary::Load(const std::string &path)
    {
        if (m_loaded)
            Unload();

        m_path = path;

#if defined(CORA_OS_WINDOWS)
        if (NULL == (m_handle = LoadLibraryA(m_path.c_str())))
        {
            throw std::system_error(
                std::error_code(::GetLastError(), std::system_category()), std::string("Couldn't load the library at " + path));
        }
#elif defined(CORA_OS_LINUX)
        if (nullptr == (m_handle = dlopen(m_path.c_str(), RTLD_LAZY | RTLD_GLOBAL)))
        {
            throw std::system_error(
                std::error_code(errno, std::system_category()), std::string("Couldn't load the library at " + path + ", "+ dlerror()));
        }
#endif
        m_loaded = true;
    };

    void *SharedLibrary::Resolve(const char *symbolName) const
    {
        if (!m_handle)
        {
            return nullptr;
        }

#if defined(_WIN32)
        return reinterpret_cast<void *>(GetProcAddress(static_cast<HMODULE>(m_handle), symbolName));
#else
        return dlsym(m_handle, symbolName);
#endif
    };

    void SharedLibrary::Unload() const
    {
        if (m_loaded)
        {
#if defined(CORA_OS_WINDOWS)
            FreeLibrary(m_handle);
#elif defined(CORA_OS_LINUX)
            dlclose(m_handle);
#endif
        }
    };

} // namespace cora::vmachine
