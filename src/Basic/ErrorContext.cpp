#include "Cora/Common/ErrorContext.hpp"

#include <sstream>

namespace cora::common
{
    std::string_view SourceContext::Line(std::size_t lineNumber) const
    {
        if (lineNumber == 0)
        {
            return {};
        }

        std::size_t currentLine = 1;
        std::size_t start = 0;
        for (std::size_t i = 0; i <= m_Source.size(); ++i)
        {
            if (i == m_Source.size() || m_Source[i] == '\n')
            {
                if (currentLine == lineNumber)
                {
                    return std::string_view(m_Source).substr(start, i - start);
                }
                ++currentLine;
                start = i + 1;
            }
        }

        return {};
    }

    std::optional<std::size_t> SourceContext::LineCount() const
    {
        if (m_Source.empty())
        {
            return 0u;
        }

        std::size_t lines = 1;
        for (char c : m_Source)
        {
            if (c == '\n')
            {
                ++lines;
            }
        }

        return lines;
    }
}
