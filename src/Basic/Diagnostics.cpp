#include "Cora/Common/Diagnostics.hpp"
#include "Cora/Common/Exceptions.hpp"
#include "Cora/Common/PrettyPrinter.hpp"

#include <iostream>

namespace cora::common
{
    Diagnostic DiagnosticReporter::MakeError(std::string message, SourceLocation location, std::optional<SourceRange> range) const
    {
        return Diagnostic{DiagnosticSeverity::Error, std::move(message), location, std::move(range), {}};
    }

    Diagnostic DiagnosticReporter::MakeWarning(std::string message, SourceLocation location, std::optional<SourceRange> range) const
    {
        return Diagnostic{DiagnosticSeverity::Warning, std::move(message), location, std::move(range), {}};
    }

    std::string DiagnosticReporter::Render(const Diagnostic &diagnostic) const
    {
        return FormatDiagnostic(diagnostic, m_Context);
    }

    void DiagnosticReporter::Throw(const Diagnostic &diagnostic) const
    {
        throw DiagnosticException(Render(diagnostic));
    }

    void DiagnosticReporter::Warn(const Diagnostic &diagnostic) const
    {
        std::cerr << Render(diagnostic) << '\n';
    }

    [[noreturn]] void ThrowDiagnostic(const SourceContext &context,
                                      std::string message,
                                      DiagnosticSeverity severity,
                                      SourceLocation location,
                                      std::optional<SourceRange> range)
    {
        DiagnosticReporter reporter(context);
        const Diagnostic diagnostic{severity, std::move(message), location, std::move(range), {}};
        if (severity == DiagnosticSeverity::Warning)
        {
            reporter.Warn(diagnostic);
            throw DiagnosticException(reporter.Render(diagnostic));
        }

        reporter.Throw(diagnostic);
    }
}
