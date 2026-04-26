#include "ASTStmt.hpp"

namespace cora::ast
{
    Statement::Statement()
        : Node(NodeType::Statement) {}

    Statement::Statement(NodeType kind)
        : Node(NodeType::Statement) {}

    std::string Statement::Repr()
    {
        return GetNodeTypeString();
    }

    Statement::~Statement() = default;

    Assignment::Assignment(Expression *target, Expression *value)
        : Statement(NodeType::Assignment), m_Target(target), m_Value(value) {};

    Expression *Assignment::GetTarget() const { return m_Target; };

    Expression *Assignment::GetValue() const { return m_Value; };

    Assignment::~Assignment()
    {
        delete m_Value;
        delete m_Target;
    };

    VarDeclaration::VarDeclaration(std::string name, std::string type, Expression *expr, bool constant)
        : Statement(NodeType::VarDeclaration), m_Name(std::move(name)), m_Type(std::move(type)), m_Expression(expr), m_Constant(constant) {};

    const std::string &VarDeclaration::GetName() const { return m_Name; };

    const std::string &VarDeclaration::GetType() const { return m_Type; };

    Expression *VarDeclaration::GetExpression() const
    {
        return m_Expression;
    };

    bool VarDeclaration::IsConst() const
    {
        return m_Constant;
    }

    VarDeclaration::~VarDeclaration()
    {
        delete m_Expression;
    };

    ExprStmt::ExprStmt(Expression *expr)
        : Statement(NodeType::PassStmt), m_Expr(expr) {}

    Expression *ExprStmt::GetExpression() const
    {
        return m_Expr;
    }

    ExprStmt::~ExprStmt()
    {
        delete m_Expr;
    }

    PrintStmt::PrintStmt()
        : Statement(NodeType::PassStmt) {}

    PrintStmt::~PrintStmt()
    {
        for (Expression *expr : expressions)
        {
            delete expr;
        }
    };

    AssignStmt::AssignStmt(Expression *target, Expression *expr)
        : Statement(NodeType::PassStmt), m_Target(target), m_Value(expr) {}

    Expression *AssignStmt::GetTarget() const
    {
        return m_Target;
    }

    Expression *AssignStmt::GetValue() const
    {
        return m_Value;
    }

    AssignStmt::~AssignStmt()
    {
        delete m_Target;
        delete m_Value;
    }

    VarDeclStmt::VarDeclStmt(std::string name, std::optional<std::string> declaredType, Expression *expr,
                             AccessModifier access, bool constant)
        : Statement(NodeType::NewStmt),
          m_Name(std::move(name)),
          m_DeclaredType(std::move(declaredType)),
          m_Expr(expr),
          m_Access(access),
          m_Constant(constant) {}

    const std::string &VarDeclStmt::GetName() const
    {
        return m_Name;
    }

    const std::optional<std::string> &VarDeclStmt::GetDeclaredType() const
    {
        return m_DeclaredType;
    }

    Expression *VarDeclStmt::GetExpression() const
    {
        return m_Expr;
    }

    AccessModifier VarDeclStmt::GetAccessModifier() const
    {
        return m_Access;
    }

    bool VarDeclStmt::IsConst() const
    {
        return m_Constant;
    }

    VarDeclStmt::~VarDeclStmt()
    {
        delete m_Expr;
    }

    IfStmt::IfStmt()
        : Statement(NodeType::IfStmt), elseBlock(nullptr) {}

    IfStmt::~IfStmt()
    {
        for (auto &branch : branches)
        {
            delete branch.first;
            delete branch.second;
        }
        delete elseBlock;
    }

    DoStmt::DoStmt()
        : Statement(NodeType::DoStmt) {}

    DoStmt::~DoStmt() = default;

    ForStmt::ForStmt()
        : Statement(NodeType::ForStmt) {}

    ForStmt::~ForStmt() = default;

    NewStmt::NewStmt()
        : Statement(NodeType::NewStmt) {}

    NewStmt::~NewStmt() = default;

    PassStmt::PassStmt()
        : Statement(NodeType::PassStmt) {}

    WhileStmt::WhileStmt()
        : Statement(NodeType::WhileStmt), condition(nullptr), block(nullptr) {}

    WhileStmt::WhileStmt(Expression *cond, BlockStmt *blk)
        : Statement(NodeType::WhileStmt), condition(cond), block(blk) {}

    WhileStmt::~WhileStmt()
    {
        delete condition;
        delete block;
    }

    BreakStmt::BreakStmt()
        : Statement(NodeType::BreakStmt) {}

    BlockStmt::BlockStmt()
        : Statement(NodeType::BraceStmt) {}

    BlockStmt::~BlockStmt()
    {
        for (Statement *stmt : statements)
        {
            delete stmt;
        }
    }

    YieldStmt::YieldStmt()
        : Statement(NodeType::YieldStmt) {}

    YieldStmt::~YieldStmt() = default;

    ThrowStmt::ThrowStmt(Expression *value)
        : Statement(NodeType::ThrowStmt), m_Value(value) {}

    Expression *ThrowStmt::GetValue() const
    {
        return m_Value;
    }

    ThrowStmt::~ThrowStmt()
    {
        delete m_Value;
    }

    TryCatchStmt::TryCatchStmt(BlockStmt *tryBlock, std::vector<CatchClause> catches)
        : Statement(NodeType::TryCatchStmt), m_TryBlock(tryBlock), m_Catches(std::move(catches)) {}

