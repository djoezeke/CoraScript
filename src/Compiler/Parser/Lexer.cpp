#include "Cora/Compiler/Parser/Lexer.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace cora::compiler
{
    namespace parser
    {

        Lexer::Lexer()
            : m_Source(), m_Tokens(), m_Position(0), m_Prev(TokenType::End, "<eof>", 1, 1) {}

        Lexer::Lexer(std::string source)
            : m_Source(std::move(source)), m_Tokens(), m_Position(0), m_Prev(TokenType::End, "<eof>", 1, 1)
        {
            BuildTokens();
        }

        std::deque<Token> Lexer::Lex(const std::string &source)
        {
            m_Source = source;
            BuildTokens();
            return m_Tokens;
        }

        std::deque<Token> Lexer::Tokenize()
        {
            if (m_Tokens.empty())
            {
                BuildTokens();
            }
            return m_Tokens;
        }

        void Lexer::SetFileName(std::string fileName)
        {
            m_FileName = fileName.empty() ? "<memory>" : std::move(fileName);
        }

        void Lexer::SetModuleName(std::string moduleName)
        {
            m_ModuleName = std::move(moduleName);
        }

        Token Lexer::NextToken()
        {
            if (m_Tokens.empty())
            {
                BuildTokens();
            }

            if (m_Position >= m_Tokens.size())
            {
                return m_Prev;
            }

            m_Prev = m_Tokens[m_Position++];
            return m_Prev;
        }

        Token Lexer::PrevToken() const
        {
            return m_Prev;
        }

        void Lexer::BuildTokens()
        {
            m_Tokens.clear();
            m_Position = 0;

            static const std::unordered_map<std::string, TokenType> keywords = {
                {"if", TokenType::If},
                {"elif", TokenType::Elif},
                {"else", TokenType::Else},
                {"while", TokenType::While},
                {"for", TokenType::For},
                {"import", TokenType::Import},
                {"in", TokenType::In},
                {"range", TokenType::Range},
                {"break", TokenType::Break},
                {"continue", TokenType::Continue},
                {"pass", TokenType::Pass},
                {"let", TokenType::Let},
                {"const", TokenType::Const},
                {"and", TokenType::And},
                {"or", TokenType::Or},
                {"not", TokenType::Not},
                {"null", TokenType::Null},
                {"none", TokenType::Null},
                {"true", TokenType::True},
                {"false", TokenType::False},
                {"class", TokenType::T_CLASS},
                {"new", TokenType::T_NEW},
                {"delete", TokenType::T_DELETE},
                {"fun", TokenType::T_FUN},
                {"return", TokenType::T_RETURN},
                {"this", TokenType::T_THIS},
                {"public", TokenType::T_PUBLIC},
                {"private", TokenType::T_PRIVATE},
                {"namespace", TokenType::T_NAMESPACE},
            };

            std::istringstream stream(m_Source);
            std::deque<int> indentStack;
            indentStack.push_back(0);

            std::string line;
            unsigned int lineNo = 1;
            int braceDepth = 0;

            while (std::getline(stream, line))
            {
                const std::string trimmed = Trim(line);
                if (trimmed.empty() || trimmed.rfind("#", 0) == 0)
                {
                    ++lineNo;
                    continue;
                }

                const int indent = CountIndent(line);
                std::size_t pos = static_cast<std::size_t>(indent);

                if (braceDepth == 0)
                {
                    if (indent > indentStack.back())
                    {
                        indentStack.push_back(indent);
                        PushToken(TokenType::Indent, "<indent>", lineNo, 1);
                    }
                    while (indent < indentStack.back())
                    {
                        indentStack.pop_back();
                        PushToken(TokenType::Dedent, "<dedent>", lineNo, 1);
                    }
                    if (indent != indentStack.back())
                    {
                        RaiseLexError("Indentation mismatch", lineNo, 1);
                    }
                }

                unsigned int column = static_cast<unsigned int>(pos) + 1;
                while (pos < line.size())
                {
                    const char ch = line[pos];

                    if (std::isspace(static_cast<unsigned char>(ch)))
                    {
                        ++pos;
                        ++column;
                        continue;
                    }

                    if (ch == '#')
                    {
                        break;
                    }

                    if (ch == '/' && pos + 1 < line.size() && line[pos + 1] == '/')
                    {
                        break;
                    }

                    if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_')
                    {
                        const std::size_t start = pos;
                        const unsigned int startCol = column;
                        while (pos < line.size() && (std::isalnum(static_cast<unsigned char>(line[pos])) || line[pos] == '_'))
                        {
                            ++pos;
                            ++column;
                        }

                        const std::string text = line.substr(start, pos - start);
                        auto keyword = keywords.find(text);
                        PushToken(keyword != keywords.end() ? keyword->second : TokenType::Identifier, text, lineNo, startCol);
                        continue;
                    }

                    if (std::isdigit(static_cast<unsigned char>(ch)))
                    {
                        const std::size_t start = pos;
                        const unsigned int startCol = column;
                        bool sawDot = false;

                        while (pos < line.size())
                        {
                            const char c = line[pos];
                            if (std::isdigit(static_cast<unsigned char>(c)))
                            {
                                ++pos;
                                ++column;
                            }
                            else if (c == '.' && !sawDot)
                            {
                                sawDot = true;
                                ++pos;
                                ++column;
                            }
                            else
                            {
                                break;
                            }
                        }

                        PushToken(TokenType::Number, line.substr(start, pos - start), lineNo, startCol);
                        continue;
                    }

                    if (ch == '"' || ch == '\'')
                    {
                        const char quote = ch;
                        const unsigned int startCol = column;
                        ++pos;
                        ++column;

                        std::string value;
                        while (pos < line.size() && line[pos] != quote)
                        {
                            if (line[pos] == '\\' && pos + 1 < line.size())
                            {
                                const char escaped = line[pos + 1];
                                switch (escaped)
                                {
                                case 'n':
                                    value.push_back('\n');
                                    break;
                                case 't':
                                    value.push_back('\t');
                                    break;
                                case '"':
                                    value.push_back('"');
                                    break;
                                case '\\':
                                    value.push_back('\\');
                                    break;
                                case '\'':
                                    value.push_back('\'');
                                    break;
                                default:
                                    value.push_back(escaped);
                                    break;
                                }
                                pos += 2;
                                column += 2;
                            }
                            else
                            {
                                value.push_back(line[pos]);
                                ++pos;
                                ++column;
                            }
                        }

                        if (pos >= line.size() || line[pos] != quote)
                        {
                            RaiseLexError("Unterminated string", lineNo, startCol);
                        }

                        ++pos;
                        ++column;
                        PushToken(TokenType::String, value, lineNo, startCol);
                        continue;
                    }

                    auto addToken = [&](TokenType type, const std::string &text, unsigned int width)
                    {
                        PushToken(type, text, lineNo, column);
                        pos += width;
                        column += width;
                    };

                    if (pos + 1 < line.size())
                    {
                        const std::string two = line.substr(pos, 2);
                        if (two == "==")
                        {
                            addToken(TokenType::Equal, two, 2);
                            continue;
                        }
                        if (two == "!=")
                        {
                            addToken(TokenType::NotEqual, two, 2);
                            continue;
                        }
                        if (two == "<=")
                        {
                            addToken(TokenType::LessEqual, two, 2);
                            continue;
                        }
                        if (two == ">=")
                        {
                            addToken(TokenType::GreaterEqual, two, 2);
                            continue;
                        }
                        if (two == "&&")
                        {
                            addToken(TokenType::And, two, 2);
                            continue;
                        }
                        if (two == "||")
                        {
                            addToken(TokenType::Or, two, 2);
                            continue;
                        }
                    }

                    switch (ch)
                    {
                    case '(':
                        addToken(TokenType::LParen, "(", 1);
                        break;
                    case ')':
                        addToken(TokenType::RParen, ")", 1);
                        break;
                    case '{':
                        ++braceDepth;
                        addToken(TokenType::LBrace, "{", 1);
                        break;
                    case '}':
                        --braceDepth;
                        if (braceDepth < 0)
                        {
                            RaiseLexError("Unexpected '}'", lineNo, column);
                        }
                        addToken(TokenType::RBrace, "}", 1);
                        break;
                    case ':':
                        addToken(TokenType::Colon, ":", 1);
                        break;
                    case ',':
                        addToken(TokenType::Comma, ",", 1);
                        break;
                    case ';':
                        addToken(TokenType::Semicolon, ";", 1);
                        break;
                    case '.':
                        addToken(TokenType::Dot, ".", 1);
                        break;
                    case '=':
                        addToken(TokenType::Assign, "=", 1);
                        break;
                    case '+':
                        addToken(TokenType::Plus, "+", 1);
                        break;
                    case '-':
                        addToken(TokenType::Minus, "-", 1);
                        break;
                    case '*':
                        addToken(TokenType::Star, "*", 1);
                        break;
                    case '/':
                        addToken(TokenType::Slash, "/", 1);
                        break;
                    case '%':
                        addToken(TokenType::Percent, "%", 1);
                        break;
                    case '<':
                        addToken(TokenType::Less, "<", 1);
                        break;
                    case '>':
                        addToken(TokenType::Greater, ">", 1);
                        break;
                    case '!':
                        addToken(TokenType::Not, "!", 1);
                        break;
                    default:
                        RaiseLexError("Unexpected character '" + std::string(1, ch) + "'", lineNo, column);
                    }
                }

                PushToken(TokenType::Newline, "\\n", lineNo, static_cast<unsigned int>(line.size()) + 1);
                ++lineNo;
            }

            while (indentStack.size() > 1)
            {
                indentStack.pop_back();
                PushToken(TokenType::Dedent, "<dedent>", lineNo, 1);
            }

            PushToken(TokenType::End, "<eof>", lineNo, 1);
            m_Prev = m_Tokens.front();
        }

        std::string Lexer::Trim(const std::string &line)
        {
            std::size_t start = 0;
            while (start < line.size() && std::isspace(static_cast<unsigned char>(line[start])))
            {
                ++start;
            }

            std::size_t end = line.size();
            while (end > start && std::isspace(static_cast<unsigned char>(line[end - 1])))
            {
                --end;
            }

            return line.substr(start, end - start);
        }

        int Lexer::CountIndent(const std::string &line)
        {
            int count = 0;
            for (char c : line)
            {
                if (c == ' ')
                {
                    ++count;
                }
                else if (c == '\t')
                {
                    count += 4;
                }
                else
                {
                    break;
                }
            }
            return count;
        }

        void Lexer::PushToken(TokenType type, const std::string &text, unsigned int line, unsigned int column)
        {
            m_Tokens.emplace_back(type, text, line, column);
        }

        [[noreturn]] void Lexer::RaiseLexError(const std::string &message, unsigned int line, unsigned int column) const
        {
            error::DiagnosticContext context;
            context.fileName = m_FileName;
            context.moduleName = m_ModuleName;
            context.line = line;
            context.column = column;
            throw error::LexingError(message, context);
        }

        Lexer::~Lexer() = default;

    } // namespace parser

} // namespace cora::compiler
