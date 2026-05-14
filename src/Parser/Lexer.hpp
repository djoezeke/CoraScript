#ifndef CORA_PARSER_LEXER_H
#define CORA_PARSER_LEXER_H

#include "Token.hpp"
#include "Cora/SourceManager.hpp"
#include "Cora/DiagnosticEngine.hpp"

#include <string>
#include <vector>

namespace cora::parser
{
    using namespace cora;

    class Lexer
    {
    public:
        Lexer(SourceManager &sm, DiagnosticEngine &de);
        Lexer(SourceManager &sm, DiagnosticEngine &de, uint32_t fileID);

        std::vector<Token> Tokenize();

        void SetFileID(uint32_t fileID);
        void SetModuleName(std::string moduleName);

        Token NextToken();
        Token PrevToken() const;

        ~Lexer();

    private:
        static std::string Trim(const std::string &line);
        static int CountIndent(const std::string &line);

        void BuildTokens();
        void PushToken(TokenType type, const std::string &text, uint32_t offset, uint32_t line, uint32_t column);
        void RaiseLexError(ErrorCode id, const std::string &message, uint32_t offset) const;

    private:
        SourceManager &m_SourceManager;
        DiagnosticEngine &m_DiagnosticEngine;
        uint32_t m_FileID{0};
        std::vector<Token> m_Tokens;
        std::size_t m_Position{0};
        Token m_Prev;
        std::string m_ModuleName;
    };

} // namespace cora::parser

#endif // CORA_PARSER_LEXER_H
