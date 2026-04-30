#include "BytecodeReader.hpp"

#include <fstream>
#include <stdexcept>

namespace cora::vmachine
{
    namespace
    {
        constexpr std::uint32_t kMagic = 0x434F5241U; // CORA
        constexpr std::uint32_t kVersion = 1U;

        std::uint32_t ReadU32(std::istream &in)
        {
            std::uint32_t value = 0;
            in.read(reinterpret_cast<char *>(&value), sizeof(value));
            return value;
        }

        std::int32_t ReadI32(std::istream &in)
        {
            std::int32_t value = 0;
            in.read(reinterpret_cast<char *>(&value), sizeof(value));
            return value;
        }

        std::int64_t ReadI64(std::istream &in)
        {
            std::int64_t value = 0;
            in.read(reinterpret_cast<char *>(&value), sizeof(value));
            return value;
        }

        double ReadDouble(std::istream &in)
        {
            double value = 0.0;
            in.read(reinterpret_cast<char *>(&value), sizeof(value));
            return value;
        }

        std::string ReadString(std::istream &in)
        {
            const std::uint32_t length = ReadU32(in);
            std::string value(length, '\0');
            if (length > 0)
            {
                in.read(&value[0], static_cast<std::streamsize>(length));
            }
            return value;
        }
    }

    void BytecodeReader::Read(const std::string &filePath)
    {
        std::ifstream in(filePath, std::ios::binary);
        if (!in)
        {
            throw std::runtime_error("BytecodeReader: unable to open file");
        }
        Read(in);
    }

    void BytecodeReader::Read(std::istream &in)
    {
        m_program = BytecodeProgram{};

        const std::uint32_t magic = ReadU32(in);
        const std::uint32_t version = ReadU32(in);
        if (magic != kMagic || version != kVersion)
        {
            throw std::runtime_error("BytecodeReader: invalid bytecode header");
        }

        const std::uint32_t constantCount = ReadU32(in);
        m_program.constants.reserve(constantCount);
        for (std::uint32_t i = 0; i < constantCount; ++i)
        {
            const auto type = static_cast<cora::compiler::runtime::value_t>(ReadU32(in));
            using Value = cora::compiler::runtime::value;
            switch (type)
            {
            case cora::compiler::runtime::value_t::null:
                m_program.constants.emplace_back(nullptr);
                break;
            case cora::compiler::runtime::value_t::boolean:
                m_program.constants.emplace_back(ReadI32(in) != 0);
                break;
            case cora::compiler::runtime::value_t::integer:
                m_program.constants.emplace_back(static_cast<Value::integer_type>(ReadI64(in)));
                break;
            case cora::compiler::runtime::value_t::floating:
                m_program.constants.emplace_back(ReadDouble(in));
                break;
            case cora::compiler::runtime::value_t::string:
                m_program.constants.emplace_back(ReadString(in));
                break;
            default:
                throw std::runtime_error("BytecodeReader: unsupported constant type");
            }
        }

        const std::uint32_t nameCount = ReadU32(in);
        m_program.names.reserve(nameCount);
        for (std::uint32_t i = 0; i < nameCount; ++i)
        {
            m_program.names.push_back(ReadString(in));
        }

        const std::uint32_t codeCount = ReadU32(in);
        m_program.code.reserve(codeCount);
        for (std::uint32_t i = 0; i < codeCount; ++i)
        {
            Instruction inst;
            inst.op = static_cast<OpCode>(ReadU32(in));
            inst.a = ReadI32(in);
            inst.b = ReadI32(in);
            inst.c = ReadI32(in);
            m_program.code.push_back(inst);
        }
    }

    const BytecodeProgram &BytecodeReader::GetProgram() const
    {
        return m_program;
    }

} // namespace cora::vmachine
