#ifndef CORA_COMMON_DIAGNOSTICSEMITTER_H
#define CORA_COMMON_DIAGNOSTICSEMITTER_H

#include <iomanip>
#include <iostream>
#include <memory>
#include <vector>

#include "Diagnostic.hpp"
#include "SourceLocation.hpp"
#include "SourceManager.hpp"

namespace cora
{

    class DiagnosticEmitter
    {
    public:
        const SourceManager &sourceManager;

    public:
        DiagnosticEmitter(const SourceManager &sourceManager)
            : sourceManager(sourceManager)
        {
            start();
        };

        virtual ~DiagnosticEmitter()
        {
            end();
        };

        virtual void emit(const Diagnostic &diag) = 0;

        virtual void start() {};
        virtual void end() {};

        std::string getIDString(ErrorCode id)
        {
            return "E" + std::to_string(static_cast<int>(id));
        };

        std::string getTypeString(Diagnostic::Type type)
        {
            switch (type)
            {
            case Diagnostic::Type::Fatal:
                return "fatal";
            case Diagnostic::Type::Error:
                return "error";
            case Diagnostic::Type::Warning:
                return "warning";
                break;
            case Diagnostic::Type::Note:
                return "note";
                break;
            case Diagnostic::Type::Remark:
                return "remark";
                break;
            default:
                return "info";
                break;
            }
        };
    };

    class ConsoleEmitter : public DiagnosticEmitter
    {
    public:
        ConsoleEmitter(const SourceManager &sourceManager)
            : DiagnosticEmitter(sourceManager) {};

        virtual void emit(const Diagnostic &diag) override
        {
            printType(diag.type);
            std::cerr << BOLD << ": " << diag.message << RESET << "\n";

            if (diag.loc.isValid())
            {
                printLocation(diag.loc, diag.type, diag.id);
                printSnippet(diag);
            }
            std::cerr << std::endl;
        };

    private:
        void printType(Diagnostic::Type type)
        {
            switch (type)
            {
            case Diagnostic::Type::Fatal:
            case Diagnostic::Type::Error:
                std::cerr << RED << BOLD << "error";
                break;
            case Diagnostic::Type::Warning:
                std::cerr << YELLOW << BOLD << "warning";
                break;
            case Diagnostic::Type::Note:
                std::cerr << GREEN << BOLD << "note";
                break;
            case Diagnostic::Type::Remark:
                std::cerr << BLUE << BOLD << "remark";
                break;
            default:
                std::cerr << BOLD << "info";
                break;
            }
        };

        void printLocation(const SourceLocation &loc, const Diagnostic::Type type, const ErrorCode id)
        {
            const SourceFile *file = sourceManager.getFile(loc.fileID);
            std::cerr << getTypeString(type) << " [" << getIDString(id) << " ] --> " << (file ? file->path : "unknown") << ":" << loc.line << ":" << loc.column << "\n";
        };

        void printSnippet(const Diagnostic &diag)
        {
            const SourceLocation &loc = diag.loc;
            std::string_view line = sourceManager.getLineContent(loc.fileID, loc.line);
            if (line.empty())
                return;

            std::string lineNum = std::to_string(loc.line);
            size_t gutterWidth = lineNum.size();
            std::string padding(gutterWidth, ' ');

            std::cerr << BLUE << BOLD << padding << " |" << RESET << "\n";
            std::cerr << BLUE << BOLD << lineNum << " | " << RESET << line << "\n";
            std::cerr << BLUE << BOLD << padding << " | " << RESET;

            // Print caret
            for (uint32_t i = 1; i < loc.column; ++i)
            {
                std::cerr << " ";
            }

            const char *color = RED;
            if (diag.type == Diagnostic::Type::Warning)
                color = YELLOW;
            else if (diag.type == Diagnostic::Type::Note)
                color = GREEN;

            std::cerr << color << BOLD << "^" << RESET;

            // Print labels on the same line
            bool firstLabel = true;
            for (const auto &label : diag.labels)
            {
                if (label.range.start.line == loc.line)
                {
                    if (firstLabel)
                    {
                        std::cerr << color << BOLD << " " << label.message << RESET;
                        firstLabel = false;
                    }
                    else
                    {
                        // TODO: handle multiple labels on same line
                    }
                }
            }

            // If no labels but we want to emphasize the primary message again near the caret
            if (firstLabel && !diag.labels.empty())
            {
                // Maybe just leave it
            }

            std::cerr << "\n";
            std::cerr << BLUE << BOLD << padding << " |" << RESET << "\n";
        };

        // Simple ANSI color helpers
        const char *RESET = "\033[0m";
        const char *RED = "\033[31m";
        const char *GREEN = "\033[32m";
        const char *YELLOW = "\033[33m";
        const char *BLUE = "\033[34m";
        const char *BOLD = "\033[1m";
    };

    class JsonEmitter : public DiagnosticEmitter
    {
    private:
        bool first = true;

    public:
        JsonEmitter(const SourceManager &sourceManager)
            : DiagnosticEmitter(sourceManager) {};

        virtual void emit(const Diagnostic &diag) override
        {
            if (!first)
                std::cout << ",\n";
            first = false;

            std::cout << "  {\n";
            std::cout << "    \"id\": \"" << getIDString(diag.id) << "\",\n";
            std::cout << "    \"type\": \"" << getTypeString(diag.type) << "\",\n";
            std::cout << "    \"message\": \"" << diag.message << "\",\n";
            if (diag.loc.isValid())
            {
                const auto *file = sourceManager.getFile(diag.loc.fileID);
                std::cout << "    \"location\": {\n";
                std::cout << "      \"file\": \"" << (file ? file->path : "unknown") << "\",\n";
                std::cout << "      \"line\": " << diag.loc.line << ",\n";
                std::cout << "      \"column\": " << diag.loc.column << "\n";
                std::cout << "    }\n";
            }
            std::cout << "  }";
        };

        void start() override { std::cout << "[\n"; }
        void end() override { std::cout << "\n]\n"; }
    };

} // namespace cora

#endif // CORA_COMMON_DIAGNOSTICSEMITTER_H