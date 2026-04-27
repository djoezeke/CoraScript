#include "IRReader.hpp"

#include <sstream>

namespace cora::ir
{

    std::vector<BasicBlock *> IRReader::Read()
    {
    }

    std::vector<BasicBlock *> IRReader::Read(std::istream &in) {
    };

    std::vector<BasicBlock *> IRReader::ReadFile(std::string filename) {
    };

    void IRReader::ReadPhiInstruction() {};
    void IRReader::ReadBinaryInstruction() {};
    void IRReader::ReadLoadInstruction() {};
    void IRReader::ReadStoreInstruction() {};
    void IRReader::ReadCallInstruction() {};
    void IRReader::ReadBranchInstruction() {};
    void IRReader::ReadReturnInstruction() {};
    void IRReader::ReadJumpInstruction() {};
    void IRReader::ReadAllocaInstruction() {};

    IRReader::~IRReader() = default;

} // namespace cora::ir
