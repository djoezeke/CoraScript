#ifndef CORA_COMMON_SOURCELOCATION_H
#define CORA_COMMON_SOURCELOCATION_H

#include <cstdint>

namespace cora
{

    struct Range
    {
        uint32_t start;
        uint32_t end;
    };

    struct SourceLocation
    {
        uint32_t fileID = 0;
        uint32_t offset = 0;
        uint32_t line = 0;
        uint32_t column = 0;

        bool isValid() const { return line > 0; }
    };

    struct SourceRange
    {
        SourceLocation start;
        SourceLocation end;

        bool isValid() const { return start.isValid() && end.isValid(); }
    };

} // namespace cora

#endif // CORA_COMMON_SOURCELOCATION_H