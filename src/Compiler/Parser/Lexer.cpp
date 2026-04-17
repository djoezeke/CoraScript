#include "Cora/Compiler/Parser/Lexer.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace cora::compiler
{
    namespace parser
    {

        Lexer::Lexer() {
        };

        std::deque<Token> Lexer::Lex(const std::string &source) const
        {
            std::deque<Token> tokens;
            std::istringstream stream(source);
            std::string line;
            std::deque<int> indentStack{0};
            int lineNo = 1;
            int braceDepth = 0;

            const std::unordered_map<std::string, TokenType> keywords = {
                {"if", TokenType::If},
                {"elif", TokenType::Elif},
                {"else", TokenType::Else},
                {"while", TokenType::While},
                {"for", TokenType::For},
                {"in", TokenType::In},
                {"range", TokenType::Range},
                {"break", TokenType::Break},
                {"continue", TokenType::Continue},
                {"pass", TokenType::Pass},
                {"let", TokenType::Let},
                {"int", TokenType::Int},
                {"float", TokenType::Float},
                {"bool", TokenType::Bool},
                {"string", TokenType::StringType},
                {"print", TokenType::Print},
                {"and", TokenType::And},
                {"or", TokenType::Or},
                {"not", TokenType::Not},
                {"null", TokenType::Null},
                {"none", TokenType::Null},
                {"true", TokenType::True},
                {"false", TokenType::False},
            };

            while (std::getline(stream, line))
            {
                const std::string trimmed = Trim(line);
                if (trimmed.empty() || trimmed.rfind("#", 0) == 0)
                {
                    ++lineNo;
                    continue;
                }

                int indent = CountIndent(line);
                std::size_t pos = static_cast<std::size_t>(indent);

                if (braceDepth == 0)
                {
                    if (indent > indentStack.back())
                    {
                        indentStack.push_back(indent);
                        tokens.push_back({TokenType::Indent, "<indent>", lineNo, 1});
                    }
                    while (indent < indentStack.back())
                    {
                        indentStack.pop_back();
                        tokens.push_back({TokenType::Dedent, "<dedent>", lineNo, 1});
                    }
                    if (indent != indentStack.back())
                    {
                        throw std::runtime_error("Indentation mismatch at line " + std::to_string(lineNo));
                    }
                }

                int column = static_cast<int>(pos) + 1;
                while (pos < line.size())
                {
                    char ch = line[pos];
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
                        std::size_t start = pos;
                        int startCol = column;
                        while (pos < line.size() && (std::isalnum(static_cast<unsigned char>(line[pos])) || line[pos] == '_'))
                        {
                            ++pos;
                            ++column;
                        }
                        std::string text = line.substr(start, pos - start);
                        auto keyword = keywords.find(text);
                        tokens.push_back({keyword != keywords.end() ? keyword->second : TokenType::Identifier, text, lineNo, startCol});
                        continue;
                    }

                    if (std::isdigit(static_cast<unsigned char>(ch)))
                    {
                        std::size_t start = pos;
                        int startCol = column;
                        bool sawDot = false;
                        while (pos < line.size())
                        {
                            char c = line[pos];
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
                        tokens.push_back({TokenType::Number, line.substr(start, pos - start), lineNo, startCol});
                        continue;
                    }

                    if (ch == '"' || ch == '\'')
                    {
                        const char quote = ch;
                        int startCol = column;
                        ++pos;
                        ++column;
                        std::string value;
                        while (pos < line.size() && line[pos] != quote)
                        {
                            if (line[pos] == '\\' && pos + 1 < line.size())
                            {
                                char escaped = line[pos + 1];
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
                            throw std::runtime_error("Unterminated string at line " + std::to_string(lineNo));
                        }
                        ++pos;
                        ++column;
                        tokens.push_back({TokenType::String, value, lineNo, startCol});
                        continue;
                    }

                    auto addToken = [&](TokenType type, std::string text, int width)
                    {
                        tokens.push_back({type, std::move(text), lineNo, column});
                        pos += static_cast<std::size_t>(width);
                        column += width;
                    };

                    if (pos + 1 < line.size())
                    {
                        std::string two = line.substr(pos, 2);
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
                            throw std::runtime_error("Unexpected '}' at line " + std::to_string(lineNo));
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
                        throw std::runtime_error("Unexpected character '" + std::string(1, ch) + "' at line " + std::to_string(lineNo));
                    }
                }

                tokens.push_back({TokenType::Newline, "\\n", lineNo, static_cast<int>(line.size()) + 1});
                ++lineNo;
            }

            while (indentStack.size() > 1)
            {
                indentStack.pop_back();
                tokens.push_back({TokenType::Dedent, "<dedent>", lineNo, 1});
            }

            tokens.push_back({TokenType::End, "<eof>", lineNo, 1});
            return tokens;
        };

        std::string Lexer::Trim(const std::string &line) const
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

        int Lexer::CountIndent(const std::string &line) const
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

        Lexer::~Lexer() {
        };
    } // namespace parser

} // namespace cora::compiler

namespace cora::script
{
    namespace
    {
    }
}
