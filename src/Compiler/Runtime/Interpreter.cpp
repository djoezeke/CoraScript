#include "Cora/Compiler/Runtime/Interpreter.hpp"

#include "Cora/Compiler/Builtin/Builtin.hpp"
#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"
#include "Cora/Compiler/Runtime/Variable.hpp"

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

            struct ReturnSignal
            {
                Value value;
            };

            static std::shared_ptr<NativeFunction> MakeNative(const std::string &name, NativeFunction::Fn fn)
            {
                return std::make_shared<NativeFunction>(name, std::move(fn));
            }
        }

        Interpreter::Interpreter()
        {
            builtin::RegisterBuiltinFunctions(m_GlobalScope);
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

        void Interpreter::InvokeConstructor(const std::shared_ptr<Object> &object, const std::vector<Value> &arguments)
        {
            if (!object)
            {
                return;
            }

            auto initIt = object->fields.find("__init__");
            if (initIt == object->fields.end() || !initIt->second.IsCallable())
            {
                return;
            }

            auto initCallable = initIt->second.AsCallable();
            if (initCallable)
            {
                std::vector<Value> methodArgs;
                methodArgs.reserve(arguments.size() + 1);
                methodArgs.emplace_back(object);
                methodArgs.insert(methodArgs.end(), arguments.begin(), arguments.end());
                (void)initCallable->Call(methodArgs);
            }
        }

        void Interpreter::InvokeDestructor(const Value &value)
        {
            if (!value.IsObject())
            {
                return;
            }

            auto object = value.AsObject();
            if (!object)
            {
                return;
            }

            auto delIt = object->fields.find("__del__");
            if (delIt == object->fields.end() || !delIt->second.IsCallable())
            {
                return;
            }

            auto delCallable = delIt->second.AsCallable();
            if (delCallable)
            {
                (void)delCallable->Call({Value(object)});
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

            if (auto *member = dynamic_cast<ast::MemberExpr *>(expr))
            {
                Value objectValue = EvalExpr(member->GetObject());
                auto object = objectValue.AsObject();
                if (!object)
                {
                    throw std::runtime_error("Member access on non-object value");
                }

                auto field = object->fields.find(member->GetMember());
                if (field != object->fields.end())
                {
                    if (field->second.IsCallable())
                    {
                        auto callable = field->second.AsCallable();
                        auto method = MakeNative(member->GetMember(), [object, callable](const std::vector<Value> &arguments) -> Value
                                                 {
                            std::vector<Value> methodArgs;
                            methodArgs.reserve(arguments.size() + 1);
                            methodArgs.emplace_back(object);
                            methodArgs.insert(methodArgs.end(), arguments.begin(), arguments.end());
                            return callable->Call(methodArgs); });
                        return Value(std::static_pointer_cast<Callable>(method));
                    }
                    return field->second;
                }

                if (member->GetMember() == "type")
                {
                    return Value(object->className);
                }

                if (member->GetMember() == "toString")
                {
                    auto method = MakeNative("toString", [object](const std::vector<Value> &) -> Value
                                             { return Value("<object " + object->className + ">"); });
                    return Value(std::static_pointer_cast<Callable>(method));
                }

                if (member->GetMember() == "get")
                {
                    auto method = MakeNative("get", [object](const std::vector<Value> &arguments) -> Value
                                             {
                        if (arguments.empty())
                        {
                            return Value(nullptr);
                        }
                        const std::string key = arguments.front().AsString();
                        auto it = object->fields.find(key);
                        if (it == object->fields.end())
                        {
                            return Value(nullptr);
                        }
                        return it->second; });
                    return Value(std::static_pointer_cast<Callable>(method));
                }

                if (member->GetMember() == "set")
                {
                    auto method = MakeNative("set", [object](const std::vector<Value> &arguments) -> Value
                                             {
                        if (arguments.size() < 2)
                        {
                            return Value(nullptr);
                        }
                        const std::string key = arguments[0].AsString();
                        object->fields[key] = arguments[1];
                        return arguments[1]; });
                    return Value(std::static_pointer_cast<Callable>(method));
                }

                if (member->GetMember() == "keys")
                {
                    auto method = MakeNative("keys", [object](const std::vector<Value> &) -> Value
                                             {
                        std::string result;
                        bool first = true;
                        for (const auto &entry : object->fields)
                        {
                            if (!first)
                            {
                                result += ",";
                            }
                            result += entry.first;
                            first = false;
                        }
                        return Value(result); });
                    return Value(std::static_pointer_cast<Callable>(method));
                }

                throw std::runtime_error("Unknown object member: " + member->GetMember());
            }

            if (auto *call = dynamic_cast<ast::CallExpr *>(expr))
            {
                Value callee = EvalExpr(call->GetCallee());
                auto callable = callee.AsCallable();
                if (!callable)
                {
                    throw std::runtime_error("Attempted to call a non-callable value");
                }

                std::vector<Value> arguments;
                arguments.reserve(call->GetArguments().size());
                for (Expression *argument : call->GetArguments())
                {
                    arguments.push_back(EvalExpr(argument));
                }
                return callable->Call(arguments);
            }

            if (auto *newExpr = dynamic_cast<ast::NewExpr *>(expr))
            {
                std::vector<Value> arguments;
                arguments.reserve(newExpr->GetArguments().size());
                for (Expression *argument : newExpr->GetArguments())
                {
                    arguments.push_back(EvalExpr(argument));
                }

                auto object = std::make_shared<Object>(newExpr->GetClassName());
                Value instance(object);

                Scope *scope = m_CurrentScope->ResolveVariable(newExpr->GetClassName());
                if (scope != nullptr)
                {
                    Variable *variable = scope->GetVariable(newExpr->GetClassName());
                    if (variable != nullptr && variable->GetValue() != nullptr)
                    {
                        if (variable->GetValue()->IsObject())
                        {
                            auto prototype = variable->GetValue()->AsObject();
                            if (prototype)
                            {
                                object->fields = prototype->fields;
                                InvokeConstructor(object, arguments);
                                return instance;
                            }
                        }

                        if (variable->GetValue()->IsCallable())
                        {
                            auto callable = variable->GetValue()->AsCallable();
                            Value constructed = callable->Call(arguments);
                            if (constructed.IsObject())
                            {
                                InvokeConstructor(constructed.AsObject(), arguments);
                                return constructed;
                            }
                            return constructed;
                        }
                    }
                }

                InvokeConstructor(object, arguments);

                return instance;
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

            if (auto *returnStmt = dynamic_cast<ast::ReturnStmt *>(stmt))
            {
                Value value = returnStmt->GetValue() != nullptr ? EvalExpr(returnStmt->GetValue()) : Value(nullptr);
                throw ReturnSignal{value};
            }

            if (auto *functionDecl = dynamic_cast<ast::FunctionDeclStmt *>(stmt))
            {
                const std::string functionName = functionDecl->GetName();
                const std::deque<std::string> parameters = functionDecl->GetParameters();
                ast::BlockStmt *body = functionDecl->GetBody();
                Scope *closure = m_CurrentScope;

                auto callable = MakeNative(functionName, [this, functionName, parameters, body, closure](const std::vector<Value> &arguments) -> Value
                                           {
                    FunctionScope functionScope(functionName, closure);

                    for (std::size_t i = 0; i < parameters.size(); ++i)
                    {
                        Value argumentValue = i < arguments.size() ? arguments[i] : Value(nullptr);
                        functionScope.NewVariableValue(parameters[i], new Value(argumentValue), false);
                    }

                    Scope *previousScope = m_CurrentScope;
                    m_CurrentScope = &functionScope;

                    try
                    {
                        for (Statement *statement : body->statements)
                        {
                            ExecStmt(statement);
                        }
                    }
                    catch (const ReturnSignal &signal)
                    {
                        m_CurrentScope = previousScope;
                        return signal.value;
                    }
                    catch (...)
                    {
                        m_CurrentScope = previousScope;
                        throw;
                    }

                    m_CurrentScope = previousScope;
                    return Value(nullptr); });

                m_CurrentScope->SetVariableValue(functionName, new Value(std::static_pointer_cast<Callable>(callable)), false);
                return;
            }

            if (auto *classDecl = dynamic_cast<ast::ClassDeclStmt *>(stmt))
            {
                auto klass = std::make_shared<Object>(classDecl->GetName());

                for (ast::FunctionDeclStmt *method : classDecl->GetMethods())
                {
                    const std::string methodName = method->GetName();
                    const std::deque<std::string> parameters = method->GetParameters();
                    ast::BlockStmt *body = method->GetBody();
                    Scope *closure = m_CurrentScope;

                    auto callable = MakeNative(methodName, [this, methodName, parameters, body, closure](const std::vector<Value> &arguments) -> Value
                                               {
                        FunctionScope functionScope(methodName, closure);

                        std::size_t argumentOffset = 0;
                        if (!arguments.empty() && arguments[0].IsObject())
                        {
                            functionScope.NewVariableValue("this", new Value(arguments[0]), true);
                            argumentOffset = 1;
                        }

                        for (std::size_t i = 0; i < parameters.size(); ++i)
                        {
                            const std::size_t argIndex = argumentOffset + i;
                            Value argumentValue = argIndex < arguments.size() ? arguments[argIndex] : Value(nullptr);
                            functionScope.NewVariableValue(parameters[i], new Value(argumentValue), false);
                        }

                        Scope *previousScope = m_CurrentScope;
                        m_CurrentScope = &functionScope;

                        try
                        {
                            for (Statement *statement : body->statements)
                            {
                                ExecStmt(statement);
                            }
                        }
                        catch (const ReturnSignal &signal)
                        {
                            m_CurrentScope = previousScope;
                            return signal.value;
                        }
                        catch (...)
                        {
                            m_CurrentScope = previousScope;
                            throw;
                        }

                        m_CurrentScope = previousScope;
                        return Value(nullptr); });

                    klass->fields[methodName] = Value(std::static_pointer_cast<Callable>(callable));
                }

                m_CurrentScope->SetVariableValue(classDecl->GetName(), new Value(klass), true);
                return;
            }

            if (auto *deleteStmt = dynamic_cast<ast::DeleteStmt *>(stmt))
            {
                Expression *target = deleteStmt->GetTarget();

                if (auto *variableExpr = dynamic_cast<ast::VariableExpr *>(target))
                {
                    Scope *ownerScope = m_CurrentScope->ResolveVariable(variableExpr->GetName());
                    if (ownerScope == nullptr)
                    {
                        throw std::runtime_error("Delete of undefined variable: " + variableExpr->GetName());
                    }

                    Variable *variable = ownerScope->GetVariable(variableExpr->GetName());
                    if (variable != nullptr && variable->GetValue() != nullptr)
                    {
                        InvokeDestructor(*(variable->GetValue()));
                    }

                    if (!ownerScope->DeleteVariable(variableExpr->GetName()))
                    {
                        throw std::runtime_error("Delete failed for variable: " + variableExpr->GetName());
                    }
                    return;
                }

                if (auto *memberExpr = dynamic_cast<ast::MemberExpr *>(target))
                {
                    Value objectValue = EvalExpr(memberExpr->GetObject());
                    auto object = objectValue.AsObject();
                    if (!object)
                    {
                        throw std::runtime_error("Delete member on non-object value");
                    }

                    auto fieldIt = object->fields.find(memberExpr->GetMember());
                    if (fieldIt != object->fields.end())
                    {
                        InvokeDestructor(fieldIt->second);
                        object->fields.erase(fieldIt);
                    }
                    return;
                }

                throw std::runtime_error("Unsupported delete target");
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
