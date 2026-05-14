#include "Lexer.hpp"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace cora::parser
{

    Lexer::Lexer(SourceManager &sm, DiagnosticEngine &de)
        : m_SourceManager(sm), m_DiagnosticEngine(de), m_Tokens(), m_Position(0), m_Prev(TokenType::End, "<eof>", 1, 1) {}

    Lexer::Lexer(SourceManager &sm, DiagnosticEngine &de, uint32_t fileID)
        : m_SourceManager(sm), m_DiagnosticEngine(de), m_FileID(fileID), m_Tokens(), m_Position(0), m_Prev(TokenType::End, "<eof>", 1, 1)
    {
        BuildTokens();
    }

    std::vector<Token> Lexer::Tokenize()
    {
        if (m_Tokens.empty())
        {
            BuildTokens();
        }
        return m_Tokens;
    }

    void Lexer::SetFileID(uint32_t fileID)
    {
        m_FileID = fileID;
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

        const SourceFile *file = m_SourceManager.getFile(m_FileID);
        if (!file)
            return;

        const std::string &source = file->content;

        std::string normalizedSource;
        normalizedSource.reserve(source.size());
        for (std::size_t i = 0; i < source.size();)
        {
            if (i + 1 < source.size() && source[i] == '/' && source[i + 1] == '*')
            {
                i += 2;
                normalizedSource.push_back(' ');
                normalizedSource.push_back(' ');
                while (i + 1 < source.size() && !(source[i] == '*' && source[i + 1] == '/'))
                {
                    normalizedSource.push_back(source[i] == '\n' ? '\n' : ' ');
                    ++i;
                }
                if (i + 1 < source.size())
                {
                    i += 2;
                    normalizedSource.push_back(' ');
                    normalizedSource.push_back(' ');
                }
                continue;
            }
            normalizedSource.push_back(source[i]);
            ++i;
        }

        static const std::unordered_map<std::string, TokenType> keywords = {
            {"if", TokenType::If},
            {"elif", TokenType::Elif},
            {"else", TokenType::Else},
            {"while", TokenType::While},
            {"for", TokenType::For},
            {"do", TokenType::Do},
            {"func", TokenType::Func},
            {"return", TokenType::Return},
            {"class", TokenType::Class},
            {"enum", TokenType::Enum},
            {"struct", TokenType::Struct},
            {"switch", TokenType::Switch},
            {"match", TokenType::Match},
            {"default", TokenType::Default},
            {"try", TokenType::Try},
            {"catch", TokenType::Catch},
            {"this", TokenType::This},
            {"import", TokenType::Import},
            {"in", TokenType::In},
            {"range", TokenType::Range},
            {"break", TokenType::Break},
            {"continue", TokenType::Continue},
            {"pass", TokenType::Pass},
            {"from", TokenType::From},
            {"as", TokenType::As},
            {"let", TokenType::Let},
            {"const", TokenType::Const},
            {"and", TokenType::And},
            {"or", TokenType::Or},
            {"not", TokenType::Not},
            {"null", TokenType::Null},
            {"none", TokenType::Null},
            {"true", TokenType::True},
            {"false", TokenType::False},
            {"int", TokenType::Int},
            {"str", TokenType::Str},
            {"void", TokenType::Void},
        };

        std::istringstream stream(normalizedSource);
        std::vector<int> indentStack;
        indentStack.push_back(0);

        std::string line;
        uint32_t lineNo = 1;
        int braceDepth = 0;
        uint32_t currentOffset = 0;

        while (std::getline(stream, line))
        {
            uint32_t lineStartOffset = currentOffset;
            currentOffset += static_cast<uint32_t>(line.size()) + 1; // +1 for newline

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
                    PushToken(TokenType::Indent, "<indent>", lineStartOffset, lineNo, 1);
                }
                while (indent < indentStack.back())
                {
                    indentStack.pop_back();
                    PushToken(TokenType::Dedent, "<dedent>", lineStartOffset, lineNo, 1);
                }
                if (indent != indentStack.back())
                {
                    RaiseLexError(ErrorCode::E0004, "Indentation mismatch", lineStartOffset);
                }
            }

            uint32_t column = static_cast<uint32_t>(pos) + 1;
            while (pos < line.size())
            {
                const char ch = line[pos];
                uint32_t charOffset = lineStartOffset + static_cast<uint32_t>(pos);

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
                    const uint32_t startCol = column;
                    const uint32_t startOffset = lineStartOffset + static_cast<uint32_t>(pos);
                    while (pos < line.size() && (std::isalnum(static_cast<unsigned char>(line[pos])) || line[pos] == '_'))
                    {
                        ++pos;
                        ++column;
                    }

                    const std::string text = line.substr(start, pos - start);
                    auto keyword = keywords.find(text);
                    PushToken(keyword != keywords.end() ? keyword->second : TokenType::Identifier, text, startOffset, lineNo, startCol);
                    continue;
                }

                if (std::isdigit(static_cast<unsigned char>(ch)))
                {
                    const std::size_t start = pos;
                    const uint32_t startCol = column;
                    const uint32_t startOffset = lineStartOffset + static_cast<uint32_t>(pos);
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

                    if (sawDot)
                    {
                        PushToken(TokenType::Float,
                                  line.substr(start, pos - start), startOffset, lineNo,
                                  startCol);
                    }
                    else
                    {
                        PushToken(TokenType::Integer,
                                  line.substr(start, pos - start), startOffset, lineNo,
                                  startCol);
                    }
                    continue;
                }

                if (ch == '"' || ch == '\'')
                {
                    const char quote = ch;
                    const uint32_t startCol = column;
                    const uint32_t startOffset = lineStartOffset + static_cast<uint32_t>(pos);

                    if (pos + 2 < line.size() && line[pos + 1] == quote && line[pos + 2] == quote)
                    {
                        pos += 3;
                        column += 3;

                        std::string value;
                        bool closed = false;
                        while (true)
                        {
                            if (pos + 2 < line.size() && line[pos] == quote && line[pos + 1] == quote && line[pos + 2] == quote)
                            {
                                pos += 3;
                                column += 3;
                                closed = true;
                                break;
                            }

                            if (pos < line.size())
                            {
                                value.push_back(line[pos]);
                                ++pos;
                                ++column;
                                continue;
                            }

                            if (!std::getline(stream, line))
                            {
                                break;
                            }

                            currentOffset += static_cast<uint32_t>(line.size()) + 1;
                            value.push_back('\n');
                            ++lineNo;
                            pos = 0;
                            column = 1;
                        }

                        if (!closed)
                        {
                            RaiseLexError(ErrorCode::E0003, "Unterminated triple-quoted string", startOffset);
                            break;
                        }

                        PushToken(TokenType::String, value, startOffset, lineNo, startCol);
                        continue;
                    }

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
                        RaiseLexError(ErrorCode::E0003, "Unterminated string", startOffset);
                        break;
                    }

                    ++pos;
                    ++column;
                    PushToken(TokenType::String, value, startOffset, lineNo, startCol);
                    continue;
                }

                auto addToken = [&](TokenType type, const std::string &text, uint32_t width)
                {
                    PushToken(type, text, charOffset, lineNo, column);
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
                    if (two == "->")
                    {
                        addToken(TokenType::Arrow, two, 2);
                        continue;
                    }
                    if (two == "::")
                    {
                        addToken(TokenType::ScopeResolution, two, 2);
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
                        RaiseLexError(ErrorCode::E0004, "Unexpected '}'", charOffset);
                    }
                    addToken(TokenType::RBrace, "}", 1);
                    break;
                case '[':
                    addToken(TokenType::LBracket, "[", 1);
                    break;
                case ']':
                    addToken(TokenType::RBracket, "]", 1);
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
                case '?':
                    addToken(TokenType::Question, "?", 1);
                    break;
                default:
                    RaiseLexError(ErrorCode::E0001, "Unexpected character '" + std::string(1, ch) + "'", charOffset);
                    pos++;
                    column++;
                }
            }

            PushToken(TokenType::Newline, "\\n", currentOffset - 1, lineNo, static_cast<uint32_t>(line.size()) + 1);
            ++lineNo;
        }

        while (indentStack.size() > 1)
        {
            indentStack.pop_back();
            PushToken(TokenType::Dedent, "<dedent>", currentOffset, lineNo, 1);
        }

        PushToken(TokenType::End, "<eof>", currentOffset, lineNo, 1);
        if (!m_Tokens.empty())
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

    void Lexer::PushToken(TokenType type, const std::string &text, uint32_t offset, uint32_t line, uint32_t column)
    {
        Token token(type, text, line, column);
        SourceLocation start = {m_FileID, offset, line, column};
        SourceLocation end = {m_FileID, offset + static_cast<uint32_t>(text.size()), line, column + static_cast<uint32_t>(text.size())};
        token.SetSourceRange({start, end});
        m_Tokens.push_back(token);
    }

    void Lexer::RaiseLexError(ErrorCode id, const std::string &message, uint32_t offset) const
    {
        SourceLocation loc = m_SourceManager.getLoc(m_FileID, offset);
        m_DiagnosticEngine.report(ErrorDiagnostic(id, message, loc));
    }

    Lexer::~Lexer() = default;

} // namespace cora::parser
