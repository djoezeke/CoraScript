#include "Validator.hpp"

#include "../AST/ASTExpr.hpp"
#include "Cora/Basic/Error.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cora::compiler
{
    namespace semantic
    {
        namespace
        {
            struct VariableInfo
            {
                bool isConst{false};
                std::optional<std::string> classType;
            };

            struct FieldInfo
            {
                ast::AccessModifier access{ast::AccessModifier::Private};
                bool isConst{false};
                bool hasInitializer{false};
            };

            struct ClassInfo
            {
                std::unordered_map<std::string, FieldInfo> fields;
                std::unordered_map<std::string, bool> methodIsPublic;
            };

            class Validator
            {
            public:
                Validator(std::string fileName, std::string moduleName)
                    : m_FileName(fileName.empty() ? "<memory>" : std::move(fileName)),
                      m_ModuleName(std::move(moduleName))
                {
                    EnterScope();
                }

                void Validate(const std::deque<ast::Statement *> &program)
                {
                    CollectClassInfo(program);
                    for (ast::Statement *statement : program)
                    {
                        VisitStatement(statement);
                    }
                }

            private:
                void CollectClassInfo(const std::deque<ast::Statement *> &statements)
                {
                    for (ast::Statement *statement : statements)
                    {
                        if (auto *classDecl = dynamic_cast<ast::ClassDeclStmt *>(statement))
                        {
                            ClassInfo info;
                            for (ast::VarDeclStmt *field : classDecl->GetFields())
                            {
                                info.fields[field->GetName()] = FieldInfo{field->GetAccessModifier(), field->IsConst(), field->GetExpression() != nullptr};
                            }
                            for (ast::FunctionDeclStmt *method : classDecl->GetMethods())
                            {
                                const bool isPublic = method->GetAccessModifier() == ast::AccessModifier::Public;
                                auto it = info.methodIsPublic.find(method->GetName());
                                if (it == info.methodIsPublic.end())
                                {
                                    info.methodIsPublic.emplace(method->GetName(), isPublic);
                                }
                                else
                                {
                                    it->second = it->second || isPublic;
                                }
                            }
                            m_Classes[classDecl->GetName()] = std::move(info);
                            continue;
                        }

                        if (auto *namespaceDecl = dynamic_cast<ast::NamespaceDeclStmt *>(statement))
                        {
                            if (namespaceDecl->GetBody() != nullptr)
                            {
                                CollectClassInfo(namespaceDecl->GetBody()->statements);
                            }
                        }
                    }
                }

                void EnterScope()
                {
                    m_Scopes.emplace_back();
                }

                void ExitScope()
                {
                    if (!m_Scopes.empty())
                    {
                        m_Scopes.pop_back();
                    }
                }

                void DeclareVariable(const std::string &name, VariableInfo info)
                {
                    if (m_Scopes.empty())
                    {
                        EnterScope();
                    }
                    m_Scopes.back()[name] = std::move(info);
                }

                VariableInfo *LookupVariable(const std::string &name)
                {
                    for (auto it = m_Scopes.rbegin(); it != m_Scopes.rend(); ++it)
                    {
                        auto found = it->find(name);
                        if (found != it->end())
                        {
                            return &found->second;
                        }
                    }
                    return nullptr;
                }

                std::optional<std::string> ResolveExpressionClass(ast::Expression *expr)
                {
                    if (expr == nullptr)
                    {
                        return std::nullopt;
                    }

                    if (auto *variable = dynamic_cast<ast::VariableExpr *>(expr))
                    {
                        if (variable->GetName() == "this" && !m_ClassStack.empty())
                        {
                            return m_ClassStack.back();
                        }

                        VariableInfo *info = LookupVariable(variable->GetName());
                        if (info != nullptr)
                        {
                            return info->classType;
                        }

                        return std::nullopt;
                    }

                    if (auto *create = dynamic_cast<ast::NewExpr *>(expr))
                    {
                        return create->GetClassName();
                    }

                    if (auto *call = dynamic_cast<ast::CallExpr *>(expr))
                    {
                        if (auto *calleeVar = dynamic_cast<ast::VariableExpr *>(call->GetCallee()))
                        {
                            if (m_Classes.find(calleeVar->GetName()) != m_Classes.end())
                            {
                                return calleeVar->GetName();
                            }
                        }
                    }

                    return std::nullopt;
                }

                bool IsCurrentClass(const std::string &className) const
                {
                    return !m_ClassStack.empty() && m_ClassStack.back() == className;
                }

                std::string CurrentNamespacePath() const
                {
                    std::string joined;
                    for (const auto &entry : m_NamespaceStack)
                    {
                        if (!joined.empty())
                        {
                            joined += "::";
                        }
                        joined += entry;
                    }
                    return joined;
                }

                [[noreturn]] void Raise(const std::string &message, const ast::Node *node) const
                {
                    error::DiagnosticContext context;
                    context.fileName = m_FileName;
                    context.moduleName = m_ModuleName;
                    context.namespaceName = CurrentNamespacePath();
                    if (!m_ClassStack.empty())
                    {
                        context.className = m_ClassStack.back();
                    }
                    if (!m_FunctionStack.empty())
                    {
                        context.functionName = m_FunctionStack.back();
                    }
                    if (node != nullptr)
                    {
                        context.line = node->GetStartPosition().Line();
                        context.column = node->GetStartPosition().Column();
                    }
                    throw error::ParsingError(message, context);
                }

                void ValidateMemberRead(ast::MemberExpr *memberExpr)
                {
                    if (memberExpr == nullptr)
                    {
                        return;
                    }

                    VisitExpression(memberExpr->GetObject());

                    const std::optional<std::string> ownerClass = ResolveExpressionClass(memberExpr->GetObject());
                    if (!ownerClass.has_value())
                    {
                        return;
                    }

                    auto classIt = m_Classes.find(*ownerClass);
                    if (classIt == m_Classes.end())
                    {
                        return;
                    }

                    const std::string &memberName = memberExpr->GetMember();
                    const auto fieldIt = classIt->second.fields.find(memberName);
                    if (fieldIt != classIt->second.fields.end())
                    {
                        if (fieldIt->second.access == ast::AccessModifier::Private && !IsCurrentClass(*ownerClass))
                        {
                            Raise("Cannot access private member: " + memberName, memberExpr);
                        }
                        return;
                    }

                    const auto methodIt = classIt->second.methodIsPublic.find(memberName);
                    if (methodIt != classIt->second.methodIsPublic.end())
                    {
                        if (!methodIt->second && !IsCurrentClass(*ownerClass))
                        {
                            Raise("Cannot access private member: " + memberName, memberExpr);
                        }
                    }
                }

                void ValidateMemberAssignment(ast::MemberExpr *memberExpr)
                {
                    if (memberExpr == nullptr)
                    {
                        return;
                    }

                    VisitExpression(memberExpr->GetObject());

                    const std::optional<std::string> ownerClass = ResolveExpressionClass(memberExpr->GetObject());
                    if (!ownerClass.has_value())
                    {
                        return;
                    }

                    auto classIt = m_Classes.find(*ownerClass);
                    if (classIt == m_Classes.end())
                    {
                        return;
                    }

                    const std::string &memberName = memberExpr->GetMember();
                    const auto fieldIt = classIt->second.fields.find(memberName);
                    if (fieldIt == classIt->second.fields.end())
                    {
                        return;
                    }

                    if (fieldIt->second.access == ast::AccessModifier::Private && !IsCurrentClass(*ownerClass))
                    {
                        Raise("Cannot assign private member: " + memberName, memberExpr);
                    }

                    if (!fieldIt->second.isConst)
                    {
                        return;
                    }

                    if (fieldIt->second.hasInitializer)
                    {
                        Raise("Cannot assign to constant member: " + memberName, memberExpr);
                    }

                    const bool inConstructor = !m_FunctionStack.empty() && m_FunctionStack.back() == "__init__" && IsCurrentClass(*ownerClass);
                    const auto *objectVariable = dynamic_cast<ast::VariableExpr *>(memberExpr->GetObject());
                    const bool isThisObject = objectVariable != nullptr && objectVariable->GetName() == "this";
                    if (!(inConstructor && isThisObject))
                    {
                        Raise("Cannot assign to constant member: " + memberName, memberExpr);
                    }

                    if (!m_CurrentCtorConstAssignments.has_value())
                    {
                        Raise("Cannot assign to constant member: " + memberName, memberExpr);
                    }

                    auto [_, inserted] = m_CurrentCtorConstAssignments->insert(memberName);
                    if (!inserted)
                    {
                        Raise("Cannot assign to constant member: " + memberName, memberExpr);
                    }
                }

                void ValidateAssignmentTarget(ast::Expression *target)
                {
                    if (auto *variableTarget = dynamic_cast<ast::VariableExpr *>(target))
                    {
                        VariableInfo *info = LookupVariable(variableTarget->GetName());
                        if (info != nullptr && info->isConst)
                        {
                            Raise("Cannot assign to constant variable: " + variableTarget->GetName(), variableTarget);
                        }
                        return;
                    }

                    if (auto *memberTarget = dynamic_cast<ast::MemberExpr *>(target))
                    {
                        ValidateMemberAssignment(memberTarget);
                        return;
                    }

                    if (target != nullptr)
                    {
                        VisitExpression(target);
                    }
                }

                void VisitExpression(ast::Expression *expression)
                {
                    if (expression == nullptr)
                    {
                        return;
                    }

                    if (auto *unary = dynamic_cast<ast::UnaryExpr *>(expression))
                    {
                        VisitExpression(unary->GetRhs());
                        return;
                    }

                    if (auto *binary = dynamic_cast<ast::BinaryExpr *>(expression))
                    {
                        VisitExpression(binary->GetLeft());
                        VisitExpression(binary->GetRight());
                        return;
                    }

                    if (auto *call = dynamic_cast<ast::CallExpr *>(expression))
                    {
                        VisitExpression(call->GetCallee());
                        for (ast::Expression *argument : call->GetArguments())
                        {
                            VisitExpression(argument);
                        }
                        return;
                    }

                    if (auto *member = dynamic_cast<ast::MemberExpr *>(expression))
                    {
                        ValidateMemberRead(member);
                        return;
                    }

                    if (auto *create = dynamic_cast<ast::NewExpr *>(expression))
                    {
                        for (ast::Expression *argument : create->GetArguments())
                        {
                            VisitExpression(argument);
                        }
                    }
                }

                void VisitBlock(ast::BlockStmt *block)
                {
                    if (block == nullptr)
                    {
                        return;
                    }

                    EnterScope();
                    for (ast::Statement *statement : block->statements)
                    {
                        VisitStatement(statement);
                    }
                    ExitScope();
                }

                void VisitFunction(ast::FunctionDeclStmt *function)
                {
                    if (function == nullptr)
                    {
                        return;
                    }

                    m_FunctionStack.push_back(function->GetName());
                    EnterScope();

                    if (!m_ClassStack.empty())
                    {
                        DeclareVariable("this", VariableInfo{false, m_ClassStack.back()});
                    }

                    for (const std::string &parameter : function->GetParameters())
                    {
                        DeclareVariable(parameter, VariableInfo{});
                    }

                    const std::optional<std::unordered_set<std::string>> previousCtorAssignments = m_CurrentCtorConstAssignments;
                    if (!m_ClassStack.empty() && function->GetName() == "__init__")
                    {
                        m_CurrentCtorConstAssignments = std::unordered_set<std::string>{};
                    }
                    else
                    {
                        m_CurrentCtorConstAssignments.reset();
                    }

                    if (function->GetBody() != nullptr)
                    {
                        for (ast::Statement *statement : function->GetBody()->statements)
                        {
                            VisitStatement(statement);
                        }
                    }

                    m_CurrentCtorConstAssignments = previousCtorAssignments;
                    ExitScope();
                    m_FunctionStack.pop_back();
                }

                void VisitClass(ast::ClassDeclStmt *classDecl)
                {
                    if (classDecl == nullptr)
                    {
                        return;
                    }

                    m_ClassStack.push_back(classDecl->GetName());

                    for (ast::VarDeclStmt *field : classDecl->GetFields())
                    {
                        if (field != nullptr && field->GetExpression() != nullptr)
                        {
                            VisitExpression(field->GetExpression());
                        }
                    }

                    for (ast::FunctionDeclStmt *method : classDecl->GetMethods())
                    {
                        VisitFunction(method);
                    }

                    m_ClassStack.pop_back();
                }

                void VisitStatement(ast::Statement *statement)
                {
                    if (statement == nullptr)
                    {
                        return;
                    }

                    if (auto *varDecl = dynamic_cast<ast::VarDeclaration *>(statement))
                    {
                        VisitExpression(varDecl->GetExpression());

                        std::optional<std::string> classType;
                        if (m_Classes.find(varDecl->GetType()) != m_Classes.end())
                        {
                            classType = varDecl->GetType();
                        }
                        DeclareVariable(varDecl->GetName(), VariableInfo{varDecl->IsConst(), classType});
                        return;
                    }

                    if (auto *varDecl = dynamic_cast<ast::VarDeclStmt *>(statement))
                    {
                        if (varDecl->GetExpression() != nullptr)
                        {
                            VisitExpression(varDecl->GetExpression());
                        }

                        std::optional<std::string> classType;
                        if (varDecl->GetDeclaredType().has_value() && m_Classes.find(*varDecl->GetDeclaredType()) != m_Classes.end())
                        {
                            classType = *varDecl->GetDeclaredType();
                        }
                        DeclareVariable(varDecl->GetName(), VariableInfo{varDecl->IsConst(), classType});
                        return;
                    }

                    if (auto *assignment = dynamic_cast<ast::Assignment *>(statement))
                    {
                        VisitExpression(assignment->GetValue());
                        ValidateAssignmentTarget(assignment->GetTarget());
                        return;
                    }

                    if (auto *assignment = dynamic_cast<ast::AssignStmt *>(statement))
                    {
                        VisitExpression(assignment->GetValue());
                        ValidateAssignmentTarget(assignment->GetTarget());
                        return;
                    }

                    if (auto *exprStmt = dynamic_cast<ast::ExprStmt *>(statement))
                    {
                        VisitExpression(exprStmt->GetExpression());
                        return;
                    }

                    if (auto *printStmt = dynamic_cast<ast::PrintStmt *>(statement))
                    {
                        for (ast::Expression *expression : printStmt->expressions)
                        {
                            VisitExpression(expression);
                        }
                        return;
                    }

                    if (auto *ifStmt = dynamic_cast<ast::IfStmt *>(statement))
                    {
                        for (const auto &branch : ifStmt->branches)
                        {
                            VisitExpression(branch.first);
                            VisitBlock(branch.second);
                        }
                        if (ifStmt->elseBlock != nullptr)
                        {
                            VisitBlock(ifStmt->elseBlock);
                        }
                        return;
                    }

                    if (auto *whileStmt = dynamic_cast<ast::WhileStmt *>(statement))
                    {
                        VisitExpression(whileStmt->condition);
                        VisitBlock(whileStmt->block);
                        return;
                    }

                    if (auto *forRange = dynamic_cast<ast::ForRangeStmt *>(statement))
                    {
                        EnterScope();
                        DeclareVariable(forRange->name, VariableInfo{});
                        VisitExpression(forRange->start);
                        VisitExpression(forRange->end);
                        VisitExpression(forRange->step);
                        VisitBlock(forRange->block);
                        ExitScope();
                        return;
                    }

                    if (auto *forCStyle = dynamic_cast<ast::ForCStyleStmt *>(statement))
                    {
                        EnterScope();
                        VisitStatement(forCStyle->init);
                        VisitExpression(forCStyle->condition);
                        VisitStatement(forCStyle->update);
                        VisitBlock(forCStyle->block);
                        ExitScope();
                        return;
                    }

                    if (auto *returnStmt = dynamic_cast<ast::ReturnStmt *>(statement))
                    {
                        VisitExpression(returnStmt->GetValue());
                        return;
                    }

                    if (auto *throwStmt = dynamic_cast<ast::ThrowStmt *>(statement))
                    {
                        VisitExpression(throwStmt->GetValue());
                        return;
                    }

                    if (auto *tryCatch = dynamic_cast<ast::TryCatchStmt *>(statement))
                    {
                        VisitBlock(tryCatch->GetTryBlock());
                        for (const auto &catchClause : tryCatch->GetCatches())
                        {
                            EnterScope();
                            if (catchClause.variableName.has_value())
                            {
                                DeclareVariable(*catchClause.variableName, VariableInfo{});
                            }
                            VisitBlock(catchClause.block);
                            ExitScope();
                        }
                        return;
                    }

                    if (auto *deleteStmt = dynamic_cast<ast::DeleteStmt *>(statement))
                    {
                        VisitExpression(deleteStmt->GetTarget());
                        return;
                    }

                    if (auto *functionDecl = dynamic_cast<ast::FunctionDeclStmt *>(statement))
                    {
                        VisitFunction(functionDecl);
                        return;
                    }

                    if (auto *classDecl = dynamic_cast<ast::ClassDeclStmt *>(statement))
                    {
                        VisitClass(classDecl);
                        return;
                    }

                    if (auto *namespaceDecl = dynamic_cast<ast::NamespaceDeclStmt *>(statement))
                    {
                        m_NamespaceStack.push_back(namespaceDecl->GetName());
                        VisitBlock(namespaceDecl->GetBody());
                        m_NamespaceStack.pop_back();
                        return;
                    }

                    if (auto *block = dynamic_cast<ast::BlockStmt *>(statement))
                    {
                        VisitBlock(block);
                    }
                }

            private:
                std::string m_FileName;
                std::string m_ModuleName;
                std::unordered_map<std::string, ClassInfo> m_Classes;
                std::vector<std::unordered_map<std::string, VariableInfo>> m_Scopes;
                std::vector<std::string> m_NamespaceStack;
                std::vector<std::string> m_ClassStack;
                std::vector<std::string> m_FunctionStack;
                std::optional<std::unordered_set<std::string>> m_CurrentCtorConstAssignments;
            };
        } // namespace

        void ValidateProgram(const std::deque<ast::Statement *> &program,
                             const std::string &fileName,
                             const std::string &moduleName)
        {
            Validator validator(fileName, moduleName);
            validator.Validate(program);
        }

    } // namespace semantic

} // namespace cora::compiler
