#include "JitPipeline.hpp"

#include "../IRGen/Bytecode.hpp"

#include <numeric>

namespace cora::embed::internal
{
    JitPipeline::JitPipeline(JitConfig config)
        : m_Config(config)
    {
    }

    bool JitPipeline::Enabled() const
    {
        return m_Config.enabled;
    }

    bool JitPipeline::TracingEnabled() const
    {
        return m_Config.tracing;
    }

    std::uint32_t JitPipeline::HotLoopThreshold() const
    {
        return m_Config.hotLoopThreshold;
    }

    bool JitPipeline::ShouldCompile(std::uint64_t hitCount) const
    {
        return Enabled() && hitCount >= HotLoopThreshold();
    }

    std::string JitPipeline::BackendName() const
    {
        if (!Enabled())
        {
            return "interpreter";
        }

        return TracingEnabled() ? "jit-tracing" : "jit-baseline";
    }

    void JitPipeline::ResetExecutionState()
    {
        if (!Enabled())
        {
            return;
        }

        for (auto &entry : m_Traces)
        {
            entry.second.hitCount = 0;
            entry.second.executions = 0;
            entry.second.sideExits = 0;
        }
    }

    bool JitPipeline::OnLoopBackEdge(const BytecodeProgram &program, std::int32_t loopHeader, std::int32_t backEdgeIp)
    {
        if (!Enabled())
        {
            return false;
        }

        CompiledTrace &trace = m_Traces[loopHeader];
        if (trace.ir.empty())
        {
            trace.loopHeader = loopHeader;
            trace.backEdge = backEdgeIp;
        }

        ++trace.hitCount;

        if (!trace.compiled && ShouldCompile(trace.hitCount))
        {
            trace.compiled = CompileTrace(program, trace);
        }

        return trace.compiled;
    }

    bool JitPipeline::HasCompiledTrace(std::int32_t loopHeader) const
    {
        const auto found = m_Traces.find(loopHeader);
        return found != m_Traces.end() && found->second.compiled;
    }

    void JitPipeline::RecordCompiledTraceExecution(std::int32_t loopHeader)
    {
        auto found = m_Traces.find(loopHeader);
        if (found == m_Traces.end() || !found->second.compiled)
        {
            return;
        }

        ++found->second.executions;
    }

    std::size_t JitPipeline::TraceCount() const
    {
        return m_Traces.size();
    }

    std::uint64_t JitPipeline::TotalSideExits() const
    {
        return std::accumulate(
            m_Traces.begin(),
            m_Traces.end(),
            static_cast<std::uint64_t>(0),
            [](std::uint64_t total, const auto &entry)
            {
                return total + entry.second.sideExits;
            });
    }

    std::uint64_t JitPipeline::TotalCompiledExecutions() const
    {
        return std::accumulate(
            m_Traces.begin(),
            m_Traces.end(),
            static_cast<std::uint64_t>(0),
            [](std::uint64_t total, const auto &entry)
            {
                return total + entry.second.executions;
            });
    }

    bool JitPipeline::CompileTrace(const BytecodeProgram &program, CompiledTrace &trace)
    {
        if (!TracingEnabled())
        {
            return true;
        }

        if (trace.loopHeader < 0 || trace.backEdge < trace.loopHeader)
        {
            return false;
        }

        const auto begin = static_cast<std::size_t>(trace.loopHeader);
        const auto end = static_cast<std::size_t>(trace.backEdge);

        if (end >= program.code.size())
        {
            return false;
        }

        trace.ir.clear();
        trace.ir.reserve(end - begin + 1);

        for (std::size_t ip = begin; ip <= end; ++ip)
        {
            const Instruction &ins = program.code[ip];
            if (ins.op == OpCode::JumpIfFalse)
            {
                const bool exitsLoop = ins.a > trace.backEdge || ins.a < trace.loopHeader;
                if (exitsLoop)
                {
                    ++trace.sideExits;
                }
            }

            trace.ir.push_back(TraceInstruction{
                static_cast<std::uint8_t>(ins.op),
                ins.a,
                ins.b,
            });
        }

        return !trace.ir.empty();
    }
}
