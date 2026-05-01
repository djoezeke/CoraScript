#include "ASTStmt.hpp"

#include <sstream>

namespace cora::ast
{
    Statement::Statement()
        : Node(NodeType::Statement)
    {
    }

    Statement::Statement(NodeType kind)
        : Node(kind)
    {
    }

    std::string Statement::Repr()
    {
        return GetNodeTypeString();
    }

    Statement::~Statement() = default;

    ExprStmt::~ExprStmt()
    {
        delete expr;
    }

    IfStmt::~IfStmt()
    {
        delete cond;
        delete true_block;
        delete false_block;
    }

    ForStmt::~ForStmt()
    {
        delete init;
        delete condition;
        delete update;
        delete block;
    }

    WhileStmt::~WhileStmt()
    {
        delete condition;
        delete block;
    }

    VarDeclStmt::~VarDeclStmt()
    {
        delete name;
        delete value;
    }

    ReturnStmt::~ReturnStmt()
    {
        delete value;
    }

    ThrowStmt::~ThrowStmt()
    {
        delete value;
    }
} // namespace cora::ast
