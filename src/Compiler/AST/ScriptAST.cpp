#include "Cora/Compiler/AST/ScriptAST.hpp"

namespace cora
{
    namespace script
    {
        BlockStmt::~BlockStmt()
        {
            for (Stmt *statement : statements)
            {
                delete statement;
            }
        }

        IfStmt::~IfStmt()
        {
            for (auto &entry : branches)
            {
                delete entry.first;
                delete entry.second;
            }
            delete elseBlock;
        }

        WhileStmt::~WhileStmt()
        {
            delete condition;
            delete block;
        }

        ForRangeStmt::~ForRangeStmt()
        {
            delete start;
            delete end;
            delete step;
            delete block;
        }

        ForCStyleStmt::~ForCStyleStmt()
        {
            delete init;
            delete condition;
            delete update;
            delete block;
        }
    }
}
