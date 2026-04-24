#include "Cora/Basic/Error.hpp"

#include <sstream>

namespace cora::compiler
{
    namespace error
    {
        namespace
        {
            constexpr const char *kColorReset = "\033[0m";
            constexpr const char *kColorRed = "\033[1;31m";
            constexpr const char *kColorCyan = "\033[1;36m";
            constexpr const char *kColorYellow = "\033[1;33m";
            constexpr const char *kColorDim = "\033[2m";
        }

        Error::Error(ErrorKind kind, std::string message, DiagnosticContext context)
            : std::runtime_error(std::move(message)), m_Kind(kind), m_Context(std::move(context)), m_Traceback() {}

        ErrorKind Error::Kind() const noexcept
        {
            return m_Kind;
        }

        const DiagnosticContext &Error::Context() const noexcept
        {
            return m_Context;
        }

        const std::vector<DiagnosticContext> &Error::Traceback() const noexcept
        {
            return m_Traceback;
        }

        void Error::AddTraceFrame(DiagnosticContext context)
        {
            m_Traceback.push_back(std::move(context));
        }

        std::string Error::KindName(ErrorKind kind)
        {
            switch (kind)
            {
            case ErrorKind::Lexing:
                return "LexingError";
            case ErrorKind::Parsing:
                return "ParsingError";
            case ErrorKind::Runtime:
                return "RuntimeError";
            default:
                return "Error";
            }
        }

        std::string Error::JoinContext(const DiagnosticContext &context)
        {
            std::ostringstream out;
            bool hasAny = false;

            auto appendPart = [&](const char *label, const std::string &value)
            {
                if (value.empty())
                {
                    return;
                }
                if (hasAny)
                {
                    out << ", ";
                }
                out << label << "=" << value;
                hasAny = true;
            };

            appendPart("module", context.moduleName);
            appendPart("namespace", context.namespaceName);
            appendPart("class", context.className);
            appendPart("function", context.functionName);

            if (context.line != 0 || context.column != 0)
            {
                if (hasAny)
                {
                    out << ", ";
                }
                out << "line=" << context.line << ", column=" << context.column;
                hasAny = true;
            }

            return out.str();
        }

        std::string Error::Format() const
        {
            std::ostringstream out;
            const std::string kindName = KindName(m_Kind);

            out << kColorRed << kindName << kColorReset << ": " << what() << '\n';

            const std::string fileLabel = m_Context.fileName.empty() ? "<unknown>" : m_Context.fileName;
            out << "  " << kColorCyan << "-->" << kColorReset << " " << fileLabel << ":" << m_Context.line << ":" << m_Context.column << '\n';

            const std::string contextLine = JoinContext(m_Context);
            if (!contextLine.empty())
            {
                out << "  " << kColorDim << contextLine << kColorReset << '\n';
            }

            if (!m_Traceback.empty())
            {
                out << kColorYellow << "Traceback (most recent call last):" << kColorReset << '\n';
                for (auto it = m_Traceback.rbegin(); it != m_Traceback.rend(); ++it)
                {
                    const DiagnosticContext &frame = *it;
                    const std::string frameFile = frame.fileName.empty() ? "<unknown>" : frame.fileName;
                    out << "  File \"" << frameFile << "\", line " << frame.line << ", col " << frame.column;

                    if (!frame.functionName.empty())
                    {
                        out << ", in " << frame.functionName;
                    }
                    else if (!frame.className.empty())
                    {
                        out << ", in class " << frame.className;
                    }
                    else if (!frame.namespaceName.empty())
                    {
                        out << ", in namespace " << frame.namespaceName;
                    }

                    out << '\n';
                }
            }

            return out.str();
        }

        Error::~Error() = default;

        LexingError::LexingError(std::string message, DiagnosticContext context)
            : Error(ErrorKind::Lexing, std::move(message), std::move(context)) {}

        ParsingError::ParsingError(std::string message, DiagnosticContext context)
            : Error(ErrorKind::Parsing, std::move(message), std::move(context)) {}

        RuntimeError::RuntimeError(std::string message, DiagnosticContext context)
            : Error(ErrorKind::Runtime, std::move(message), std::move(context)) {}

    } // namespace error

} // namespace cora::compiler
