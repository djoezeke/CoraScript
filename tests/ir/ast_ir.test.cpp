#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include "Cora.hpp"
#include "IR/IRBuilder.hpp"
#include "IR/IRPrinter.hpp"
#include "Parser/Parser.hpp"

TEST_CASE("IR Generation for simple assignment", "[IRBuilder]") {
    std::string source = "let x = 1 + 2;";
    std::string fileName = "test.cora";

    cora::parser::Parser parser;
    parser.SetFileName(fileName);
    parser.SetModuleName(fileName);

    std::vector<cora::ast::Statement *> parsed = parser.Parse(source);
    REQUIRE(!parsed.empty());

    std::deque<cora::ast::Statement *> programQueue(parsed.begin(), parsed.end());

    cora::ir::IRBuilder builder;
    builder.Build(programQueue);

    const std::vector<cora::ir::BasicBlock *> &blocks = builder.GetBlocks();
    REQUIRE(!blocks.empty());

    // Basic verification: Check if there's at least one block and some instructions
    REQUIRE(blocks.size() >= 1);
    REQUIRE(blocks[0]->insts.size() >= 1);

    // Further detailed checks can be added here, e.g.,
    // - Check for specific instruction opcodes
    // - Check operand values
    // - Print IR for visual inspection (can be commented out for automated tests)
    // cora::ir::IRPrinter printer;
    // std::cout << "
Generated IR:
" << printer.Print(blocks) << std::endl;
}

int main(int argc, char* argv[]) {
    Catch::Session session;
    int returnCode = session.run(argc, argv);
    return returnCode;
}
