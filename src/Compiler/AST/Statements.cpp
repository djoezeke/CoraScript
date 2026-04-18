#include "Cora/Compiler/AST/Statements.hpp"

namespace cora::compiler
{
    namespace ast
    {
        Statement::Statement()
            : Node(NodeType::Statement), m_StmtType(StatementType::PassStmt) {}

        Statement::Statement(StatementType kind)
            : Node(NodeType::Statement), m_StmtType(kind) {}

        StatementType Statement::GetStmtType() const
        {
            return m_StmtType;
        }

        std::string Statement::Repr()
        {
            return "Statement";
        }

        Statement::~Statement() = default;

        ExprStmt::ExprStmt(Expression *expr)
            : Statement(StatementType::PassStmt), m_Expr(expr) {}

        Expression *ExprStmt::GetExpression() const
        {
            return m_Expr;
        }

        ExprStmt::~ExprStmt()
        {
            delete m_Expr;
        }

        PrintStmt::PrintStmt()
            : Statement(StatementType::PassStmt) {}

        PrintStmt::~PrintStmt()
        {
            for (Expression *expr : expressions)
            {
                delete expr;
            }
        }

        AssignStmt::AssignStmt(Expression *target, Expression *expr)
            : Statement(StatementType::PassStmt), m_Target(target), m_Value(expr) {}

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

        VarDeclStmt::VarDeclStmt(std::string name, std::optional<std::string> declaredType, Expression *expr, AccessModifier access)
            : Statement(StatementType::NewStmt), m_Name(std::move(name)), m_DeclaredType(std::move(declaredType)), m_Expr(expr), m_Access(access) {}

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

        VarDeclStmt::~VarDeclStmt()
        {
            delete m_Expr;
        }

        IfStmt::IfStmt()
            : Statement(StatementType::IfStmt), elseBlock(nullptr) {}

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
            : Statement(StatementType::DoStmt) {}

        DoStmt::~DoStmt() = default;

        ForStmt::ForStmt()
            : Statement(StatementType::ForStmt) {}

        ForStmt::~ForStmt() = default;

        NewStmt::NewStmt()
            : Statement(StatementType::NewStmt) {}

        NewStmt::~NewStmt() = default;

        PassStmt::PassStmt()
            : Statement(StatementType::PassStmt) {}

        WhileStmt::WhileStmt()
            : Statement(StatementType::WhileStmt), condition(nullptr), block(nullptr) {}

        WhileStmt::WhileStmt(Expression *cond, BlockStmt *blk)
            : Statement(StatementType::WhileStmt), condition(cond), block(blk) {}

        WhileStmt::~WhileStmt()
        {
            delete condition;
            delete block;
        }

        BreakStmt::BreakStmt()
            : Statement(StatementType::BreakStmt) {}

        BlockStmt::BlockStmt()
            : Statement(StatementType::BraceStmt) {}

        BlockStmt::~BlockStmt()
        {
            for (Statement *stmt : statements)
            {
                delete stmt;
            }
        }

        YieldStmt::YieldStmt()
            : Statement(StatementType::YieldStmt) {}

        YieldStmt::~YieldStmt() = default;

        ThrowStmt::ThrowStmt()
            : Statement(StatementType::ThrowStmt) {}

        ThrowStmt::~ThrowStmt() = default;

        DeleteStmt::DeleteStmt(Expression *target)
            : Statement(StatementType::DeleteStmt), m_Target(target) {}

        Expression *DeleteStmt::GetTarget() const
        {
            return m_Target;
        }

        DeleteStmt::~DeleteStmt()
        {
            delete m_Target;
        }

        SwitchStmt::SwitchStmt()
            : Statement(StatementType::SwitchStmt) {}

        SwitchStmt::~SwitchStmt() = default;

        ReturnStmt::ReturnStmt(Expression *value)
            : Statement(StatementType::ReturnStmt), m_Value(value) {}

        Expression *ReturnStmt::GetValue() const
        {
            return m_Value;
        }

        ReturnStmt::~ReturnStmt()
        {
            delete m_Value;
        }

        FunctionDeclStmt::FunctionDeclStmt(std::string name, std::deque<std::string> parameters, BlockStmt *body, AccessModifier access)
            : Statement(StatementType::NewStmt),
              m_Name(std::move(name)),
              m_Parameters(std::move(parameters)),
              m_Body(body),
              m_Access(access) {}

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

        FunctionDeclStmt::~FunctionDeclStmt()
        {
            delete m_Body;
        }

        ClassDeclStmt::ClassDeclStmt(std::string name, std::deque<VarDeclStmt *> fields, std::deque<FunctionDeclStmt *> methods)
            : Statement(StatementType::NewStmt), m_Name(std::move(name)), m_Fields(std::move(fields)), m_Methods(std::move(methods)) {}

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
            : Statement(StatementType::NewStmt), m_Name(std::move(name)), m_Body(body) {}

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
            : Statement(StatementType::PassStmt), m_ModuleName(std::move(moduleName)) {}

        const std::string &ImportStmt::GetModuleName() const
        {
            return m_ModuleName;
        }

        ImportStmt::~ImportStmt() = default;

        ForEachStmt::ForEachStmt()
            : Statement(StatementType::ForEachStmt) {}

        ForEachStmt::~ForEachStmt() = default;

        ContinueStmt::ContinueStmt()
            : Statement(StatementType::ContinueStmt) {}

        ForRangeStmt::ForRangeStmt(std::string loopName, Expression *startExpr, Expression *endExpr, Expression *stepExpr, BlockStmt *loopBlock)
            : Statement(StatementType::ForStmt),
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
            : Statement(StatementType::ForStmt),
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

    } // namespace ast

} // namespace cora::compiler
