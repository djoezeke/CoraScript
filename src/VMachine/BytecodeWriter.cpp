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

} // namespace cora::vmachine
