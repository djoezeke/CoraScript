#include "ASTNode.hpp"
#include "ASTStmt.hpp"

#include <sstream>

namespace cora::compiler
{
    namespace ast
    {
        Node::Node(NodeType nodetype)
            : m_Type(nodetype), m_Range(SourceRange()) {}

        Node::Node(NodeType nodetype, SourceRange sourcerange)
            : m_Type(nodetype), m_Range(std::move(sourcerange)) {}

        Node::Node(NodeType nodetype, SourceLocation location)
            : m_Type(nodetype), m_Range(SourceRange(location)) {}

        Node::Node(NodeType nodetype, SourceLocation start, SourceLocation end)
            : m_Type(nodetype), m_Range(SourceRange(start, end)) {}

        NodeType Node::GetNodeType()
        {
            return m_Type;
        }

        SourceLocation Node::GetStartPosition() const noexcept
        {
            return m_Range.GetStart();
        }

        SourceLocation Node::GetEndPosition() const noexcept
        {
            return m_Range.GetEnd();
        }

        SourceRange Node::GetSourceRange() const
        {
            return m_Range;
        }

        void Node::SetNodeType(NodeType nodetype) noexcept
        {
            m_Type = nodetype;
        }

        void Node::SetStartPosition(SourceLocation start) noexcept
        {
            m_Range.SetStart(start);
        }

        void Node::SetEndPosition(SourceLocation end) noexcept
        {
            m_Range.SetEnd(end);
        }

        void Node::SetSourceRange(SourceRange range) noexcept
        {
            m_Range = range;
        }

        Program::Program()
            : Node(NodeType::Program) {}

        void Program::PushBack(Module *module)
        {
            m_Modules.push_back(module);
        }

        void Program::EmplaceBack(Module *module)
        {
            m_Modules.emplace_back(module);
        }

        const ModuleList &Program::GetModules() const
        {
            return m_Modules;
        }

        std::string Program::Repr()
        {
            std::ostringstream out;
            out << "Program(modules=" << m_Modules.size() << ")";
            return out.str();
        }

        Program::~Program()
        {
            for (Module *module : m_Modules)
            {
                delete module;
            }
        }

        Module::Module()
            : Node(NodeType::Module) {}

        void Module::PushBack(Statement *statement)
        {
            m_Statements.push_back(statement);
        }

        void Module::EmplaceBack(Statement *statement)
        {
            m_Statements.emplace_back(statement);
        }

        const StatementList &Module::GetStatements() const
        {
            return m_Statements;
        }

        std::string Module::Repr()
        {
            std::ostringstream out;
            out << "Module(statements=" << m_Statements.size() << ")";
            return out.str();
        }

        Module::~Module()
        {
            for (Statement *statement : m_Statements)
            {
                delete statement;
            }
        }

        std::ostream &operator<<(std::ostream &ostream, const Node *node)
        {
            if (node == nullptr)
            {
                return ostream << "<null-node>";
            }

            return ostream << const_cast<Node *>(node)->Repr();
        }

    } // namespace ast

} // namespace cora::compiler
