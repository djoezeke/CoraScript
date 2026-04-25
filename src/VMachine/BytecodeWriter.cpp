#include "BytecodeWriter.hpp"

#include <fstream>
#include <stdexcept>

namespace cora::vmachine
{
    namespace
    {
        void WriteU32(std::vector<std::uint8_t> &out, std::uint32_t value)
        {
            out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
            out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
            out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
            out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
        }

        void WriteI32(std::vector<std::uint8_t> &out, std::int32_t value)
        {
            WriteU32(out, static_cast<std::uint32_t>(value));
        }

        void WriteI64(std::vector<std::uint8_t> &out, std::int64_t value)
        {
            const auto *bytes = reinterpret_cast<const std::uint8_t *>(&value);
            out.insert(out.end(), bytes, bytes + sizeof(value));
        }

        void WriteString(std::vector<std::uint8_t> &out, const std::string &value)
        {
            WriteU32(out, static_cast<std::uint32_t>(value.size()));
            out.insert(out.end(), value.begin(), value.end());
        }
    }

    BytecodeWriter::RawBytecode BytecodeWriter::Write(const BytecodeProgram &program) const
    {
        RawBytecode bytes;
        bytes.reserve(64U + program.code.size() * 9U);

        WriteU32(bytes, kMagic);
        WriteU32(bytes, kVersion);

        WriteU32(bytes, static_cast<std::uint32_t>(program.constants.size()));
        for (std::int64_t constant : program.constants)
        {
            WriteI64(bytes, constant);
        }

        WriteU32(bytes, static_cast<std::uint32_t>(program.names.size()));
        for (const std::string &name : program.names)
        {
            WriteString(bytes, name);
        }

        WriteU32(bytes, static_cast<std::uint32_t>(program.code.size()));
        for (const Instruction &instruction : program.code)
        {
            bytes.push_back(static_cast<std::uint8_t>(instruction.op));
            WriteI32(bytes, instruction.a);
            WriteI32(bytes, instruction.b);
        }

        return bytes;
    }

    void BytecodeWriter::Write(const std::string &file, const BytecodeProgram &program) const
    {
        Write(file, Write(program));
    }

    void BytecodeWriter::Write(const std::string &file, const RawBytecode &bytecode) const
    {
        std::ofstream output(file, std::ios::binary);
        if (!output)
        {
            throw std::runtime_error("BytecodeWriter: failed to open output file: " + file);
        }

        if (!bytecode.empty())
        {
            output.write(reinterpret_cast<const char *>(bytecode.data()), static_cast<std::streamsize>(bytecode.size()));
        }
    }

} // namespace cora::vmachine
