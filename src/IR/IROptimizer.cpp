#include "IROptimizer.hpp"

namespace cora::ir
{
    IROptimizer::IROptimizer() = default;

    void IROptimizer::Optimize(std::vector<BasicBlock *> &)
    {
    }

    IROptimizer::~IROptimizer() = default;
}