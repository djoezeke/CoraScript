#include "BCReader.hpp"

#include <cstring>
#include <sstream>

namespace cora::bc
{
    namespace
    {
        std::uint32_t ReadU32(const BCReader::RawBytecode &bytes, std::size_t offset)
        {
            return static_cast<std::uint32_t>(bytes[offset]) |
                   (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
                   (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
                   (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
        }

        std::int64_t ReadI64(const BCReader::RawBytecode &bytes, std::size_t offset)
        {
            std::int64_t value = 0;
            std::memcpy(&value, bytes.data() + offset, sizeof(value));
            return value;
        }

        std::string ReadString(const BCReader::RawBytecode &bytes, std::size_t &offset)
        {
            const std::uint32_t length = ReadU32(bytes, offset);
            offset += 4U;
            if (offset + length > bytes.size())
            {
                throw std::runtime_error("BCReader: malformed string payload");
            }

            std::string value(reinterpret_cast<const char *>(bytes.data() + offset), length);
            offset += length;
            return value;
        }
    }

    std::vector<BasicBlock *> BCReader::Read(std::istream &in) {
    };

    std::vector<BasicBlock *> BCReader::ReadFile(std::string filename) {
    };

    std::vector<BasicBlock *> BCReader::Read()
    {
    }

    void BCReader::ReadBinaryInstruction() {};
    void BCReader::ReadLoadInstruction() {};
    void BCReader::ReadStoreInstruction() {};
    void BCReader::ReadCallInstruction() {};
    void BCReader::ReadReturnInstruction() {};
    void BCReader::ReadJumpInstruction() {};
    void BCReader::ReadAllocaInstruction() {};

    BCReader::~BCReader() = default;

} // namespace cora::bc
