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

} // namespace cora::vmachine
