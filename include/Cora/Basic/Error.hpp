#ifndef CORA_BASIC_ERROR_H
#define CORA_BASIC_ERROR_H

#include <stdexcept>
#include <string>
#include <vector>

namespace cora::error
{
    enum class ErrorKind
    {
        Generic,
        Lexing,
        Parsing,
        Runtime,
    };

    struct DiagnosticContext
    {
        std::string fileName;
        std::string moduleName;
        std::string namespaceName;
        std::string className;
        std::string functionName;
        unsigned int line{0};
        unsigned int column{0};
    };

    class Error : public std::runtime_error
    {
    public:
        Error(ErrorKind kind, std::string message, DiagnosticContext context = {});

        ErrorKind Kind() const noexcept;
        const DiagnosticContext &Context() const noexcept;
        const std::vector<DiagnosticContext> &Traceback() const noexcept;

        void AddTraceFrame(DiagnosticContext context);
        std::string Format() const;

        ~Error() override;

    private:
        static std::string KindName(ErrorKind kind);
        static std::string JoinContext(const DiagnosticContext &context);

    private:
        ErrorKind m_Kind;
        DiagnosticContext m_Context;
        std::vector<DiagnosticContext> m_Traceback;
    };

    class LexingError final : public Error
    {
    public:
        explicit LexingError(std::string message, DiagnosticContext context = {});
    };

    class ParsingError final : public Error
    {
    public:
        explicit ParsingError(std::string message, DiagnosticContext context = {});
    };

    class RuntimeError final : public Error
    {
    public:
        explicit RuntimeError(std::string message, DiagnosticContext context = {});
    };

} // namespace cora::error

#endif // CORA_BASIC_ERROR_H
