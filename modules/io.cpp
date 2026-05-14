#include "Cora/FFI.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>

class File {
public:
    File() : m_open(false) {}
    ~File() { close(); }

    bool open(std::string path, std::string mode) {
        if (mode == "r") m_file.open(path, std::ios::in);
        else if (mode == "w") m_file.open(path, std::ios::out);
        else if (mode == "a") m_file.open(path, std::ios::app);
        m_open = m_file.is_open();
        return m_open;
    }

    void close() {
        if (m_open) {
            m_file.close();
            m_open = false;
        }
    }

    void write(std::string text) {
        if (m_open) m_file << text;
    }

    std::string read_all() {
        if (!m_open) return "";
        // Reset to beginning if reading
        m_file.clear();
        m_file.seekg(0, std::ios::beg);
        std::string content((std::istreambuf_iterator<char>(m_file)), std::istreambuf_iterator<char>());
        return content;
    }

private:
    std::fstream m_file;
    bool m_open;
};

void print_val(cora::compiler::runtime::value v) {
    std::cout << v.AsString() << std::endl;
}

bool file_exists(std::string path) {
    return std::filesystem::exists(path);
}

CORA_MODULE(io, m) {
    m.def("print", &print_val);
    m.def("exists", &file_exists);

    cora::ffi::class_<File>(m, "File")
        .def_constructor()
        .def("open", &File::open)
        .def("close", &File::close)
        .def("write", &File::write)
        .def("read_all", &File::read_all);
}
