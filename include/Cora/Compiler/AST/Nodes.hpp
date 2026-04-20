#ifndef CORA_COMPILER_AST_NODES_H
#define CORA_COMPILER_AST_NODES_H

#include <deque>
#include <iostream>
#include <string>
#include <utility>

#include "Cora/Compiler/Basic/SourceLocation.h"

namespace cora::compiler
{
    using namespace basic;

    namespace ast
    {

        using ModuleList = std::deque<class Module *>;
        using StatementList = std::deque<class Statement *>;
        using ExpressionList = std::deque<class Expression *>;

        enum class NodeType
        {
            Module,
            Program,
            Statement,
            Expression,
        };

        enum class StatementType
        {
            IfStmt,
            DoStmt,
            ForStmt,
            NewStmt,
            PassStmt,
            Assignment,
            WhileStmt,
            BreakStmt,
            BraceStmt,
            YieldStmt,
            ThrowStmt,
            DeleteStmt,
            SwitchStmt,
            ReturnStmt,
            ForEachStmt,
            ContinueStmt,
            VarDeclaration,
        };

        enum class ExpressionType
        {
            Null,
            Bool,
            Byte,
            Float,
            Array,
            String,
            Integer,

            Identifier,

            TryExpr,
            CastExpr,
            CallExpr,
            MemberExpr,
            NewExpr,
            BlockExpr,
            AssignExpr,
            BinaryExpr,
            TernaryExpr,
            ArrayAccessExpr,
            PrefixUnaryExpr,
            PostfixUnaryExpr,

        };

        class Node
        {
        public:
            Node(NodeType nodetype);

            Node(NodeType nodetype, SourceRange sourcerange);

            Node(NodeType nodetype, SourceLocation location);

            Node(NodeType nodetype, SourceLocation start, SourceLocation end);

            /**
             * @brief Compute a representation of the node.
             * @return String representation of the node
             */
            virtual std::string Repr() = 0;

            /**
             * @brief Return the node type
             *
             * @return NodeType
             */
            NodeType GetNodeType();

            /**
             * @brief Return the node type string
             *
             * @return const std::string&
             */
            // std::string GetNodeType();

            /**
             * @brief Get the start position of the node
             *
             * @return Position
             */
            SourceLocation GetStartPosition() const noexcept;

            /**
             * @brief Get the end position of the node
             *
             * @return Position
             */
            SourceLocation GetEndPosition() const noexcept;

            /**
             * @brief Get the source range of the node
             *
             * @return SourceRange
             */
            SourceRange GetSourceRange() const;

            /**
             * @brief Set the Node Type object
             *
             * @param nodetype
             */
            void SetNodeType(NodeType nodetype) noexcept;

            /**
             * @brief Set the Node start position
             *
             * @param start
             */
            void SetStartPosition(SourceLocation start) noexcept;

            /**
             * @brief Set the Node end position
             *
             * @param end
             */
            void SetEndPosition(SourceLocation end) noexcept;

            /**
             * @brief Set the Node source range.
             *
             * @param range
             */
            void SetSourceRange(SourceRange range) noexcept;

            virtual ~Node() = default;

        private:
            NodeType m_Type;
            SourceRange m_Range;
        };

        class Program : public Node
        {
        public:
            Program();

            void PushBack(Module *module);

            void EmplaceBack(Module *module);

            const ModuleList &GetModules() const;

            std::string Repr() override;

            ~Program() override;

        private:
            ModuleList m_Modules;
        };

        class Module : public Node
        {
        public:
            Module();

            void PushBack(Statement *statement);

            void EmplaceBack(Statement *statement);

            const StatementList &GetStatements() const;

            std::string Repr() override;

            ~Module() override;

        private:
            StatementList m_Statements;
        };

        std::ostream &operator<<(std::ostream &ostream, const Node *node);

    } // namespace ast

} // namespace cora::compiler

#endif // CORA_COMPILER_AST_NODES_H
