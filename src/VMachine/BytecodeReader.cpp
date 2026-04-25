#include "BytecodeReader.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace cora::vmachine
{
    namespace
    {
        std::uint32_t ReadU32(const BytecodeReader::RawBytecode &bytes, std::size_t offset)
        {
            return static_cast<std::uint32_t>(bytes[offset]) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
                   (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
        }

        std::int64_t ReadI64(const BytecodeReader::RawBytecode &bytes, std::size_t offset)
        {
            std::int64_t value = 0;
            std::memcpy(&value, bytes.data() + offset, sizeof(value));
            return value;
        }

        std::string ReadString(const BytecodeReader::RawBytecode &bytes, std::size_t &offset)
        {
            const std::uint32_t length = ReadU32(bytes, offset);
            offset += 4U;
            if (offset + length > bytes.size())
            {
                throw std::runtime_error("BytecodeReader: malformed string payload");
            }

            std::string value(reinterpret_cast<const char *>(bytes.data() + offset), length);
            offset += length;
            return value;
        }
    } // namespace

    void BytecodeReader::Read(const std::string &file)
    {
        std::ifstream input(file, std::ios::binary | std::ios::ate);
        if (!input)
        {
            throw std::runtime_error("BytecodeReader: failed to open input file: " + file);
        }

        const std::streamsize size = input.tellg();
        if (size < 0)
        {
            throw std::runtime_error("BytecodeReader: failed to determine file size");
        }

        input.seekg(0, std::ios::beg);

        RawBytecode bytes(static_cast<std::size_t>(size));
        if (!bytes.empty())
        {
            input.read(reinterpret_cast<char *>(bytes.data()), size);
        }

        Read(bytes);
    }

    void BytecodeReader::Read(const RawBytecode &bytecode)
    {
        m_bytecode = bytecode;
        m_program = {};

        if (bytecode.size() < 16U)
        {
            throw std::runtime_error("BytecodeReader: bytecode too small");
        }

        std::size_t offset = 0U;
        const std::uint32_t magic = ReadU32(bytecode, offset);
        offset += 4U;
        if (magic != kMagic)
        {
            throw std::runtime_error("BytecodeReader: invalid bytecode magic");
        }

        const std::uint32_t version = ReadU32(bytecode, offset);
        offset += 4U;
        if (version != kVersion)
        {
            throw std::runtime_error("BytecodeReader: unsupported bytecode version");
        }

        const std::uint32_t constantCount = ReadU32(bytecode, offset);
        offset += 4U;
        m_program.constants.reserve(constantCount);
        for (std::uint32_t i = 0; i < constantCount; ++i)
        {
            if (offset + sizeof(std::int64_t) > bytecode.size())
            {
                throw std::runtime_error("BytecodeReader: malformed constant payload");
            }
            m_program.constants.push_back(ReadI64(bytecode, offset));
            offset += sizeof(std::int64_t);
        }

        const std::uint32_t nameCount = ReadU32(bytecode, offset);
        offset += 4U;
        m_program.names.reserve(nameCount);
        for (std::uint32_t i = 0; i < nameCount; ++i)
        {
            m_program.names.push_back(ReadString(bytecode, offset));
        }

        const std::uint32_t instructionCount = ReadU32(bytecode, offset);
        offset += 4U;
        const std::size_t expectedSize = offset + static_cast<std::size_t>(instructionCount) * 9U;
        if (bytecode.size() != expectedSize)
        {
            throw std::runtime_error("BytecodeReader: malformed bytecode payload size");
        }

        m_program.code.reserve(instructionCount);
        for (std::uint32_t index = 0; index < instructionCount; ++index)
        {
            const auto op = static_cast<OpCode>(bytecode[offset]);
            const std::int32_t a = static_cast<std::int32_t>(ReadU32(bytecode, offset + 1U));
            const std::int32_t b = static_cast<std::int32_t>(ReadU32(bytecode, offset + 5U));
            m_program.code.push_back({op, a, b});
            offset += 9U;
        }
    }

    const BytecodeReader::RawBytecode &BytecodeReader::GetRawBytecode() const
    {
        return m_bytecode;
    }

    const BytecodeProgram &BytecodeReader::GetProgram() const
    {
        return m_program;
    }

} // namespace cora::vmachine
