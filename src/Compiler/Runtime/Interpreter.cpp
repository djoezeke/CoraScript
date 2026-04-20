#include "Cora/Compiler/Runtime/Interpreter.hpp"

#include "Cora/Compiler/Builtin/Builtin.hpp"
#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"
#include "Cora/Compiler/Runtime/Variable.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

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

            class OverloadSetCallable final : public Callable
            {
            public:
                explicit OverloadSetCallable(std::string name)
                    : m_Name(std::move(name)) {}

                void Add(std::shared_ptr<Callable> callable)
                {
                    if (!callable)
                    {
                        return;
                    }

                    const int arity = callable->Arity();
                    if (arity >= 0)
                    {
                        m_ByArity[arity] = callable;
                        return;
                    }

                    m_Fallback = callable;
                }

                Value Call(const std::vector<Value> &arguments) override
                {
                    const auto exact = m_ByArity.find(static_cast<int>(arguments.size()));
                    if (exact != m_ByArity.end() && exact->second)
                    {
                        return exact->second->Call(arguments);
                    }

                    if (m_Fallback)
                    {
                        return m_Fallback->Call(arguments);
                    }

                    throw std::runtime_error("No overload for '" + m_Name + "' with " + std::to_string(arguments.size()) + " arguments");
                }

                std::string Name() const override
                {
                    return m_Name;
                }

            private:
                std::string m_Name;
                std::unordered_map<int, std::shared_ptr<Callable>> m_ByArity;
                std::shared_ptr<Callable> m_Fallback;
            };

            static std::shared_ptr<Function> MakeNative(const std::string &name, Function::Func fn, int arity = -1)
            {
                return std::make_shared<Function>(name, std::move(fn), arity);
            }

            static std::shared_ptr<Callable> MergeCallable(std::shared_ptr<Callable> existing, std::shared_ptr<Callable> added, const std::string &name)
            {
                if (!existing)
                {
                    return added;
                }

                auto overloadSet = std::dynamic_pointer_cast<OverloadSetCallable>(existing);
                if (!overloadSet)
                {
                    overloadSet = std::make_shared<OverloadSetCallable>(name);
                    overloadSet->Add(existing);
                }

                overloadSet->Add(added);
                return std::static_pointer_cast<Callable>(overloadSet);
            }
        } // namespace

        Interpreter::Interpreter()
        {
            RegisterBuiltins();
        }

        void Interpreter::Run(const std::string &source)
        {
            parser::Parser parser;
            parser.SetFileName(m_FileName);
            parser.SetModuleName(m_ModuleName);
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

            std::filesystem::path sourcePath(path);
            m_FileName = sourcePath.string();
            m_ModuleName = sourcePath.stem().string();
            m_WorkingDirectory = sourcePath.parent_path();

            std::stringstream buffer;
            buffer << input.rdbuf();
            Run(buffer.str());
        }

        void Interpreter::SetFileName(std::string fileName)
        {
            m_FileName = std::move(fileName);
            const std::filesystem::path path(m_FileName);
            if (!m_FileName.empty())
            {
                m_ModuleName = path.stem().string();
                if (path.has_parent_path())
                {
                    m_WorkingDirectory = path.parent_path();
                }
            }
        }

        void Interpreter::RegisterBuiltins()
        {
            builtin::Builtins(m_GlobalScope);
        }

        void Interpreter::Execute(const std::deque<Statement *> &program)
        {
            for (Statement *stmt : program)
            {
                ExecStmt(stmt);
            }
        }

        Scope *Interpreter::PushTransientScope(ScopeKind kind, Scope *parent)
        {
            return PushTransientScope(std::make_unique<Scope>(parent ? parent : m_CurrentScope, kind));
        }

        Scope *Interpreter::PushTransientScope(std::unique_ptr<Scope> scope)
        {
            if (!scope)
            {
                return m_CurrentScope;
            }

            Scope *raw = scope.get();
            m_TransientScopes.push_back(std::move(scope));
            m_CurrentScope = raw;
            return raw;
        }

        void Interpreter::PopTransientScope()
        {
            if (m_TransientScopes.empty())
            {
                return;
            }

            Scope *parent = m_TransientScopes.back()->GetParent();
            m_TransientScopes.pop_back();
            m_CurrentScope = parent ? parent : &m_GlobalScope;
        }

        error::DiagnosticContext Interpreter::MakeContext(unsigned int line, unsigned int column) const
        {
            error::DiagnosticContext context;
            context.fileName = m_FileName;
            context.moduleName = m_ModuleName;
            if (!m_NamespaceStack.empty())
            {
                context.namespaceName = m_NamespaceStack.back();
            }
            if (!m_ClassStack.empty())
            {
                context.className = m_ClassStack.back();
            }
            if (!m_FunctionStack.empty())
            {
                context.functionName = m_FunctionStack.back();
            }
            context.line = line;
            context.column = column;
            return context;
        }

        [[noreturn]] void Interpreter::RaiseRuntimeError(const std::string &message, unsigned int line, unsigned int column) const
        {
            throw error::RuntimeError(message, MakeContext(line, column));
        }

        std::shared_ptr<Object> Interpreter::LoadModuleFromFile(const std::string &modulePath, const std::filesystem::path &path)
        {
            std::ifstream input(path);
            if (!input)
            {
                return nullptr;
            }

            std::stringstream buffer;
            buffer << input.rdbuf();

            parser::Parser parser;
            parser.SetFileName(path.string());
            parser.SetModuleName(modulePath);
            std::deque<Statement *> program = parser.ParseProgram(buffer.str());

            ModuleScope moduleScope(modulePath, &m_GlobalScope);
            Scope *previous = m_CurrentScope;
            m_CurrentScope = &moduleScope;

            try
            {
                Execute(program);
            }
            catch (...)
            {
                m_CurrentScope = previous;
                for (Statement *stmt : program)
                {
                    delete stmt;
                }
                throw;
            }

            m_CurrentScope = previous;

            auto moduleObject = std::make_shared<Object>(modulePath);
            for (const auto &entry : moduleScope.GetVariables())
            {
                if (entry.second == nullptr || entry.second->GetValue() == nullptr)
                {
                    continue;
                }

                moduleObject->fields[entry.first] = *(entry.second->GetValue());
            }

            for (Statement *stmt : program)
            {
                delete stmt;
            }

            return moduleObject;
        }

        std::shared_ptr<Object> Interpreter::ImportModule(const std::string &modulePath)
        {
            auto cached = m_ModuleCache.find(modulePath);
            if (cached != m_ModuleCache.end())
            {
                return cached->second;
            }

            const std::size_t dotPosition = modulePath.find('.');
            const std::string rootName = modulePath.substr(0, dotPosition == std::string::npos ? modulePath.size() : dotPosition);

            Scope *builtinScope = m_GlobalScope.ResolveVariable(rootName);
            if (builtinScope != nullptr)
            {
                Variable *variable = builtinScope->GetVariable(rootName);
                if (variable != nullptr && variable->GetValue() != nullptr && variable->GetValue()->IsObject())
                {
                    auto object = variable->GetValue()->AsObject();
                    std::shared_ptr<Object> current = object;
                    if (dotPosition != std::string::npos)
                    {
                        std::size_t segmentStart = dotPosition + 1;
                        std::size_t segmentDot = modulePath.find('.', segmentStart);
                        while (segmentStart <= modulePath.size())
                        {
                            const std::string segment = modulePath.substr(segmentStart, segmentDot == std::string::npos ? std::string::npos : segmentDot - segmentStart);
                            if (segment.empty())
                            {
                                break;
                            }

                            auto member = current->fields.find(segment);
                            if (member == current->fields.end() || !member->second.IsObject())
                            {
                                RaiseRuntimeError("Unknown module path: " + modulePath);
                            }

                            current = member->second.AsObject();
                            if (segmentDot == std::string::npos)
                            {
                                break;
                            }
                            segmentStart = segmentDot + 1;
                            segmentDot = modulePath.find('.', segmentStart);
                        }
                    }

                    m_ModuleCache[modulePath] = current;
                    return current;
                }
            }

            std::string moduleFile = modulePath;
            std::replace(moduleFile.begin(), moduleFile.end(), '.', '/');
            std::filesystem::path relativePath = moduleFile + ".cora";

            const std::filesystem::path searchRoot = m_WorkingDirectory.empty() ? std::filesystem::current_path() : m_WorkingDirectory;
            const std::filesystem::path sourcePath = searchRoot / relativePath;

            auto moduleObject = LoadModuleFromFile(modulePath, sourcePath);
            if (!moduleObject)
            {
                RaiseRuntimeError("Unable to import module '" + modulePath + "' from builtins or current directory");
            }

            m_ModuleCache[modulePath] = moduleObject;
            m_GlobalScope.SetVariableValue(rootName, new Value(moduleObject), true);
            return moduleObject;
        }

        std::shared_ptr<Callable> Interpreter::FindBestMethodOverload(const std::shared_ptr<Object> &object, const std::string &methodName, std::size_t) const
        {
            if (!object)
            {
                return nullptr;
            }

            auto method = object->fields.find(methodName);
            if (method == object->fields.end() || !method->second.IsCallable())
            {
                return nullptr;
            }

            return method->second.AsCallable();
        }

        bool Interpreter::CanAccessMember(const std::shared_ptr<Object> &) const
        {
            return true;
        }

        Value Interpreter::ResolveMemberValue(const std::shared_ptr<Object> &object, const std::string &memberName)
        {
            if (!object)
            {
                RaiseRuntimeError("Member access on null object");
            }

            auto member = object->fields.find(memberName);
            if (member == object->fields.end())
            {
                RaiseRuntimeError("Unknown object member: " + memberName);
            }

            if (member->second.IsCallable())
            {
                auto callable = member->second.AsCallable();
                auto bound = MakeNative(memberName, [object, callable](const std::vector<Value> &arguments) -> Value
                                        {
                                            if (!callable)
                                            {
                                                throw std::runtime_error("Null callable member");
                                            }

                                            const int arity = callable->Arity();
                                            if (arity >= 0 && arity == static_cast<int>(arguments.size()) + 1)
                                            {
                                                std::vector<Value> methodArguments;
                                                methodArguments.reserve(arguments.size() + 1);
                                                methodArguments.emplace_back(object);
                                                methodArguments.insert(methodArguments.end(), arguments.begin(), arguments.end());
                                                return callable->Call(methodArguments);
                                            }

                                            return callable->Call(arguments); }, callable ? callable->Arity() : -1);
                return Value(std::static_pointer_cast<Callable>(bound));
            }

            return member->second;
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
                    RaiseRuntimeError("Undefined variable: " + var->GetName());
                }

                Variable *variable = scope->GetVariable(var->GetName());
                if (variable == nullptr || variable->GetValue() == nullptr)
                {
                    RaiseRuntimeError("Undefined variable: " + var->GetName());
                }

                return *(variable->GetValue());
            }

            if (auto *member = dynamic_cast<ast::MemberExpr *>(expr))
            {
                Value objectValue = EvalExpr(member->GetObject());
                auto object = objectValue.AsObject();
                if (!object)
                {
                    RaiseRuntimeError("Member access on non-object value");
                }
                if (!CanAccessMember(object))
                {
                    RaiseRuntimeError("Member is not accessible: " + member->GetMember());
                }
                return ResolveMemberValue(object, member->GetMember());
            }

            if (auto *call = dynamic_cast<ast::CallExpr *>(expr))
            {
                Value callee = EvalExpr(call->GetCallee());

                std::vector<Value> arguments;
                arguments.reserve(call->GetArguments().size());
                for (Expression *argument : call->GetArguments())
                {
                    arguments.push_back(EvalExpr(argument));
                }

                if (callee.IsCallable())
                {
                    auto callable = callee.AsCallable();
                    if (!callable)
                    {
                        RaiseRuntimeError("Attempted to call a null callable");
                    }
                    return callable->Call(arguments);
                }

                if (callee.IsObject())
                {
                    auto prototype = callee.AsObject();
                    auto instance = std::make_shared<Object>(prototype ? prototype->className : "Object");
                    if (prototype)
                    {
                        instance->fields = prototype->fields;
                    }
                    InvokeConstructor(instance, arguments);
                    return Value(instance);
                }

                RaiseRuntimeError("Attempted to call a non-callable value");
            }

            if (auto *newExpr = dynamic_cast<ast::NewExpr *>(expr))
            {
                std::vector<Value> arguments;
                arguments.reserve(newExpr->GetArguments().size());
                for (Expression *argument : newExpr->GetArguments())
                {
                    arguments.push_back(EvalExpr(argument));
                }

                Scope *scope = m_CurrentScope->ResolveVariable(newExpr->GetClassName());
                std::shared_ptr<Object> prototype;
                if (scope != nullptr)
                {
                    Variable *variable = scope->GetVariable(newExpr->GetClassName());
                    if (variable != nullptr && variable->GetValue() != nullptr && variable->GetValue()->IsObject())
                    {
                        prototype = variable->GetValue()->AsObject();
                    }
                }

                auto instance = std::make_shared<Object>(prototype ? prototype->className : newExpr->GetClassName());
                if (prototype)
                {
                    instance->fields = prototype->fields;
                }
                InvokeConstructor(instance, arguments);
                return Value(instance);
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

            RaiseRuntimeError("Unknown expression node");
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
            if (!initCallable)
            {
                return;
            }

            std::vector<Value> methodArgs;
            methodArgs.reserve(arguments.size() + 1);
            methodArgs.emplace_back(object);
            methodArgs.insert(methodArgs.end(), arguments.begin(), arguments.end());
            (void)initCallable->Call(methodArgs);
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

            if (auto *assign = dynamic_cast<ast::Assignment *>(stmt))
            {
                Value value = EvalExpr(assign->GetValue());

                if (auto *variableTarget = dynamic_cast<ast::VariableExpr *>(assign->GetTarget()))
                {
                    Scope *scope = m_CurrentScope->ResolveVariable(variableTarget->GetName());
                    if (scope == nullptr)
                    {
                        m_CurrentScope->NewVariableValue(variableTarget->GetName(), new Value(value), false);
                        return;
                    }

                    Variable *variable = scope->GetVariable(variableTarget->GetName());
                    if (variable == nullptr)
                    {
                        scope->SetVariableValue(variableTarget->GetName(), new Value(value), false);
                        return;
                    }

                    variable->SetValue(new Value(value));
                    return;
                }

                if (auto *memberTarget = dynamic_cast<ast::MemberExpr *>(assign->GetTarget()))
                {
                    Value objectValue = EvalExpr(memberTarget->GetObject());
                    auto object = objectValue.AsObject();
                    if (!object)
                    {
                        RaiseRuntimeError("Member assignment on non-object value");
                    }

                    object->fields[memberTarget->GetMember()] = value;
                    return;
                }

                RaiseRuntimeError("Invalid assignment target");
            }

            if (auto *assignStmt = dynamic_cast<ast::AssignStmt *>(stmt))
            {
                Value value = EvalExpr(assignStmt->GetValue());

                if (auto *variableTarget = dynamic_cast<ast::VariableExpr *>(assignStmt->GetTarget()))
                {
                    Scope *scope = m_CurrentScope->ResolveVariable(variableTarget->GetName());
                    if (scope == nullptr)
                    {
                        m_CurrentScope->NewVariableValue(variableTarget->GetName(), new Value(value), false);
                        return;
                    }

                    Variable *variable = scope->GetVariable(variableTarget->GetName());
                    if (variable == nullptr)
                    {
                        scope->SetVariableValue(variableTarget->GetName(), new Value(value), false);
                        return;
                    }

                    variable->SetValue(new Value(value));
                    return;
                }

                if (auto *memberTarget = dynamic_cast<ast::MemberExpr *>(assignStmt->GetTarget()))
                {
                    Value objectValue = EvalExpr(memberTarget->GetObject());
                    auto object = objectValue.AsObject();
                    if (!object)
                    {
                        RaiseRuntimeError("Member assignment on non-object value");
                    }

                    object->fields[memberTarget->GetMember()] = value;
                    return;
                }

                RaiseRuntimeError("Invalid assignment target");
            }

            if (auto *decl = dynamic_cast<ast::VarDeclaration *>(stmt))
            {
                Value value = EvalExpr(decl->GetExpression());
                CheckTypeCompatibility(decl->GetType(), decl->GetName(), value);
                m_CurrentScope->SetVariableValue(decl->GetName(), new Value(value), false);
                return;
            }

            if (auto *decl = dynamic_cast<ast::VarDeclStmt *>(stmt))
            {
                Value value = EvalExpr(decl->GetExpression());
                if (decl->GetDeclaredType().has_value())
                {
                    CheckTypeCompatibility(*(decl->GetDeclaredType()), decl->GetName(), value);
                }
                m_CurrentScope->SetVariableValue(decl->GetName(), new Value(value), false);
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

            if (auto *namespaceStmt = dynamic_cast<ast::NamespaceDeclStmt *>(stmt))
            {
                std::string qualified = namespaceStmt->GetName();
                if (!m_NamespaceStack.empty())
                {
                    qualified = m_NamespaceStack.back() + "::" + qualified;
                }

                m_NamespaceStack.push_back(qualified);
                ExecBlock(namespaceStmt->GetBody());
                m_NamespaceStack.pop_back();
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
                    RaiseRuntimeError("range step cannot be 0");
                }

                for (double i = start; step > 0.0 ? i < end : i > end; i += step)
                {
                    Scope *scope = m_CurrentScope->ResolveVariable(forRange->name);
                    if (scope == nullptr)
                    {
                        m_CurrentScope->NewVariableValue(forRange->name, new Value(i), false);
                    }
                    else
                    {
                        Variable *var = scope->GetVariable(forRange->name);
                        if (var)
                        {
                            var->SetValue(new Value(i));
                        }
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

            if (auto *importStmt = dynamic_cast<ast::ImportStmt *>(stmt))
            {
                const std::string modulePath = importStmt->GetModuleName();
                if (modulePath.empty())
                {
                    return;
                }

                auto moduleObject = ImportModule(modulePath);

                const std::size_t dotPosition = modulePath.find('.');
                const std::string rootName = modulePath.substr(0, dotPosition == std::string::npos ? modulePath.size() : dotPosition);

                auto setIfAllowed = [&](const std::string &name, const Value &value, bool constant)
                {
                    Scope *owner = m_CurrentScope->ResolveVariable(name);
                    if (owner != nullptr)
                    {
                        Variable *existing = owner->GetVariable(name);
                        if (existing != nullptr && existing->IsConst())
                        {
                            return;
                        }
                    }

                    m_CurrentScope->SetVariableValue(name, new Value(value), constant);
                };

                setIfAllowed(rootName, Value(moduleObject), true);

                if (moduleObject)
                {
                    for (const auto &entry : moduleObject->fields)
                    {
                        if (entry.first.rfind("__", 0) == 0)
                        {
                            continue;
                        }

                        setIfAllowed(entry.first, entry.second, false);
                    }
                }

                return;
            }

            if (auto *functionDecl = dynamic_cast<ast::FunctionDeclStmt *>(stmt))
            {
                const std::string functionName = functionDecl->GetName();
                const std::deque<std::string> parameters = functionDecl->GetParameters();
                ast::BlockStmt *body = functionDecl->GetBody();
                const std::optional<std::string> returnType = functionDecl->GetReturnType();
                Scope *closure = m_CurrentScope;

                auto callable = MakeNative(functionName, [this, functionName, parameters, body, closure, returnType](const std::vector<Value> &arguments) -> Value
                                           {
                                               FunctionScope functionScope(functionName, closure);

                                               for (std::size_t i = 0; i < parameters.size(); ++i)
                                               {
                                                   Value argumentValue = i < arguments.size() ? arguments[i] : Value(nullptr);
                                                   functionScope.NewVariableValue(parameters[i], new Value(argumentValue), false);
                                               }

                                               Scope *previousScope = m_CurrentScope;
                                               m_CurrentScope = &functionScope;
                                               m_FunctionStack.push_back(functionName);

                                               try
                                               {
                                                   for (Statement *statement : body->statements)
                                                   {
                                                       ExecStmt(statement);
                                                   }
                                               }
                                               catch (const ReturnSignal &signal)
                                               {
                                                   m_FunctionStack.pop_back();
                                                   m_CurrentScope = previousScope;
                                                   if (returnType.has_value())
                                                   {
                                                       CheckTypeCompatibility(*returnType, functionName, signal.value);
                                                   }
                                                   return signal.value;
                                               }
                                               catch (...)
                                               {
                                                   m_FunctionStack.pop_back();
                                                   m_CurrentScope = previousScope;
                                                   throw;
                                               }

                                               m_FunctionStack.pop_back();
                                               m_CurrentScope = previousScope;
                                               Value none(nullptr);
                                               if (returnType.has_value())
                                               {
                                                   CheckTypeCompatibility(*returnType, functionName, none);
                                               }
                                               return none; }, static_cast<int>(parameters.size()));

                Scope *resolved = m_CurrentScope->ResolveVariable(functionName);
                if (resolved != nullptr)
                {
                    Variable *existing = resolved->GetVariable(functionName);
                    if (existing != nullptr && existing->GetValue() != nullptr && existing->GetValue()->IsCallable())
                    {
                        auto merged = MergeCallable(existing->GetValue()->AsCallable(), std::static_pointer_cast<Callable>(callable), functionName);
                        resolved->SetVariableValue(functionName, new Value(merged), false);
                        return;
                    }
                }

                m_CurrentScope->SetVariableValue(functionName, new Value(std::static_pointer_cast<Callable>(callable)), false);
                return;
            }

            if (auto *classDecl = dynamic_cast<ast::ClassDeclStmt *>(stmt))
            {
                auto klass = std::make_shared<Object>(classDecl->GetName());
                m_ClassStack.push_back(classDecl->GetName());

                klass->fields["__repr__"] = Value(std::static_pointer_cast<Callable>(MakeNative("__repr__", [](const std::vector<Value> &arguments) -> Value
                                                                                                {
                                                                                                      if (arguments.empty() || !arguments.front().IsObject())
                                                                                                      {
                                                                                                          return Value("<object>");
                                                                                                      }

                                                                                                      auto self = arguments.front().AsObject();
                                                                                                      return Value("<" + self->className + " object>"); }, 1)));

                klass->fields["__str__"] = Value(std::static_pointer_cast<Callable>(MakeNative("__str__", [](const std::vector<Value> &arguments) -> Value
                                                                                               {
                                                                                                    if (arguments.empty() || !arguments.front().IsObject())
                                                                                                    {
                                                                                                        return Value("<object>");
                                                                                                    }

                                                                                                    auto self = arguments.front().AsObject();
                                                                                                    auto reprIt = self->fields.find("__repr__");
                                                                                                    if (reprIt != self->fields.end() && reprIt->second.IsCallable())
                                                                                                    {
                                                                                                        auto callable = reprIt->second.AsCallable();
                                                                                                        if (callable)
                                                                                                        {
                                                                                                            return callable->Call({Value(self)});
                                                                                                        }
                                                                                                    }

                                                                                                    return Value("<" + self->className + " object>"); }, 1)));

                klass->fields["__dir__"] = Value(std::static_pointer_cast<Callable>(MakeNative("__dir__", [](const std::vector<Value> &arguments) -> Value
                                                                                               {
                                                                                                    if (arguments.empty() || !arguments.front().IsObject())
                                                                                                    {
                                                                                                        return Value("");
                                                                                                    }

                                                                                                    auto self = arguments.front().AsObject();
                                                                                                    std::vector<std::string> names;
                                                                                                    names.reserve(self->fields.size());
                                                                                                    for (const auto &entry : self->fields)
                                                                                                    {
                                                                                                        names.push_back(entry.first);
                                                                                                    }

                                                                                                    std::sort(names.begin(), names.end());
                                                                                                    std::ostringstream out;
                                                                                                    for (std::size_t i = 0; i < names.size(); ++i)
                                                                                                    {
                                                                                                        if (i != 0)
                                                                                                        {
                                                                                                            out << ",";
                                                                                                        }
                                                                                                        out << names[i];
                                                                                                    }

                                                                                                    return Value(out.str()); }, 1)));

                for (ast::FunctionDeclStmt *method : classDecl->GetMethods())
                {
                    const std::string methodName = method->GetName();
                    const std::deque<std::string> parameters = method->GetParameters();
                    ast::BlockStmt *body = method->GetBody();
                    const std::optional<std::string> returnType = method->GetReturnType();
                    Scope *closure = m_CurrentScope;

                    auto callable = MakeNative(methodName, [this, methodName, parameters, body, closure, returnType](const std::vector<Value> &arguments) -> Value
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
                                                   m_FunctionStack.push_back(methodName);

                                                   try
                                                   {
                                                       for (Statement *statement : body->statements)
                                                       {
                                                           ExecStmt(statement);
                                                       }
                                                   }
                                                   catch (const ReturnSignal &signal)
                                                   {
                                                       m_FunctionStack.pop_back();
                                                       m_CurrentScope = previousScope;
                                                       if (returnType.has_value())
                                                       {
                                                           CheckTypeCompatibility(*returnType, methodName, signal.value);
                                                       }
                                                       return signal.value;
                                                   }
                                                   catch (...)
                                                   {
                                                       m_FunctionStack.pop_back();
                                                       m_CurrentScope = previousScope;
                                                       throw;
                                                   }

                                                   m_FunctionStack.pop_back();
                                                   m_CurrentScope = previousScope;
                                                   Value none(nullptr);
                                                   if (returnType.has_value())
                                                   {
                                                       CheckTypeCompatibility(*returnType, methodName, none);
                                                   }
                                                   return none; }, static_cast<int>(parameters.size() + 1));

                    auto existing = klass->fields.find(methodName);
                    if (existing != klass->fields.end() && existing->second.IsCallable())
                    {
                        auto merged = MergeCallable(existing->second.AsCallable(), std::static_pointer_cast<Callable>(callable), methodName);
                        klass->fields[methodName] = Value(merged);
                    }
                    else
                    {
                        klass->fields[methodName] = Value(std::static_pointer_cast<Callable>(callable));
                    }
                }

                m_ClassStack.pop_back();

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
                        RaiseRuntimeError("Delete of undefined variable: " + variableExpr->GetName());
                    }

                    Variable *variable = ownerScope->GetVariable(variableExpr->GetName());
                    if (variable != nullptr && variable->GetValue() != nullptr)
                    {
                        InvokeDestructor(*(variable->GetValue()));
                    }

                    if (!ownerScope->DeleteVariable(variableExpr->GetName()))
                    {
                        RaiseRuntimeError("Delete failed for variable: " + variableExpr->GetName());
                    }
                    return;
                }

                if (auto *memberExpr = dynamic_cast<ast::MemberExpr *>(target))
                {
                    Value objectValue = EvalExpr(memberExpr->GetObject());
                    auto object = objectValue.AsObject();
                    if (!object)
                    {
                        RaiseRuntimeError("Delete member on non-object value");
                    }

                    auto fieldIt = object->fields.find(memberExpr->GetMember());
                    if (fieldIt != object->fields.end())
                    {
                        InvokeDestructor(fieldIt->second);
                        object->fields.erase(fieldIt);
                    }
                    return;
                }

                RaiseRuntimeError("Unsupported delete target");
            }

            RaiseRuntimeError("Unknown statement node");
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
            if (value.IsObject())
            {
                auto object = value.AsObject();
                if (object)
                {
                    auto strIt = object->fields.find("__str__");
                    if (strIt != object->fields.end() && strIt->second.IsCallable())
                    {
                        auto callable = strIt->second.AsCallable();
                        if (callable)
                        {
                            return callable->Call({Value(object)}).AsString();
                        }
                    }
                }
            }
            return value.AsString();
        }

        runtime::Value Interpreter::ApplyBinary(TokenType op, const runtime::Value &lhs, const runtime::Value &rhs) const
        {
            auto invokeOperator = [&](const runtime::Value &value, const std::string &name, const runtime::Value &other) -> std::optional<runtime::Value>
            {
                if (!value.IsObject())
                {
                    return std::nullopt;
                }

                auto object = value.AsObject();
                if (!object)
                {
                    return std::nullopt;
                }

                auto method = object->fields.find(name);
                if (method == object->fields.end() || !method->second.IsCallable())
                {
                    return std::nullopt;
                }

                auto callable = method->second.AsCallable();
                if (!callable)
                {
                    return std::nullopt;
                }

                return callable->Call({Value(object), other});
            };

            const std::string opMethod =
                op == TokenType::Plus           ? "__add__"
                : op == TokenType::Minus        ? "__sub__"
                : op == TokenType::Star         ? "__mul__"
                : op == TokenType::Slash        ? "__truediv__"
                : op == TokenType::Percent      ? "__mod__"
                : op == TokenType::Equal        ? "__eq__"
                : op == TokenType::NotEqual     ? "__ne__"
                : op == TokenType::Less         ? "__lt__"
                : op == TokenType::LessEqual    ? "__le__"
                : op == TokenType::Greater      ? "__gt__"
                : op == TokenType::GreaterEqual ? "__ge__"
                                                : "";

            if (!opMethod.empty())
            {
                auto overloaded = invokeOperator(lhs, opMethod, rhs);
                if (overloaded.has_value())
                {
                    return overloaded.value();
                }
            }

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
                    RaiseRuntimeError("Division by zero");
                }
                return runtime::Value(AsNumber(lhs) / AsNumber(rhs));
            case TokenType::Percent:
                if (AsNumber(rhs) == 0.0)
                {
                    RaiseRuntimeError("Modulo by zero");
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
                RaiseRuntimeError("Unsupported binary operator");
            }
        }

        bool Interpreter::ValuesEqual(const runtime::Value &lhs, const runtime::Value &rhs) const
        {
            return lhs.GetData() == rhs.GetData();
        }

        runtime::Value Interpreter::ApplyUnary(TokenType op, const runtime::Value &rhs) const
        {
            if (rhs.IsObject())
            {
                auto object = rhs.AsObject();
                if (object)
                {
                    const std::string methodName = op == TokenType::Minus ? "__neg__" : op == TokenType::Plus ? "__pos__"
                                                                                    : op == TokenType::Not    ? "__not__"
                                                                                                              : "";
                    if (!methodName.empty())
                    {
                        auto method = object->fields.find(methodName);
                        if (method != object->fields.end() && method->second.IsCallable())
                        {
                            auto callable = method->second.AsCallable();
                            if (callable)
                            {
                                return callable->Call({Value(object)});
                            }
                        }
                    }
                }
            }

            switch (op)
            {
            case TokenType::Minus:
                return runtime::Value(-AsNumber(rhs));
            case TokenType::Plus:
                return runtime::Value(+AsNumber(rhs));
            case TokenType::Not:
                return runtime::Value(!IsTruthy(rhs));
            default:
                RaiseRuntimeError("Unsupported unary operator");
            }
        }

        void Interpreter::CheckTypeCompatibility(const std::string &type, const std::string &name, const runtime::Value &value) const
        {
            if (type.empty())
            {
                return;
            }

            std::string lowered = type;
            for (char &ch : lowered)
            {
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            }

            if (lowered == "any" || lowered == "object")
            {
                return;
            }

            if (lowered == "job" || lowered == "void")
            {
                if (!value.IsNull())
                {
                    RaiseRuntimeError("Type mismatch for '" + name + "': expected " + type);
                }
                return;
            }

            if (lowered == "str" || lowered == "string")
            {
                if (!value.IsString())
                {
                    RaiseRuntimeError("Type mismatch for '" + name + "': expected string");
                }
                return;
            }

            if (lowered == "float")
            {
                if (!value.IsNumber())
                {
                    RaiseRuntimeError("Type mismatch for '" + name + "': expected float");
                }
                return;
            }

            if (lowered == "int" || lowered == "integer")
            {
                if (!value.IsNumber())
                {
                    RaiseRuntimeError("Type mismatch for '" + name + "': expected int");
                }

                const double number = value.AsNumber();
                if (std::floor(number) != number)
                {
                    RaiseRuntimeError("Type mismatch for '" + name + "': expected int");
                }
                return;
            }

            if (lowered == "bool" || lowered == "boolean")
            {
                if (!value.IsBool())
                {
                    RaiseRuntimeError("Type mismatch for '" + name + "': expected bool");
                }
                return;
            }

            if (!value.IsObject() || !value.AsObject() || value.AsObject()->className != type)
            {
                RaiseRuntimeError("Type mismatch for '" + name + "': expected " + type);
            }
        }

    } // namespace runtime

} // namespace cora::compiler
