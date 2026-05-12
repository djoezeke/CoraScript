#ifndef CORA_COMMON_DIAGNOSTICS_H
#define CORA_COMMON_DIAGNOSTICS_H

#include <string>
#include <vector>

#include "SourceLocation.hpp"

namespace cora
{

    enum class ErrorCode;

    struct Diagnostic
    {
        enum class Type
        {
            Ignored,
            Note,
            Remark,
            Warning,
            Error,
            Fatal
        };

        struct Label
        {
            SourceRange range;
            std::string message;
        };

        struct Hint
        {
            SourceRange range;
            std::string replacement;
        };

        ErrorCode id;
        Diagnostic::Type type;
        std::string message;
        SourceLocation loc;
        std::vector<Diagnostic::Label> labels;
        std::vector<Diagnostic::Hint> hints;

        Diagnostic() = default;

        Diagnostic(ErrorCode id, Diagnostic::Type type, const std::string &message, SourceLocation loc = {})
            : id(id), type(type), message(message), loc(loc) {}

        Diagnostic &setID(ErrorCode newID)
        {
            id = newID;
            return *this;
        };

        Diagnostic &setType(Diagnostic::Type newType)
        {
            type = newType;
            return *this;
        };

        Diagnostic &setMessage(const std::string &newMessage)
        {
            message = newMessage;
            return *this;
        };

        Diagnostic &addLabel(const SourceRange &range, const std::string &message)
        {
            labels.push_back({range, message});
            return *this;
        };

        Diagnostic &addHint(const SourceRange &range, const std::string &replacement)
        {
            hints.push_back({range, replacement});
            return *this;
        };
    };

    struct ErrorDiagnostic : public Diagnostic
    {
        ErrorDiagnostic(ErrorCode id, const std::string &message, SourceLocation loc = {})
            : Diagnostic(id, Diagnostic::Type::Error, message, loc) {}
    };

    struct WarningDiagnostic : public Diagnostic
    {
        WarningDiagnostic(ErrorCode id, const std::string &message, SourceLocation loc = {})
            : Diagnostic(id, Diagnostic::Type::Warning, message, loc) {}
    };

    struct NoteDiagnostic : public Diagnostic
    {
        NoteDiagnostic(ErrorCode id, const std::string &message, SourceLocation loc = {})
            : Diagnostic(id, Diagnostic::Type::Note, message, loc) {}
    };

    struct RemarkDiagnostic : public Diagnostic
    {
        RemarkDiagnostic(ErrorCode id, const std::string &message, SourceLocation loc = {})
            : Diagnostic(id, Diagnostic::Type::Remark, message, loc) {}
    };

    struct FatalDiagnostic : public Diagnostic
    {
        FatalDiagnostic(ErrorCode id, const std::string &message, SourceLocation loc = {})
            : Diagnostic(id, Diagnostic::Type::Fatal, message, loc) {}
    };

} // namespace cora

#endif // CORA_COMMON_DIAGNOSTICS_H
