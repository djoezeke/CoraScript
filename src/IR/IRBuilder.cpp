#include "IRBuilder.hpp"

#include "../Parser/Token.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace cora::ir
{

    IRBuilder::IRBuilder() = default;

    BasicBlock *IRBuilder::Build(const std::deque<cora::ast::Statement *> &program)
    {
        Reset();

        auto entry = std::make_unique<BasicBlock>();
        entry->name = "entry";
        m_entry = entry.get();
        m_currentBlock = m_entry;
        m_blocks.push_back(m_entry);
        m_ownedBlocks.push_back(std::move(entry));

        for (ast::Statement *stmt : program)
        {
        }

        return m_entry;
    }

    void IRBuilder::Reset()
    {
        m_ownedBlocks.clear();
        m_blocks.clear();
        m_ownedValues.clear();
        m_variables.clear();
        m_entry = nullptr;
        m_currentBlock = nullptr;
        m_tempIndex = 0;
    }

    const std::vector<BasicBlock *> &IRBuilder::GetBlocks() const
    {
        return m_blocks;
    }

    BasicBlock *IRBuilder::GetEntryBlock() const
    {
        return m_entry;
    }

    Value *IRBuilder::lookupVariable(const std::string &name) const
    {
        const auto found = m_variables.find(name);
        if (found == m_variables.end())
        {
            return nullptr;
        }
        return found->second;
    };

    void IRBuilder::assignVariable(const std::string &name, Value *value)
    {
        m_variables[name] = value;
    };

    std::string IRBuilder::makeTempName(const std::string &prefix)
    {
        std::ostringstream out;
        out << '%' << prefix << m_tempIndex++;
        return out.str();
    };

    IRBuilder::~IRBuilder() = default;

} // namespace cora::ir
