#include "Cora/Compiler/Runtime/Interpreter.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace cora::compiler
{
    namespace runtime
    {
        namespace
        {
            struct BreakSignal
            {
            };

            struct ContinueSignal
            {
            };
        }

        void Interpreter::Run(const std::string &source)
        {
            parser::Parser parser;
            std::deque<Statement *> program = parser.ParseProgram(source);
            Execute(program);
            for (Statement *stmt : program)
            {
                delete stmt;
            }
        }

        void Interpreter::RunFile(const std::string &path)
        {
            std::ifstream input(path);
            if (!input)
            {
                throw std::runtime_error("Unable to open file: " + path);
            }

            std::stringstream buffer;
            buffer << input.rdbuf();
            Run(buffer.str());
        }

        void Interpreter::Execute(const std::deque<Statement *> &program)
        {
            for (Statement *stmt : program)
            {
                ExecStmt(stmt);
            }
        }

        runtime::Value Interpreter::EvalExpr(Expression *expr)
        {
            if (auto *lit = dynamic_cast<ast::LiteralExpr *>(expr))
            {
                const ast::LiteralValue &value = lit->GetValue();
                if (std::holds_alternative<std::monostate>(value))
                {
                    return runtime::Value(nullptr);
                }
                if (std::holds_alternative<bool>(value))
                {
                    return runtime::Value(std::get<bool>(value));
                }
                if (std::holds_alternative<double>(value))
                {
                    return runtime::Value(std::get<double>(value));
                }
                return runtime::Value(std::get<std::string>(value));
            }

            if (auto *var = dynamic_cast<ast::VariableExpr *>(expr))
            {
                Scope *scope = m_CurrentScope->ResolveVariable(var->GetName());
                if (scope == nullptr)
                {
                    throw std::runtime_error("Undefined variable: " + var->GetName());
                }

                Variable *variable = scope->GetVariable(var->GetName());
                if (variable == nullptr || variable->GetValue() == nullptr)
                {
                    throw std::runtime_error("Undefined variable: " + var->GetName());
                }

                return *(variable->GetValue());
            }

            if (auto *unary = dynamic_cast<ast::UnaryExpr *>(expr))
            {
                runtime::Value rhs = EvalExpr(unary->GetRhs());
                return ApplyUnary(unary->GetOperator(), rhs);
            }

            if (auto *binary = dynamic_cast<ast::BinaryExpr *>(expr))
            {
                runtime::Value lhs = EvalExpr(binary->GetLeft());
                const TokenType op = binary->GetOperator();

                if (op == TokenType::And)
                {
                    if (!IsTruthy(lhs))
                    {
                        return runtime::Value(false);
                    }
                    return runtime::Value(IsTruthy(EvalExpr(binary->GetRight())));
                }
                if (op == TokenType::Or)
                {
                    if (IsTruthy(lhs))
                    {
                        return runtime::Value(true);
                    }
                    return runtime::Value(IsTruthy(EvalExpr(binary->GetRight())));
                }

                runtime::Value rhs = EvalExpr(binary->GetRight());
                return ApplyBinary(op, lhs, rhs);
            }

            throw std::runtime_error("Unknown expression node");
        }

        void Interpreter::ExecStmt(Statement *stmt)
        {
            if (auto *print = dynamic_cast<ast::PrintStmt *>(stmt))
            {
                bool first = true;
                for (Expression *expr : print->expressions)
                {
                    if (!first)
                    {
                        std::cout << ' ';
                    }
                    std::cout << ToString(EvalExpr(expr));
                    first = false;
                }
                std::cout << '\n';
                return;
            }

            if (auto *exprStmt = dynamic_cast<ast::ExprStmt *>(stmt))
            {
                (void)EvalExpr(exprStmt->GetExpression());
                return;
            }

            if (auto *assign = dynamic_cast<ast::AssignStmt *>(stmt))
            {
                Scope *scope = m_CurrentScope->ResolveVariable(assign->GetName());
                if (scope == nullptr)
                {
                    throw std::runtime_error("Assignment to undefined variable: " + assign->GetName());
                }

                Variable *variable = scope->GetVariable(assign->GetName());
                runtime::Value value = EvalExpr(assign->GetExpression());
                CheckTypeCompatibility(std::nullopt, value, assign->GetName());
                variable->SetValue(new runtime::Value(value));
                return;
            }

            if (auto *decl = dynamic_cast<ast::VarDeclStmt *>(stmt))
            {
                if (m_CurrentScope->GetVariable(decl->GetName()) != nullptr)
                {
                    throw std::runtime_error("Variable already declared: " + decl->GetName());
                }

                runtime::Value value = EvalExpr(decl->GetExpression());
                CheckTypeCompatibility(decl->GetDeclaredType(), value, decl->GetName());
                m_CurrentScope->NewVariableValue(decl->GetName(), new runtime::Value(value), false);
                return;
            }

            if (auto *block = dynamic_cast<ast::BlockStmt *>(stmt))
            {
                ExecBlock(block);
                return;
            }

            if (auto *ifStmt = dynamic_cast<ast::IfStmt *>(stmt))
            {
                for (const auto &branch : ifStmt->branches)
                {
                    if (IsTruthy(EvalExpr(branch.first)))
                    {
                        ExecBlock(branch.second);
                        return;
                    }
                }

                if (ifStmt->elseBlock != nullptr)
                {
                    ExecBlock(ifStmt->elseBlock);
                }
                return;
            }

            if (auto *whileStmt = dynamic_cast<ast::WhileStmt *>(stmt))
            {
                while (IsTruthy(EvalExpr(whileStmt->condition)))
                {
                    try
                    {
                        ExecBlock(whileStmt->block);
                    }
                    catch (const BreakSignal &)
                    {
                        break;
                    }
                    catch (const ContinueSignal &)
                    {
                        continue;
                    }
                }
                return;
            }

            if (auto *forRange = dynamic_cast<ast::ForRangeStmt *>(stmt))
            {
                const double start = AsNumber(EvalExpr(forRange->start));
                const double end = AsNumber(EvalExpr(forRange->end));
                const double step = AsNumber(EvalExpr(forRange->step));
                if (step == 0.0)
                {
                    throw std::runtime_error("range step cannot be 0");
                }

                for (double i = start; step > 0.0 ? i < end : i > end; i += step)
                {
                    Scope *scope = m_CurrentScope->ResolveVariable(forRange->name);
                    if (scope == nullptr)
                    {
                        m_CurrentScope->NewVariableValue(forRange->name, new runtime::Value(i), false);
                    }
                    else
                    {
                        Variable *var = scope->GetVariable(forRange->name);
                        var->SetValue(new runtime::Value(i));
                    }

                    try
                    {
                        ExecBlock(forRange->block);
                    }
                    catch (const BreakSignal &)
                    {
                        break;
                    }
                    catch (const ContinueSignal &)
                    {
                        continue;
                    }
                }
                return;
            }

            if (auto *forCStyle = dynamic_cast<ast::ForCStyleStmt *>(stmt))
            {
                if (forCStyle->init != nullptr)
                {
                    ExecStmt(forCStyle->init);
                }

                while (forCStyle->condition == nullptr || IsTruthy(EvalExpr(forCStyle->condition)))
                {
                    bool continueLoop = false;
                    try
                    {
                        ExecBlock(forCStyle->block);
                    }
                    catch (const BreakSignal &)
                    {
                        break;
                    }
                    catch (const ContinueSignal &)
                    {
                        continueLoop = true;
                    }

                    if (forCStyle->update != nullptr)
                    {
                        ExecStmt(forCStyle->update);
                    }

                    if (continueLoop)
                    {
                        continue;
                    }
                }
                return;
            }

            if (dynamic_cast<ast::BreakStmt *>(stmt) != nullptr)
            {
                throw BreakSignal{};
            }

            if (dynamic_cast<ast::ContinueStmt *>(stmt) != nullptr)
            {
                throw ContinueSignal{};
            }

            if (dynamic_cast<ast::PassStmt *>(stmt) != nullptr)
            {
                return;
            }

            throw std::runtime_error("Unknown statement node");
        }

        void Interpreter::ExecBlock(BlockStmt *block)
        {
            Scope blockScope(m_CurrentScope, ScopeKind::Block);
            Scope *parent = m_CurrentScope;
            m_CurrentScope = &blockScope;

            try
            {
                for (Statement *stmt : block->statements)
                {
                    ExecStmt(stmt);
                }
            }
            catch (...)
            {
                m_CurrentScope = parent;
                throw;
            }

            m_CurrentScope = parent;
        }

        bool Interpreter::IsTruthy(const runtime::Value &value) const
        {
            return value.AsBool();
        }

        double Interpreter::AsNumber(const runtime::Value &value) const
        {
            return value.AsNumber();
        }

        std::string Interpreter::ToString(const runtime::Value &value) const
        {
            return value.AsString();
        }

        runtime::Value Interpreter::ApplyBinary(TokenType op, const runtime::Value &lhs, const runtime::Value &rhs) const
        {
            switch (op)
            {
            case TokenType::Plus:
                if (lhs.IsString() || rhs.IsString())
                {
                    return runtime::Value(ToString(lhs) + ToString(rhs));
                }
                return runtime::Value(AsNumber(lhs) + AsNumber(rhs));
            case TokenType::Minus:
                return runtime::Value(AsNumber(lhs) - AsNumber(rhs));
            case TokenType::Star:
                return runtime::Value(AsNumber(lhs) * AsNumber(rhs));
            case TokenType::Slash:
                if (AsNumber(rhs) == 0.0)
                {
                    throw std::runtime_error("Division by zero");
                }
                return runtime::Value(AsNumber(lhs) / AsNumber(rhs));
            case TokenType::Percent:
                if (AsNumber(rhs) == 0.0)
                {
                    throw std::runtime_error("Modulo by zero");
                }
                return runtime::Value(std::fmod(AsNumber(lhs), AsNumber(rhs)));
            case TokenType::Equal:
                return runtime::Value(ValuesEqual(lhs, rhs));
            case TokenType::NotEqual:
                return runtime::Value(!ValuesEqual(lhs, rhs));
            case TokenType::Less:
                return runtime::Value(AsNumber(lhs) < AsNumber(rhs));
            case TokenType::LessEqual:
                return runtime::Value(AsNumber(lhs) <= AsNumber(rhs));
            case TokenType::Greater:
                return runtime::Value(AsNumber(lhs) > AsNumber(rhs));
            case TokenType::GreaterEqual:
                return runtime::Value(AsNumber(lhs) >= AsNumber(rhs));
            case TokenType::And:
                return runtime::Value(IsTruthy(lhs) && IsTruthy(rhs));
            case TokenType::Or:
                return runtime::Value(IsTruthy(lhs) || IsTruthy(rhs));
            default:
                throw std::runtime_error("Unsupported binary operator");
            }
        }

        bool Interpreter::ValuesEqual(const runtime::Value &lhs, const runtime::Value &rhs) const
        {
            return lhs.GetData() == rhs.GetData();
        }

        runtime::Value Interpreter::ApplyUnary(TokenType op, const runtime::Value &rhs) const
        {
            switch (op)
            {
            case TokenType::Minus:
                return runtime::Value(-AsNumber(rhs));
            case TokenType::Plus:
                return runtime::Value(+AsNumber(rhs));
            case TokenType::Not:
                return runtime::Value(!IsTruthy(rhs));
            default:
                throw std::runtime_error("Unsupported unary operator");
            }
        }

        void Interpreter::CheckTypeCompatibility(const std::optional<std::string> &declaredType, const runtime::Value &value, const std::string &name) const
        {
            if (!declaredType.has_value())
            {
                return;
            }

            const std::string &type = declaredType.value();
            if (type == "int" || type == "float")
            {
                if (!value.IsNumber())
                {
                    throw std::runtime_error("Type mismatch for '" + name + "': expected number");
                }
                return;
            }
            if (type == "bool")
            {
                if (!value.IsBool())
                {
                    throw std::runtime_error("Type mismatch for '" + name + "': expected bool");
                }
                return;
            }
            if (type == "string")
            {
                if (!value.IsString())
                {
                    throw std::runtime_error("Type mismatch for '" + name + "': expected string");
                }
                return;
            }
        }

    } // namespace runtime

} // namespace cora::compiler
