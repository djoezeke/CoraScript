#ifndef CORA_COMMON_DIAGNOSTICSCODES_H
#define CORA_COMMON_DIAGNOSTICSCODES_H

namespace cora
{

    enum class ErrorCode
    {
        E0000, // Success

        E0001, // Invalid character in source code
        E0002, // Invalid identifier
        E0003, // Unterminated string literal
        E0004, // Unexpected token
        E0005, // Expected ';' after statement
        E0006, // Type mismatch in assignment
        E0007, // Undefined variable
        E0008, // Redeclared variable

        E9999, // Generic error
    };

} // namespace cora

#endif // CORA_COMMON_DIAGNOSTICSCODES_H