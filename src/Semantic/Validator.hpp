#ifndef CORA_SEMANTIC_VALIDATOR_H
#define CORA_SEMANTIC_VALIDATOR_H

#include "../AST/ASTStmt.hpp"

#include <deque>
#include <string>

namespace cora::semantic
{

    using namespace cora;

    void ValidateProgram(const std::deque<ast::Statement *> &program,
                         const std::string &fileName,
                         const std::string &moduleName);

} // namespace cora::semantic

#endif // CORA_SEMANTIC_VALIDATOR_H
