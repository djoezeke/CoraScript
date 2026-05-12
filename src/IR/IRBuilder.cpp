#include "IRBuilder.hpp"

#include "../Parser/Token.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace cora::ir
{
    // Moved out of anonymous namespace
    std::vector<std::string> SplitQualifiedName(const std::string &name)
    {
        std::vector<std::string> parts;
        std::string current;
        for (std::size_t i = 0; i < name.size(); ++i)
        {
            if (name[i] == '.' || (name[i] == ':' && i + 1 < name.size() && name[i + 1] == ':'))
            {
                if (!current.empty())
                {
                    parts.push_back(current);
                    current.clear();
                }
                if (name[i] == ':')
                {
                    ++i;
                }
                continue;
            }

            current.push_back(name[i]);
        }

        if (!current.empty())
        {
            parts.push_back(current);
        }
        return parts;
    }

    // Moved out of anonymous namespace
    runtime::value ExtractLiteralValue(const Value *value)
    {
        if (const auto *constant = dynamic_cast<const Constant *>(value))
        {
            return constant->value;
        }
        return runtime::value(nullptr);
    }

    IRBuilder::IRBuilder() = default;

    BasicBlock *IRBuilder::Build(const std::deque<cora::ast::Statement *> &program)
    {
        Reset();

        auto entry = std::make_unique<BasicBlock>("entry");
        m_entry = entry.get();
        m_currentBlock = m_entry;
        m_blocks.push_back(m_entry);
        m_ownedBlocks.push_back(std::move(entry));

        for (ast::Statement *stmt : program)
        {
            EmitStatement(stmt);
        }

        // The blocks are already in m_blocks because CreateBlock pushes them there.
        // We just need to make sure they are in a sensible order.
        // For now, the order of creation is fine.
        return m_entry;
    }

    void IRBuilder::processFunctionParameters(ast::FuncDeclStmt *stmt, Function *func)
    {
        for (size_t i = 0; i < stmt->params.size(); ++i)
        {
            ast::ParamExpr *param = stmt->params[i];
            if (param == nullptr || param->name == nullptr)
            {
                continue;
            }
            auto *arg = MakeValue<Argument>(Type::Int(), func, static_cast<unsigned>(i));
            arg->name = param->name->name;
            assignVariable(param->name->name, arg);
            func->args.push_back(arg); // Add argument to function's argument list
        }
    }

    std::tuple<BasicBlock *, BasicBlock *, BasicBlock *> IRBuilder::createIfElseBlocks(ast::IfStmt *stmt)
    {
        BasicBlock *thenBlock = CreateBlock("if.then");
        BasicBlock *elseBlock = stmt->false_block != nullptr ? CreateBlock("if.else") : nullptr;
        BasicBlock *mergeBlock = CreateBlock("if.merge");
        return std::make_tuple(thenBlock, elseBlock, mergeBlock);
    }

    std::tuple<BasicBlock *, BasicBlock *, BasicBlock *> IRBuilder::createLoopBlocks(const std::string &prefix)
    {
        BasicBlock *condBlock = CreateBlock(prefix + ".cond");
        BasicBlock *bodyBlock = CreateBlock(prefix + ".body");
        BasicBlock *exitBlock = CreateBlock(prefix + ".exit");
        return std::make_tuple(condBlock, bodyBlock, exitBlock);
    }

    Instruction *IRBuilder::EmitStatement(ast::Statement *stmt)
    {
        if (stmt == nullptr)
        {
            return nullptr;
        }

        if (auto *exprStmt = dynamic_cast<ast::ExprStmt *>(stmt))
        {
            EmitExpression(exprStmt->expr);
            return nullptr;
        }
        if (auto *blockStmt = dynamic_cast<ast::BlockStmt *>(stmt))
        {
            EmitBlock(blockStmt);
            return nullptr;
        }
        if (auto *ifStmt = dynamic_cast<ast::IfStmt *>(stmt))
        {
            EmitIf(ifStmt);
            return nullptr;
        }
        if (auto *whileStmt = dynamic_cast<ast::WhileStmt *>(stmt))
        {
            EmitWhile(whileStmt);
            return nullptr;
        }
        if (auto *forStmt = dynamic_cast<ast::ForStmt *>(stmt))
        {
            EmitFor(forStmt);
            return nullptr;
        }
        if (auto *varDecl = dynamic_cast<ast::VarDeclStmt *>(stmt))
        {
            EmitVarDecl(varDecl);
            return nullptr;
        }
        if (auto *funcDecl = dynamic_cast<ast::FuncDeclStmt *>(stmt))
        {
            EmitFuncDecl(funcDecl);
            return nullptr;
        }
        if (auto *forInStmt = dynamic_cast<ast::ForInStmt *>(stmt))
        {
            EmitForIn(forInStmt);
            return nullptr;
        }
        if (auto *switchStmt = dynamic_cast<ast::SwitchStmt *>(stmt))
        {
            EmitSwitch(switchStmt);
            return nullptr;
        }
        if (auto *importStmt = dynamic_cast<ast::ImportStmt *>(stmt))
        {
            EmitImport(importStmt);
            return nullptr;
        }
        if (auto *classDecl = dynamic_cast<ast::ClassDecl *>(stmt))
        {
            EmitClass(classDecl);
            return nullptr;
        }
        if (auto *structDecl = dynamic_cast<ast::StructDecl *>(stmt))
        {
            EmitStruct(structDecl);
            return nullptr;
        }
        if (auto *enumDecl = dynamic_cast<ast::EnumDecl *>(stmt))
        {
            EmitEnum(enumDecl);
            return nullptr;
        }
        if (auto *tryCatch = dynamic_cast<ast::TryCatchStmt *>(stmt))
        {
            EmitTryCatch(tryCatch);
            return nullptr;
        }
        if (dynamic_cast<ast::BreakStmt *>(stmt))
        {
            if (!m_breakStack.empty())
            {
                MakeValue<JumpInstruction>(m_breakStack.back(), m_currentBlock);
            }
            return nullptr;
        }
        if (dynamic_cast<ast::ContinueStmt *>(stmt))
        {
            if (!m_continueStack.empty())
            {
                MakeValue<JumpInstruction>(m_continueStack.back(), m_currentBlock);
            }
            return nullptr;
        }
        if (auto *retStmt = dynamic_cast<ast::ReturnStmt *>(stmt))
        {
            Value *retValue = retStmt->value != nullptr ? EmitExpression(retStmt->value) : nullptr;
            // If retValue is null, it means a void return. The ReturnInstruction should reflect this.
            if (retValue == nullptr)
            {
                return MakeValue<ReturnInstruction>(m_currentBlock, nullptr); // Explicitly pass nullptr for void return
            }
            else
            {
                return MakeValue<ReturnInstruction>(m_currentBlock, retValue);
            }
        }

        return nullptr;
    }

    void IRBuilder::EmitBlock(ast::BlockStmt *block)
    {
        if (block == nullptr)
        {
            return;
        }

        for (ast::Statement *stmt : block->stmts)
        {
            EmitStatement(stmt);
        }
    }

    void IRBuilder::EmitIf(ast::IfStmt *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }

        Value *condition = EmitExpression(stmt->cond);
        if (condition == nullptr)
        {
            condition = MakeConstant(runtime::value(true));
        }

        BasicBlock *thenBlock, *elseBlock, *mergeBlock;
        std::tie(thenBlock, elseBlock, mergeBlock) = createIfElseBlocks(stmt);

        if (elseBlock)
        {
            MakeValue<BranchInstruction>(condition, thenBlock, elseBlock, m_currentBlock);
        }
        else
        {
            MakeValue<BranchInstruction>(condition, thenBlock, mergeBlock, m_currentBlock);
        }

        m_currentBlock = thenBlock;
        EmitBlock(stmt->true_block);
        MakeValue<JumpInstruction>(mergeBlock, m_currentBlock);

        if (elseBlock)
        {
            m_currentBlock = elseBlock;
            EmitBlock(stmt->false_block);
            MakeValue<JumpInstruction>(mergeBlock, m_currentBlock);
        }

        m_currentBlock = mergeBlock;
    }

    void IRBuilder::EmitWhile(ast::WhileStmt *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }

        BasicBlock *condBlock, *bodyBlock, *exitBlock;
        std::tie(condBlock, bodyBlock, exitBlock) = createLoopBlocks("while");

        MakeValue<JumpInstruction>(condBlock, m_currentBlock);

        m_currentBlock = condBlock;
        Value *condition = EmitExpression(stmt->condition);
        if (condition == nullptr)
        {
            condition = MakeConstant(runtime::value(true));
        }
        MakeValue<BranchInstruction>(condition, bodyBlock, exitBlock, m_currentBlock);

        m_breakStack.push_back(exitBlock);
        m_continueStack.push_back(condBlock);

        m_currentBlock = bodyBlock;
        EmitBlock(stmt->block);
        MakeValue<JumpInstruction>(condBlock, m_currentBlock);

        m_continueStack.pop_back();
        m_breakStack.pop_back();

        m_currentBlock = exitBlock;
    }

    void IRBuilder::EmitFor(ast::ForStmt *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }

        EmitStatement(stmt->init);

        BasicBlock *condBlock, *bodyBlock, *exitBlock;
        std::tie(condBlock, bodyBlock, exitBlock) = createLoopBlocks("for");
        BasicBlock *updateBlock = CreateBlock("for.update");

        MakeValue<JumpInstruction>(condBlock, m_currentBlock);

        m_currentBlock = condBlock;
        Value *condition = EmitExpression(stmt->condition);
        if (condition == nullptr)
        {
            condition = MakeConstant(runtime::value(true));
        }
        MakeValue<BranchInstruction>(condition, bodyBlock, exitBlock, m_currentBlock);

        m_breakStack.push_back(exitBlock);
        m_continueStack.push_back(updateBlock);

        m_currentBlock = bodyBlock;
        EmitBlock(stmt->block);
        MakeValue<JumpInstruction>(updateBlock, m_currentBlock);

        m_currentBlock = updateBlock;
        EmitStatement(stmt->update);
        MakeValue<JumpInstruction>(condBlock, m_currentBlock);

        m_continueStack.pop_back();
        m_breakStack.pop_back();

        m_currentBlock = exitBlock;
    }

    void IRBuilder::EmitVarDecl(ast::VarDeclStmt *stmt)
    {
        if (stmt == nullptr || stmt->name == nullptr)
        {
            return;
        }

        const std::string &name = stmt->name->name;
        Value *slot = lookupVariable(name);
        if (slot == nullptr)
        {
            slot = MakeValue<AllocaInstruction>(Type::Pointer(Type::Int()), name, m_currentBlock);
            assignVariable(name, slot);
        }

        if (stmt->value != nullptr)
        {
            Value *rhs = EmitExpression(stmt->value);
            if (rhs != nullptr)
            {
                MakeValue<StoreInstruction>(rhs, slot, m_currentBlock);
            }
        }
    }

    void IRBuilder::EmitFuncDecl(ast::FuncDeclStmt *stmt)
    {
        if (stmt == nullptr || stmt->name == nullptr)
        {
            return;
        }

        const std::string &name = stmt->name->name;
        BasicBlock *savedBlock = m_currentBlock;
        auto savedVars = m_variables;

        BasicBlock *funcEntry = CreateBlock("fn." + name);
        m_currentBlock = funcEntry;
        m_variables.clear();

        std::vector<Type *> paramTypes; // Collect parameter types
        for (ast::ParamExpr *param : stmt->params)
        {
            // Assuming all parameters are int for now, or infer type if available
            paramTypes.push_back(Type::Int());
        }

        auto *func = MakeValue<Function>(name, Type::Int(), paramTypes, funcEntry);

        processFunctionParameters(stmt, func);

        EmitBlock(stmt->block);

        // Ensure every function ends with a return if not already present
        if (m_currentBlock->insts.empty() || m_currentBlock->insts.back()->opcode != Instruction::Opcode::Ret)
        {
            MakeValue<ReturnInstruction>(m_currentBlock, nullptr);
        }

        savedVars[name] = func;

        m_variables = std::move(savedVars);
        m_currentBlock = savedBlock;
    }

    void IRBuilder::EmitForIn(ast::ForInStmt *stmt)
    {
        if (stmt == nullptr || stmt->iterable == nullptr)
        {
            return;
        }

        Value *iterableVal = EmitExpression(stmt->iterable);
        if (iterableVal == nullptr)
        {
            return;
        }

        BasicBlock *loopEntry = CreateBlock("forin.entry");
        BasicBlock *loopBody = CreateBlock("forin.body");
        BasicBlock *loopExit = CreateBlock("forin.exit");

        MakeValue<BranchInstruction>(loopEntry, m_currentBlock);
        m_currentBlock = loopEntry;

        if (stmt->variable != nullptr)
        {
            Value *slot = MakeValue<AllocaInstruction>(Type::Pointer(Type::Int()), stmt->variable->name->name, m_currentBlock);
            assignVariable(stmt->variable->name->name, slot);
        }

        if (stmt->block != nullptr)
        {
            EmitBlock(stmt->block);
        }

        MakeValue<BranchInstruction>(loopExit, m_currentBlock);
        m_currentBlock = loopExit;
    }

    void IRBuilder::EmitSwitch(ast::SwitchStmt *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }

        Value *cond = nullptr;
        if (stmt->cond != nullptr)
        {
            cond = EmitExpression(stmt->cond);
        }

        BasicBlock *switchExit = CreateBlock("switch.exit");
        BasicBlock *currentCase = nullptr;

        for (const auto &match : stmt->matches)
        {
            if (match != nullptr && match->block != nullptr)
            {
                currentCase = CreateBlock("case");
                m_currentBlock = currentCase;
                EmitBlock(match->block);
                MakeValue<BranchInstruction>(switchExit, m_currentBlock);
            }
        }

        m_currentBlock = switchExit;
    }

    void IRBuilder::EmitImport(ast::ImportStmt *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }
    }

    void IRBuilder::EmitClass(ast::ClassDecl *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }
    }

    void IRBuilder::EmitStruct(ast::StructDecl *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }
    }

    void IRBuilder::EmitEnum(ast::EnumDecl *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }
    }

    void IRBuilder::EmitTryCatch(ast::TryCatchStmt *stmt)
    {
        if (stmt == nullptr)
        {
            return;
        }

        if (stmt->tryBlock != nullptr)
        {
            EmitBlock(stmt->tryBlock);
        }
    }

    Value *IRBuilder::EmitExpression(ast::Expression *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        if (auto *literal = EmitLiteral(expr))
        {
            return literal;
        }

        if (auto *identifier = dynamic_cast<ast::IdentifierExpr *>(expr))
        {
            return EmitIdentifier(identifier, true);
        }

        if (auto *assign = dynamic_cast<ast::AssignExpr *>(expr))
        {
            return EmitAssignExpr(assign);
        }

        if (auto *binary = dynamic_cast<ast::BinaryExpr *>(expr))
        {
            return EmitBinaryExpr(binary);
        }

        if (auto *unary = dynamic_cast<ast::UnaryExpr *>(expr))
        {
            return EmitUnaryExpr(unary);
        }

        if (auto *call = dynamic_cast<ast::FuncCallExpr *>(expr))
        {
            return EmitFuncCallExpr(call);
        }

        if (auto *ternary = dynamic_cast<ast::TernaryExpr *>(expr))
        {
            return EmitTernaryExpr(ternary);
        }

        // if (auto *arrayExpr = dynamic_cast<ast::ArrayExpr *>(expr))
        // {
        //     // Determine the element type. For simplicity, assume all elements are of the same type.
        //     // Default to int if the array is empty or type cannot be determined.
        //     Type *elementType = Type::Int();
        //     if (!arrayExpr->value.empty())
        //     {
        //         // Try to determine type from the first element. This is a simplification.
        //         // A robust compiler would infer type from all elements or require explicit type annotation.
        //         Value *firstElementValue = EmitExpression(arrayExpr->value[0]);
        //         if (firstElementValue && firstElementValue->type)
        //         {
        //             // If the element is a pointer type, use its pointee type. Otherwise, use its own type.
        //             // This logic needs to be more robust. For now, assuming basic types.
        //             if (firstElementValue->type->isPointer())
        //             {
        //                 elementType = firstElementValue->type->pointee;
        //             }
        //             else
        //             {
        //                 elementType = firstElementValue->type;
        //             }
        //         }
        //     }

        //     // Allocate memory for the array. The size would be arrayExpr->value.size() * sizeof(elementType).
        //     // For now, we are not generating a fixed-size array IR instruction, but rather allocating space
        //     // and then storing elements. This is a simplified approach.
        //     // A proper implementation might use an 'array_alloc' instruction.
        //     Value *arrayPtr = MakeValue<AllocaInstruction>(Type::Array(elementType), makeTempName("array"), m_currentBlock);

        //     // Store each element into the allocated array
        //     for (size_t i = 0; i < arrayExpr->value.size(); ++i)
        //     {
        //         Value *elementValue = EmitExpression(arrayExpr->value[i]);
        //         if (elementValue == nullptr)
        //         {
        //             // Handle error or skip element
        //             continue;
        //         }
        //         // TODO: Generate an IR instruction for array element access (e.g., getelementptr)
        //         // and then a StoreInstruction. For now, we'll emit a placeholder store.
        //         // This part is highly simplified and needs proper array indexing logic.
        //         MakeValue<StoreInstruction>(elementValue, arrayPtr, m_currentBlock);
        //     }
        //     // For now, returning the pointer to the array.
        //     // A more refined approach might return a Value representing the array itself.
        //     return arrayPtr;
        // }

        return nullptr;
    }

    Value *IRBuilder::EmitLiteral(ast::Expression *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        if (auto *literal = dynamic_cast<ast::IntegerExpr *>(expr))
        {
            return MakeConstant(runtime::value(literal->value), makeTempName("int"));
        }
        if (auto *literal = dynamic_cast<ast::FloatExpr *>(expr))
        {
            return MakeConstant(runtime::value(literal->value), makeTempName("float"));
        }
        if (auto *literal = dynamic_cast<ast::StringExpr *>(expr))
        {
            return MakeConstant(runtime::value(literal->value), makeTempName("str"));
        }
        if (auto *literal = dynamic_cast<ast::BoolExpr *>(expr))
        {
            return MakeConstant(runtime::value(literal->value), makeTempName("bool"));
        }
        if (dynamic_cast<ast::NullExpr *>(expr) != nullptr)
        {
            return MakeConstant(runtime::value(nullptr), makeTempName("null"));
        }

        if (auto *arrayExpr = dynamic_cast<ast::ArrayExpr *>(expr))
        {
            runtime::value::array_type values;
            values.reserve(arrayExpr->value.size());
            for (ast::Expression *element : arrayExpr->value)
            {
                values.push_back(ExtractLiteralValue(EmitExpression(element)));
            }
            return MakeConstant(runtime::value(std::move(values)), makeTempName("arr"));
        }

        if (auto *structExpr = dynamic_cast<ast::StructLiteralExpr *>(expr))
        {
            auto object = std::make_shared<runtime::Object>("StructLiteral");
            for (const auto &field : structExpr->fields)
            {
                if (field.first == nullptr)
                {
                    continue;
                }
                object->fields[field.first->name] = ExtractLiteralValue(EmitExpression(field.second));
            }
            return MakeConstant(runtime::value(std::move(object)), makeTempName("obj"));
        }

        return nullptr;
    }

    Value *IRBuilder::EmitIdentifier(ast::IdentifierExpr *expr, bool load)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        const std::vector<std::string> parts = SplitQualifiedName(expr->name);
        if (parts.empty())
        {
            return nullptr;
        }

        Value *value = lookupVariable(parts.front());
        if (value == nullptr)
        {
            // If not found in local variables, it might be a global.
            // For qualified names like io.print, we currently don't have GetMember opcode,
            // so we'll try to treat the whole thing as a global name or just the base.
            // Given the current VM, let's treat the base as global and we might need GetMember for the rest.
            // But if we don't have GetMember, let's just return a placeholder for the base.
            value = MakeValue<Constant>(runtime::value(nullptr), parts.front());
        }

        if (parts.size() > 1)
        {
            runtime::value current = ExtractLiteralValue(value);
            bool resolvedAtCompileTime = true;
            for (std::size_t i = 1; i < parts.size(); ++i)
            {
                if (!current.IsObject())
                {
                    resolvedAtCompileTime = false;
                    break;
                }

                auto object = current.AsObject();
                if (!object)
                {
                    resolvedAtCompileTime = false;
                    break;
                }

                auto it = object->fields.find(parts[i]);
                if (it == object->fields.end())
                {
                    resolvedAtCompileTime = false;
                    break;
                }

                current = it->second;
            }

            if (resolvedAtCompileTime)
            {
                return MakeConstant(std::move(current), makeTempName("load"));
            }

            // If not resolved at compile time, we should ideally emit GetMember instructions.
            // For now, let's at least return the base value if it's a global.
            // Actually, to make io.print(sum) work, we'll just return the qualified name as a global if not resolved.
            return MakeValue<Constant>(runtime::value(nullptr), expr->name);
        }

        if (!load)
        {
            return value;
        }

        if (dynamic_cast<Function *>(value) != nullptr || dynamic_cast<Constant *>(value) != nullptr || dynamic_cast<Argument *>(value) != nullptr)
        {
            return value;
        }

        return MakeValue<LoadInstruction>(value, makeTempName("load"), m_currentBlock);
    }

    Value *IRBuilder::EmitBinaryExpr(ast::BinaryExpr *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        Value *lhs = EmitExpression(expr->left);
        Value *rhs = EmitExpression(expr->right);
        if (lhs == nullptr || rhs == nullptr)
        {
            return nullptr;
        }

        Instruction::Opcode opcode = Instruction::Opcode::Add;
        switch (expr->op)
        {
        case parser::TokenType::Plus:
            opcode = Instruction::Opcode::Add;
            break;
        case parser::TokenType::Minus:
            opcode = Instruction::Opcode::Sub;
            break;
        case parser::TokenType::Star:
            opcode = Instruction::Opcode::Mul;
            break;
        case parser::TokenType::Slash:
            opcode = Instruction::Opcode::Div;
            break;
        case parser::TokenType::Equal:
            opcode = Instruction::Opcode::Eq;
            break;
        case parser::TokenType::NotEqual:
            opcode = Instruction::Opcode::Ne;
            break;
        case parser::TokenType::Less:
            opcode = Instruction::Opcode::Lt;
            break;
        case parser::TokenType::LessEqual:
            opcode = Instruction::Opcode::Le;
            break;
        case parser::TokenType::Greater:
            opcode = Instruction::Opcode::Gt;
            break;
        case parser::TokenType::GreaterEqual:
            opcode = Instruction::Opcode::Ge;
            break;
        default:
            opcode = Instruction::Opcode::Add;
            break;
        }

        // Create a new instruction for the binary operation with the determined result type.
        return MakeValue<BinaryInstruction>(opcode, lhs, rhs, makeTempName(), m_currentBlock);
    }

    Value *IRBuilder::EmitUnaryExpr(ast::UnaryExpr *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        // Handle postfix ++/-- expressed as PostfixUnaryExpr
        if (auto *post = dynamic_cast<ast::PostfixUnaryExpr *>(expr))
        {
            // Only support identifier targets for now
            auto *ident = dynamic_cast<ast::IdentifierExpr *>(post->expr);
            if (ident == nullptr)
            {
                return nullptr;
            }

            // Get slot (lvalue)
            Value *slot = EmitIdentifier(ident, false);
            if (slot == nullptr)
            {
                // If variable not found, allocate one
                slot = MakeValue<AllocaInstruction>(Type::Pointer(Type::Int()), ident->name, m_currentBlock);
                assignVariable(ident->name, slot);
            }

            // Load current value
            Value *cur = EmitIdentifier(ident, true);
            if (cur == nullptr)
            {
                return nullptr;
            }

            // Create constant 1
            Value *one = MakeConstant(runtime::value(1), makeTempName("int"));

            Instruction::Opcode opcode = Instruction::Opcode::Add;
            if (post->op == parser::TokenType::Minus)
            {
                opcode = Instruction::Opcode::Sub;
            }

            // newValue = cur + 1 (or cur - 1)
            Value *newVal = MakeValue<BinaryInstruction>(opcode, cur, one, makeTempName(), m_currentBlock);

            // store newVal into slot
            MakeValue<StoreInstruction>(newVal, slot, m_currentBlock);

            return newVal;
        }

        // Fallback for prefix unary ops
        Value *value = EmitExpression(expr->expr);
        if (value == nullptr)
        {
            return nullptr;
        }

        Instruction::Opcode opcode = Instruction::Opcode::Sub;
        if (expr->op == parser::TokenType::Minus)
        {
            opcode = Instruction::Opcode::Sub;
        }
        return MakeValue<UnaryInstruction>(opcode, value, makeTempName(), m_currentBlock);
    }

    Value *IRBuilder::EmitFuncCallExpr(ast::FuncCallExpr *expr)
    {
        if (expr == nullptr || expr->name == nullptr)
        {
            return nullptr;
        }

        auto *callee = EmitIdentifier(expr->name, true);
        if (callee == nullptr)
        {
            return nullptr;
        }

        std::vector<Value *> args;
        args.reserve(expr->args.size());
        for (ast::Statement *argStmt : expr->args)
        {
            auto *exprStmt = dynamic_cast<ast::ExprStmt *>(argStmt);
            if (exprStmt == nullptr)
            {
                continue;
            }
            Value *argValue = EmitExpression(exprStmt->expr);
            if (argValue != nullptr)
            {
                args.push_back(argValue);
            }
        }

        // Create the CallInstruction.
        auto *callInst = MakeValue<CallInstruction>(callee, args, makeTempName("call"), m_currentBlock);

        // FIX: Set the return type of the CallInstruction.
        // If the callee is a Function object, use its returnType.
        if (auto *func = dynamic_cast<Function *>(callee))
        {
            callInst->type = func->returnType;
        }
        else
        {
            // If the callee is not a Function object directly, its type might represent a function type.
            // This requires more robust type system integration.
            // For now, to prevent a null dereference in Type::toString(), we default to Type::Void().
            callInst->type = Type::Void();
        }

        return callInst;
    }

    BasicBlock *IRBuilder::CreateBlock(const std::string &name)
    {
        std::ostringstream out;
        out << name << "." << m_blocks.size();
        auto block = std::make_unique<BasicBlock>(out.str());
        BasicBlock *raw = block.get();
        // std::cerr << "Creating block: " << raw->name << "";
        m_blocks.push_back(raw);
        m_ownedBlocks.push_back(std::move(block));
        return raw;
    }

    Value *IRBuilder::MakeConstant(runtime::value value, const std::string &name)
    {
        return MakeValue<Constant>(std::move(value), name);
    }

    void IRBuilder::Reset()
    {
        m_ownedBlocks.clear();
        m_blocks.clear();
        m_ownedValues.clear();
        m_variables.clear();
        m_entry = nullptr;
        m_currentBlock = nullptr;
        m_tempIndex = 0;
    }

    const std::vector<BasicBlock *> &IRBuilder::GetBlocks() const
    {
        // Instead of returning m_blocks, let's return a list of all owned blocks
        // to ensure we don't miss any that were created but not manually pushed to m_blocks.
        // Actually, CreateBlock already pushes to m_blocks.
        // Let's just make sure m_blocks contains all reachable blocks in a good order.
        return m_blocks;
    }

    BasicBlock *IRBuilder::GetEntryBlock() const
    {
        return m_entry;
    }

    Value *IRBuilder::EmitTernaryExpr(ast::TernaryExpr *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }
        return nullptr;
    }

    Value *IRBuilder::lookupVariable(const std::string &name) const
    {
        const auto found = m_variables.find(name);
        if (found == m_variables.end())
        {
            return nullptr;
        }
        return found->second;
    };

    void IRBuilder::assignVariable(const std::string &name, Value *value)
    {
        m_variables[name] = value;
    };

    std::string IRBuilder::makeTempName(const std::string &prefix)
    {
        std::ostringstream out;
        out << '%' << prefix << m_tempIndex++;
        return out.str();
    };

    IRBuilder::~IRBuilder() {};

    Value *IRBuilder::EmitAssignExpr(ast::AssignExpr *expr)
    {
        if (expr == nullptr)
        {
            return nullptr;
        }

        auto *identifier = dynamic_cast<ast::IdentifierExpr *>(expr->left);
        if (identifier == nullptr)
        {
            return nullptr;
        }

        Value *slot = EmitIdentifier(identifier, false);
        if (slot == nullptr)
        {
            slot = MakeValue<AllocaInstruction>(Type::Pointer(Type::Int()), identifier->name, m_currentBlock);
            assignVariable(identifier->name, slot);
        }

        Value *rhs = EmitExpression(expr->right);
        if (rhs == nullptr)
        {
            return nullptr;
        }

        MakeValue<StoreInstruction>(rhs, slot, m_currentBlock);
        return rhs;
    }

} // namespace cora::ir