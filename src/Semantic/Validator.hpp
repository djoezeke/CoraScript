#ifndef CORA_COMPILER_SEMANTIC_VALIDATOR_H
#define CORA_COMPILER_SEMANTIC_VALIDATOR_H

#include "../AST/ASTStmt.hpp"

#include <deque>
#include <string>

namespace cora::compiler
{
    namespace semantic
    {

        void ValidateProgram(const std::deque<ast::Statement *> &program,
                             const std::string &fileName,
                             const std::string &moduleName);

    } // namespace semantic

} // namespace cora::compiler

#endif // CORA_COMPILER_SEMANTIC_VALIDATOR_H
