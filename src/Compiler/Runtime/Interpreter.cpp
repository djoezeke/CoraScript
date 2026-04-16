#include "Cora/Compiler/Runtime/Interpreter.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace cora
{
    namespace script
    {
        void Interpreter::Run(const std::string &source)
        {
            std::deque<Token> tokens = Lex(source);
            std::deque<Stmt *> program = Parse(tokens);
            Execute(program);
            for (Stmt *stmt : program)
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

        void Interpreter::Execute(const std::deque<Stmt *> &program)
        {
            for (Stmt *stmt : program)
            {
                ExecStmt(stmt);
            }
        }

        Interpreter::Value Interpreter::EvalExpr(Expr *expr)
        {
            if (auto *lit = dynamic_cast<LiteralExpr *>(expr))
            {
                return lit->value;
            }

            if (auto *var = dynamic_cast<VariableExpr *>(expr))
            {
                auto it = m_Symbols.find(var->name);
                if (it == m_Symbols.end())
                {
                    throw std::runtime_error("Undefined variable: " + var->name);
                }
                return it->second.value;
            }

            if (auto *unary = dynamic_cast<UnaryExpr *>(expr))
            {
                Value rhs = EvalExpr(unary->rhs);
                return ApplyUnary(unary->op, rhs);
            }

            if (auto *binary = dynamic_cast<BinaryExpr *>(expr))
            {
                Value lhs = EvalExpr(binary->lhs);
                if (binary->op == TokenType::And)
                {
                    if (!IsTruthy(lhs))
                    {
                        return false;
                    }
                    return IsTruthy(EvalExpr(binary->rhs));
                }
                if (binary->op == TokenType::Or)
                {
                    if (IsTruthy(lhs))
                    {
                        return true;
                    }
                    return IsTruthy(EvalExpr(binary->rhs));
                }

                Value rhs = EvalExpr(binary->rhs);
                return ApplyBinary(binary->op, lhs, rhs);
            }

            throw std::runtime_error("Unknown expression node");
        }

        void Interpreter::ExecStmt(Stmt *stmt)
        {
            struct BreakSignal
            {
            };
            struct ContinueSignal
            {
            };

            if (auto *print = dynamic_cast<PrintStmt *>(stmt))
            {
                bool first = true;
                for (Expr *expr : print->expressions)
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

            if (auto *exprStmt = dynamic_cast<ExprStmt *>(stmt))
            {
                (void)EvalExpr(exprStmt->expr);
                return;
            }

            if (auto *assign = dynamic_cast<AssignStmt *>(stmt))
            {
                auto it = m_Symbols.find(assign->name);
                if (it == m_Symbols.end())
                {
                    throw std::runtime_error("Assignment to undefined variable: " + assign->name);
                }
                Value value = EvalExpr(assign->expr);
                CheckTypeCompatibility(it->second.declaredType, value, assign->name);
                it->second.value = std::move(value);
                return;
            }

            if (auto *decl = dynamic_cast<VarDeclStmt *>(stmt))
            {
                if (m_Symbols.find(decl->name) != m_Symbols.end())
                {
                    throw std::runtime_error("Variable already declared: " + decl->name);
                }
                Value value = EvalExpr(decl->expr);
                CheckTypeCompatibility(decl->declaredType, value, decl->name);
                m_Symbols.emplace(decl->name, Symbol{std::move(value), decl->declaredType});
                return;
            }

            if (auto *block = dynamic_cast<BlockStmt *>(stmt))
            {
                ExecBlock(block);
                return;
            }

            if (auto *ifStmt = dynamic_cast<IfStmt *>(stmt))
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

            if (auto *whileStmt = dynamic_cast<WhileStmt *>(stmt))
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

            if (auto *forRange = dynamic_cast<ForRangeStmt *>(stmt))
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
                    auto it = m_Symbols.find(forRange->name);
                    if (it == m_Symbols.end())
                    {
                        m_Symbols.emplace(forRange->name, Symbol{i, std::nullopt});
                    }
                    else
                    {
                        CheckTypeCompatibility(it->second.declaredType, i, forRange->name);
                        it->second.value = i;
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

            if (auto *forCStyle = dynamic_cast<ForCStyleStmt *>(stmt))
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

            if (dynamic_cast<BreakStmt *>(stmt) != nullptr)
            {
                throw BreakSignal{};
            }

            if (dynamic_cast<ContinueStmt *>(stmt) != nullptr)
            {
                throw ContinueSignal{};
            }

            if (dynamic_cast<PassStmt *>(stmt) != nullptr)
            {
                return;
            }

            throw std::runtime_error("Unknown statement node");
        }

        void Interpreter::ExecBlock(BlockStmt *block)
        {
            for (Stmt *stmt : block->statements)
            {
                ExecStmt(stmt);
            }
        }

        bool Interpreter::IsTruthy(const Value &value) const
        {
            if (std::holds_alternative<std::monostate>(value))
            {
                return false;
            }
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value);
            }
            if (std::holds_alternative<double>(value))
            {
                return std::get<double>(value) != 0.0;
            }
            if (std::holds_alternative<std::string>(value))
            {
                return !std::get<std::string>(value).empty();
            }
            return false;
        }

        double Interpreter::AsNumber(const Value &value) const
        {
            if (!std::holds_alternative<double>(value))
            {
                throw std::runtime_error("Expected numeric value");
            }
            return std::get<double>(value);
        }

        std::string Interpreter::ToString(const Value &value) const
        {
            if (std::holds_alternative<std::monostate>(value))
            {
                return "null";
            }
            if (std::holds_alternative<double>(value))
            {
                std::ostringstream out;
                out << std::get<double>(value);
                return out.str();
            }
            if (std::holds_alternative<bool>(value))
            {
                return std::get<bool>(value) ? "true" : "false";
            }
            return std::get<std::string>(value);
        }

        Interpreter::Value Interpreter::ApplyBinary(TokenType op, const Value &lhs, const Value &rhs) const
        {
            switch (op)
            {
            case TokenType::Plus:
                if (std::holds_alternative<std::string>(lhs) || std::holds_alternative<std::string>(rhs))
                {
                    return ToString(lhs) + ToString(rhs);
                }
                return AsNumber(lhs) + AsNumber(rhs);
            case TokenType::Minus:
                return AsNumber(lhs) - AsNumber(rhs);
            case TokenType::Star:
                return AsNumber(lhs) * AsNumber(rhs);
            case TokenType::Slash:
                if (AsNumber(rhs) == 0.0)
                {
                    throw std::runtime_error("Division by zero");
                }
                return AsNumber(lhs) / AsNumber(rhs);
            case TokenType::Percent:
                if (AsNumber(rhs) == 0.0)
                {
                    throw std::runtime_error("Modulo by zero");
                }
                return std::fmod(AsNumber(lhs), AsNumber(rhs));
            case TokenType::Equal:
                return ValuesEqual(lhs, rhs);
            case TokenType::NotEqual:
                return !ValuesEqual(lhs, rhs);
            case TokenType::Less:
                return AsNumber(lhs) < AsNumber(rhs);
            case TokenType::LessEqual:
                return AsNumber(lhs) <= AsNumber(rhs);
            case TokenType::Greater:
                return AsNumber(lhs) > AsNumber(rhs);
            case TokenType::GreaterEqual:
                return AsNumber(lhs) >= AsNumber(rhs);
            case TokenType::And:
                return IsTruthy(lhs) && IsTruthy(rhs);
            case TokenType::Or:
                return IsTruthy(lhs) || IsTruthy(rhs);
            default:
                throw std::runtime_error("Unsupported binary operator");
            }
        }

        bool Interpreter::ValuesEqual(const Value &lhs, const Value &rhs) const
        {
            if (lhs.index() == rhs.index())
            {
                if (std::holds_alternative<std::monostate>(lhs))
                {
                    return true;
                }
                if (std::holds_alternative<double>(lhs))
                {
                    return std::get<double>(lhs) == std::get<double>(rhs);
                }
                if (std::holds_alternative<bool>(lhs))
                {
                    return std::get<bool>(lhs) == std::get<bool>(rhs);
                }
                if (std::holds_alternative<std::string>(lhs))
                {
                    return std::get<std::string>(lhs) == std::get<std::string>(rhs);
                }
            }

            if (std::holds_alternative<double>(lhs) && std::holds_alternative<bool>(rhs))
            {
                return std::get<double>(lhs) == (std::get<bool>(rhs) ? 1.0 : 0.0);
            }
            if (std::holds_alternative<bool>(lhs) && std::holds_alternative<double>(rhs))
            {
                return (std::get<bool>(lhs) ? 1.0 : 0.0) == std::get<double>(rhs);
            }

            return ToString(lhs) == ToString(rhs);
        }

        Interpreter::Value Interpreter::ApplyUnary(TokenType op, const Value &rhs) const
        {
            switch (op)
            {
            case TokenType::Minus:
                return -AsNumber(rhs);
            case TokenType::Plus:
                return +AsNumber(rhs);
            case TokenType::Not:
                return !IsTruthy(rhs);
            default:
                throw std::runtime_error("Unsupported unary operator");
            }
        }

        void Interpreter::CheckTypeCompatibility(const std::optional<std::string> &declaredType, const Value &value, const std::string &name) const
        {
            if (!declaredType.has_value())
            {
                return;
            }

            const std::string &type = declaredType.value();
            if (type == "int" || type == "float")
            {
                if (!std::holds_alternative<double>(value))
                {
                    throw std::runtime_error("Type mismatch for '" + name + "': expected number");
                }
                return;
            }
            if (type == "bool")
            {
                if (!std::holds_alternative<bool>(value))
                {
                    throw std::runtime_error("Type mismatch for '" + name + "': expected bool");
                }
                return;
            }
            if (type == "string")
            {
                if (!std::holds_alternative<std::string>(value))
                {
                    throw std::runtime_error("Type mismatch for '" + name + "': expected string");
                }
                return;
            }
        }
    }
}