    BlockStmt *TryCatchStmt::GetTryBlock() const
    {
        return m_TryBlock;
    }

    const std::vector<TryCatchStmt::CatchClause> &TryCatchStmt::GetCatches() const
    {
        return m_Catches;
    }

    TryCatchStmt::~TryCatchStmt()
    {
        delete m_TryBlock;
        for (auto &clause : m_Catches)
        {
            delete clause.block;
            clause.block = nullptr;
        }
    }

    DeleteStmt::DeleteStmt(Expression *target)
        : Statement(NodeType::DeleteStmt), m_Target(target) {}

    Expression *DeleteStmt::GetTarget() const
    {
        return m_Target;
    }

    DeleteStmt::~DeleteStmt()
    {
        delete m_Target;
    }

    SwitchStmt::SwitchStmt()
        : Statement(NodeType::SwitchStmt) {}

    SwitchStmt::~SwitchStmt() = default;

    ReturnStmt::ReturnStmt(Expression *value)
        : Statement(NodeType::ReturnStmt), m_Value(value) {}

    Expression *ReturnStmt::GetValue() const
    {
        return m_Value;
    }

    ReturnStmt::~ReturnStmt()
    {
        delete m_Value;
    }

    FunctionDeclStmt::FunctionDeclStmt(std::string name, std::deque<std::string> parameters, BlockStmt *body, AccessModifier access,
                                       std::optional<std::string> returnType, std::string doc)
        : Statement(NodeType::NewStmt),
          m_Name(std::move(name)),
          m_Parameters(std::move(parameters)),
          m_Body(body),
          m_Access(access),
          m_ReturnType(std::move(returnType)),
          m_Doc(std::move(doc)) {}

    const std::string &FunctionDeclStmt::GetName() const
    {
        return m_Name;
    }

    const std::deque<std::string> &FunctionDeclStmt::GetParameters() const
    {
        return m_Parameters;
    }

    BlockStmt *FunctionDeclStmt::GetBody() const
    {
        return m_Body;
    }

    AccessModifier FunctionDeclStmt::GetAccessModifier() const
    {
        return m_Access;
    }

    const std::optional<std::string> &FunctionDeclStmt::GetReturnType() const
    {
        return m_ReturnType;
    }

    const std::string &FunctionDeclStmt::GetDoc() const
    {
        return m_Doc;
    }

    FunctionDeclStmt::~FunctionDeclStmt()
    {
        delete m_Body;
    }

    ClassDeclStmt::ClassDeclStmt(std::string name, std::deque<VarDeclStmt *> fields, std::deque<FunctionDeclStmt *> methods, std::string doc)
        : Statement(NodeType::NewStmt), m_Name(std::move(name)), m_Fields(std::move(fields)), m_Methods(std::move(methods)), m_Doc(std::move(doc)) {}

    const std::string &ClassDeclStmt::GetName() const
    {
        return m_Name;
    }

    const std::deque<VarDeclStmt *> &ClassDeclStmt::GetFields() const
    {
        return m_Fields;
    }

    const std::deque<FunctionDeclStmt *> &ClassDeclStmt::GetMethods() const
    {
        return m_Methods;
    }

    const std::string &ClassDeclStmt::GetDoc() const
    {
        return m_Doc;
    }

    ClassDeclStmt::~ClassDeclStmt()
    {
        for (VarDeclStmt *field : m_Fields)
        {
            delete field;
        }
        for (FunctionDeclStmt *method : m_Methods)
        {
            delete method;
        }
    }

    NamespaceDeclStmt::NamespaceDeclStmt(std::string name, BlockStmt *body)
        : Statement(NodeType::NewStmt), m_Name(std::move(name)), m_Body(body) {}

    const std::string &NamespaceDeclStmt::GetName() const
    {
        return m_Name;
    }

    BlockStmt *NamespaceDeclStmt::GetBody() const
    {
        return m_Body;
    }

    NamespaceDeclStmt::~NamespaceDeclStmt()
    {
        delete m_Body;
    }

    ImportStmt::ImportStmt(std::string moduleName)
        : Statement(NodeType::PassStmt), m_ModuleName(std::move(moduleName)) {}

    const std::string &ImportStmt::GetModuleName() const
    {
        return m_ModuleName;
    }

    ImportStmt::~ImportStmt() = default;

    ForEachStmt::ForEachStmt()
        : Statement(NodeType::ForEachStmt) {}

    ForEachStmt::~ForEachStmt() = default;

    ContinueStmt::ContinueStmt()
        : Statement(NodeType::ContinueStmt) {}

    ForRangeStmt::ForRangeStmt(std::string loopName, Expression *startExpr, Expression *endExpr, Expression *stepExpr, BlockStmt *loopBlock)
        : Statement(NodeType::ForStmt),
          name(std::move(loopName)),
          start(startExpr),
          end(endExpr),
          step(stepExpr),
          block(loopBlock) {}

    ForRangeStmt::~ForRangeStmt()
    {
        delete start;
        delete end;
        delete step;
        delete block;
    }

    ForCStyleStmt::ForCStyleStmt(Statement *initStmt, Expression *conditionExpr, Statement *updateStmt, BlockStmt *loopBlock)
        : Statement(NodeType::ForStmt),
          init(initStmt),
          condition(conditionExpr),
          update(updateStmt),
          block(loopBlock) {}

    ForCStyleStmt::~ForCStyleStmt()
    {
        delete init;
        delete condition;
        delete update;
        delete block;
    }

} // namespace cora::ast
