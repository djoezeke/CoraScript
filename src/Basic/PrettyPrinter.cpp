#include "Cora/Common/PrettyPrinter.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace cora::common
{
    namespace
    {
        // ANSI color codes for terminal output
        const std::string RED = std::string("\033[31m");
        const std::string YELLOW = std::string("\033[33m");
        const std::string BLUE = std::string("\033[34m");
        const std::string BOLD = std::string("\033[1m");
        const std::string RESET = std::string("\033[0m");

        bool IsTerminalSupportsColor()
        {
            // Simple check - in a real implementation, you'd check if stdout is a TTY
            // For now, assume color support
            return true;
        }

        std::string Colorize(const std::string &text, const std::string &color)
        {
            if (!IsTerminalSupportsColor())
            {
                return text;
            }
            return std::string(color) + text + RESET;
        }
    }

    std::string SeverityToString(DiagnosticSeverity severity)
    {
        switch (severity)
        {
        case DiagnosticSeverity::Note:
            return "note";
        case DiagnosticSeverity::Warning:
            return "warning";
        case DiagnosticSeverity::Error:
            return "error";
        }

        return "error";
    }

    std::string FormatDiagnostic(const Diagnostic &diagnostic, const SourceContext &context)
    {
        std::ostringstream output;
        const std::size_t lineNumber = diagnostic.location.line == 0 ? 1 : diagnostic.location.line;
        const std::size_t columnNumber = diagnostic.location.column == 0 ? 1 : diagnostic.location.column;

        // Format: filename:line:column: severity: message
        output << context.FileName() << ':' << lineNumber << ':' << columnNumber << ": ";

        const std::string severityStr = SeverityToString(diagnostic.severity);
        if (diagnostic.severity == DiagnosticSeverity::Error)
        {
            output << Colorize(severityStr + ": ", RED + BOLD);
        }
        else if (diagnostic.severity == DiagnosticSeverity::Warning)
        {
            output << Colorize(severityStr + ": ", YELLOW + BOLD);
        }
        else
        {
            output << Colorize(severityStr + ": ", BLUE + BOLD);
        }

        output << diagnostic.message << '\n';

        const std::string_view lineText = context.Line(lineNumber);
        if (!lineText.empty())
        {
            // Print the line with line number
            output << " " << lineNumber << " | " << lineText << '\n';

            // Print the caret/underline
            output << " " << std::string(std::to_string(lineNumber).size(), ' ') << " | ";
            const std::size_t caretPadding = columnNumber > 0 ? columnNumber - 1 : 0;
            output << std::string(caretPadding, ' ');

            std::string underline;
            if (diagnostic.severity == DiagnosticSeverity::Error)
            {
                underline = Colorize("^", RED + BOLD);
            }
            else if (diagnostic.severity == DiagnosticSeverity::Warning)
            {
                underline = Colorize("^", YELLOW + BOLD);
            }
            else
            {
                underline = Colorize("^", BLUE + BOLD);
            }

            const std::size_t underlineLength = diagnostic.range.has_value() && diagnostic.range->end.column > diagnostic.range->start.column
                                                    ? diagnostic.range->end.column - diagnostic.range->start.column
                                                    : 1;
            for (std::size_t i = 0; i < underlineLength; ++i)
            {
                output << underline;
            }
            output << '\n';
        }

        // Print notes
        for (const auto &note : diagnostic.notes)
        {
            const std::size_t noteLine = note.location.line == 0 ? lineNumber : note.location.line;
            const std::size_t noteColumn = note.location.column == 0 ? 1 : note.location.column;

            output << context.FileName() << ':' << noteLine << ':' << noteColumn << ": ";
            output << Colorize("note: ", BLUE + BOLD) << note.message << '\n';

            const std::string_view noteText = context.Line(noteLine);
            if (!noteText.empty() && noteLine != lineNumber)
            {
                output << " " << noteLine << " | " << noteText << '\n';
                output << " " << std::string(std::to_string(noteLine).size(), ' ') << " | ";
                const std::size_t noteCaretPadding = noteColumn > 0 ? noteColumn - 1 : 0;
                output << std::string(noteCaretPadding, ' ');
                output << Colorize("^", BLUE + BOLD) << '\n';
            }
        }

        return output.str();
    }
}
