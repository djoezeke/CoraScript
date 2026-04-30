#include "BytecodeWriter.hpp"

#include <fstream>
#include <stdexcept>

namespace cora::vmachine
{
    namespace
    {
        constexpr std::uint32_t kMagic = 0x434F5241U; // CORA
        constexpr std::uint32_t kVersion = 1U;

        void WriteU32(std::ostream &out, std::uint32_t value)
        {
            out.write(reinterpret_cast<const char *>(&value), sizeof(value));
        }

        void WriteI32(std::ostream &out, std::int32_t value)
        {
            out.write(reinterpret_cast<const char *>(&value), sizeof(value));
        }

        void WriteI64(std::ostream &out, std::int64_t value)
        {
            out.write(reinterpret_cast<const char *>(&value), sizeof(value));
        }

        void WriteDouble(std::ostream &out, double value)
        {
            out.write(reinterpret_cast<const char *>(&value), sizeof(value));
        }

        void WriteString(std::ostream &out, const std::string &value)
        {
            WriteU32(out, static_cast<std::uint32_t>(value.size()));
            if (!value.empty())
            {
                out.write(value.data(), static_cast<std::streamsize>(value.size()));
            }
        }
    }

    void BytecodeWriter::Write(const std::string &filePath, const BytecodeProgram &program)
    {
        std::ofstream out(filePath, std::ios::binary);
        if (!out)
        {
            throw std::runtime_error("BytecodeWriter: unable to open file");
        }
        Write(out, program);
    }

    void BytecodeWriter::Write(std::ostream &out, const BytecodeProgram &program)
    {
        WriteU32(out, kMagic);
        WriteU32(out, kVersion);

        WriteU32(out, static_cast<std::uint32_t>(program.constants.size()));
        for (const auto &constant : program.constants)
        {
            const auto type = constant.type();
            WriteU32(out, static_cast<std::uint32_t>(type));
            switch (type)
            {
            case cora::compiler::runtime::value_t::null:
                break;
            case cora::compiler::runtime::value_t::boolean:
                WriteI32(out, constant.as_boolean() ? 1 : 0);
                break;
            case cora::compiler::runtime::value_t::integer:
                WriteI64(out, static_cast<std::int64_t>(constant.as_integer()));
                break;
            case cora::compiler::runtime::value_t::floating:
                WriteDouble(out, constant.as_floating());
                break;
            case cora::compiler::runtime::value_t::string:
                WriteString(out, constant.as_string());
                break;
            default:
                throw std::runtime_error("BytecodeWriter: unsupported constant type");
            }
        }

        WriteU32(out, static_cast<std::uint32_t>(program.names.size()));
        for (const auto &name : program.names)
        {
            WriteString(out, name);
        }

        WriteU32(out, static_cast<std::uint32_t>(program.code.size()));
        for (const auto &inst : program.code)
        {
            WriteU32(out, static_cast<std::uint32_t>(inst.op));
            WriteI32(out, inst.a);
            WriteI32(out, inst.b);
            WriteI32(out, inst.c);
        }
    }

} // namespace cora::vmachine
