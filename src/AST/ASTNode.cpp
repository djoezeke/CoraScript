#include "ASTNode.hpp"
#include "ASTStmt.hpp"

#include <sstream>
#include <unordered_map>

namespace cora::ast
{
    Node::Node(NodeType nodetype)
        : node_type(nodetype), m_Range() {}

    Node::Node(NodeType nodetype, SourceRange sourcerange)
        : node_type(nodetype), m_Range(std::move(sourcerange)) {}

    Node::Node(NodeType nodetype, SourceLocation location)
        : node_type(nodetype), m_Range({location, location}) {}

    Node::Node(NodeType nodetype, SourceLocation start, SourceLocation end)
        : node_type(nodetype), m_Range({start, end}) {}

    NodeType Node::GetNodeType()
    {
        return node_type;
    }

    std::string Node::GetNodeTypeString() const
    {
        static const std::unordered_map<NodeType, std::string> names = {
            {NodeType::Statement, "Statement"},
            {NodeType::Expression, "Expression"},
            {NodeType::Assignment, "Assignment"},
            {NodeType::Null, "Bool"},
            {NodeType::Byte, "Byte"},
            {NodeType::Float, "Float"},
            {NodeType::Array, "Array"},
            {NodeType::String, "String"},
            {NodeType::Integer, "Integer"},
        };

        auto it = names.find(node_type);
        if (it != names.end())
        {
            return it->second;
        }

        return "NodeType(" + std::to_string(static_cast<int>(node_type)) + ")";
    };

    SourceLocation Node::GetStartPosition() const noexcept
    {
        return m_Range.start;
    }

    SourceLocation Node::GetEndPosition() const noexcept
    {
        return m_Range.end;
    }

    SourceRange Node::GetSourceRange() const
    {
        return m_Range;
    }

    void Node::SetNodeType(NodeType nodetype) noexcept
    {
        node_type = nodetype;
    }

    void Node::SetStartPosition(SourceLocation start) noexcept
    {
        m_Range.start = start;
    }

    void Node::SetEndPosition(SourceLocation end) noexcept
    {
        m_Range.end = end;
    }

    void Node::SetSourceRange(SourceRange range) noexcept
    {
        m_Range = range;
    }

    std::vector<std::string> Node::Parts(int depth)
    {
        std::vector<std::string> result;
        std::string spacing;

        // result.push_back();
        result.push_back("\b├── ");
        result.push_back("\b└── ");

        return result;
    };

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

} // namespace cora::ast
