#include "ASTExpr.hpp"
#include "ASTStmt.hpp"

#include <sstream>

namespace cora::ast
{
    std::string Expression::Repr()
    {
        return GetNodeTypeString();
    }

    Expression::~Expression() = default;

    std::string NullExpr::Repr()
    {
        return "null";
    }

    std::string BoolExpr::Repr()
    {
        return value ? "true" : "false";
    }

    std::string ByteExpr::Repr()
    {
        return std::string(1, value);
    }

    std::string FloatExpr::Repr()
    {
        std::ostringstream out;
        out << value;
        return out.str();
    }

    ArrayExpr::~ArrayExpr()
    {
        for (Expression *statement : value)
        {
            delete statement;
        }
    }

    std::string IdentifierExpr::Repr()
    {
        return name;
    }

    ArrayIdExpr::~ArrayIdExpr()
    {
        for (Statement *statement : value)
        {
            delete statement;
        }
    }

    ParamExpr::~ParamExpr()
    {
        delete name;
        delete type;
    }

    std::string UnaryExpr::Repr()
    {
        return "unary";
    }

    UnaryExpr::~UnaryExpr()
    {
        delete expr;
    }

    std::string BinaryExpr::Repr()
    {
        return "binary";
    }

    BinaryExpr::~BinaryExpr()
    {
        delete left;
        delete right;
    }

    std::string FuncCallExpr::Repr()
    {
        return name != nullptr ? name->name : "<call>";
    }

    FuncCallExpr::~FuncCallExpr()
    {
        delete name;
        for (Statement *statement : args)
        {
            delete statement;
        }
    }

    StructLiteralExpr::~StructLiteralExpr()
    {
        for (auto &[fieldName, fieldValue] : fields)
        {
            delete fieldName;
            delete fieldValue;
        }
    }

    TernaryExpr::TernaryExpr(Expression *condExpr, Expression *thenExpr, Expression *elseExpr)
        : Expression(NodeType::TernaryExpr), cond(condExpr), then(thenExpr), _else(elseExpr)
    {
    }

    TernaryExpr::~TernaryExpr()
    {
        delete cond;
        delete then;
        delete _else;
    }
} // namespace cora::ast
