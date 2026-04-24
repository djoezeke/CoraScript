#ifndef CORA_CORE_INTERNAL_JITPIPELINE_HPP
#define CORA_CORE_INTERNAL_JITPIPELINE_HPP

#include "JITEngine.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace cora::embed::internal
{
    struct BytecodeProgram;

    struct TraceInstruction
    {
        std::uint8_t op{0};
        std::int32_t a{0};
        std::int32_t b{0};
    };

    struct CompiledTrace
    {
        std::int32_t loopHeader{0};
        std::int32_t backEdge{0};
        std::vector<TraceInstruction> ir;
        std::uint64_t hitCount{0};
        std::uint64_t sideExits{0};
        std::uint64_t executions{0};
        bool compiled{false};
    };

    class JitPipeline
    {
    public:
        explicit JitPipeline(JitConfig config);

        bool Enabled() const;
        bool TracingEnabled() const;
        std::uint32_t HotLoopThreshold() const;

        bool ShouldCompile(std::uint64_t hitCount) const;
        std::string BackendName() const;

        void ResetExecutionState();
        bool OnLoopBackEdge(const BytecodeProgram &program, std::int32_t loopHeader, std::int32_t backEdgeIp);
        bool HasCompiledTrace(std::int32_t loopHeader) const;
        void RecordCompiledTraceExecution(std::int32_t loopHeader);

        std::size_t TraceCount() const;
        std::uint64_t TotalSideExits() const;
        std::uint64_t TotalCompiledExecutions() const;

    private:
        bool CompileTrace(const BytecodeProgram &program, CompiledTrace &trace);

    private:
        JitConfig m_Config;
        std::unordered_map<std::int32_t, CompiledTrace> m_Traces;
    };
}

#endif
