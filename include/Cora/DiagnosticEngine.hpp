#pragma once
#include <memory>
#include <unordered_map>
#include <vector>

#include "Diagnostic.hpp"
#include "DiagnosticCodes.hpp"
#include "DiagnosticEmitter.hpp"

namespace cora
{

    class DiagnosticEngine
    {
    public:
        void addEmitter(std::shared_ptr<DiagnosticEmitter> emitter)
        {
            emitters.push_back(emitter);
        };

        void setType(ErrorCode id, Diagnostic::Type type)
        {
            severityMap[id] = type;
        };

        void setWarningsAsErrors(bool enable)
        {
            warningsAsErrors = enable;
        };

        void report(const Diagnostic &diag)
        {
            Diagnostic currentDiag = diag;

            // Apply overrides
            if (severityMap.count(diag.id))
            {
                currentDiag.type = severityMap[diag.id];
            }

            if (currentDiag.type == Diagnostic::Type::Ignored)
            {
                return;
            }

            if (warningsAsErrors && currentDiag.type == Diagnostic::Type::Warning)
            {
                currentDiag.type = Diagnostic::Type::Error;
            }

            if (currentDiag.type == Diagnostic::Type::Error || currentDiag.type == Diagnostic::Type::Fatal)
            {
                errorCount++;
            }
            else if (currentDiag.type == Diagnostic::Type::Warning)
            {
                warningCount++;
            }

            for (auto &emitter : emitters)
            {
                emitter->emit(currentDiag);
            }
        };

        uint32_t getErrorCount() const { return errorCount; };
        uint32_t getWarningCount() const { return warningCount; };

    private:
        std::vector<std::shared_ptr<DiagnosticEmitter>> emitters;
        std::unordered_map<ErrorCode, Diagnostic::Type> severityMap;
        bool warningsAsErrors = false;
        uint32_t errorCount = 0;
        uint32_t warningCount = 0;
    };

} // namespace cora
