#include "Cora/Basic/Location.hpp"

namespace cora::basic
{

    SourceLocation::SourceLocation()
    {
        m_line = 0;
        m_Column = 0;
    };

    SourceLocation::SourceLocation(unsigned int line)
    {
        m_line = line;
        m_Column = 0;
    };

    SourceLocation::SourceLocation(unsigned int line, unsigned int column)
    {
        m_line = line;
        m_Column = column;
    };

    unsigned int SourceLocation::Line() const { return m_line; };
    unsigned int SourceLocation::Column() const { return m_Column; };

    bool SourceLocation::Valid() const { return m_line != 0 && m_Column != 0; };

    bool SourceLocation::operator==(const SourceLocation &other) const { return m_line == other.m_line && m_Column == other.m_Column; };

    bool SourceLocation::operator!=(const SourceLocation &other) const { return !(*this == other); };

    SourceRange::SourceRange() {};

    SourceRange::SourceRange(SourceLocation loc)
    {
        m_Start = loc;
        m_End = loc;
    };

    SourceRange::SourceRange(SourceLocation start, SourceLocation end)
    {
        m_Start = start;
        m_End = end;
    };

    SourceLocation SourceRange::GetStart() const { return m_Start; };

    SourceLocation SourceRange::GetEnd() const { return m_End; };

    void SourceRange::SetStart(SourceLocation start) noexcept { m_Start = start; };
    void SourceRange::SetEnd(SourceLocation end) noexcept { m_End = end; };

    bool SourceRange::Valid() const { return m_Start.Valid() && m_End.Valid(); };
    bool SourceRange::Invalid() const { return !Valid(); };

    bool SourceRange::operator==(const SourceRange &other) const { return m_Start == other.m_Start && m_End == other.m_End; };

    bool SourceRange::operator!=(const SourceRange &other) const { return !(*this == other); };

    CharSourceRange::CharSourceRange() = default;

    CharSourceRange::CharSourceRange(SourceLocation start, unsigned int bytelenght)
    {
        m_Start = start;
        m_ByteLenght = bytelenght;
    };

    CharSourceRange::CharSourceRange(SourceLocation start, SourceLocation end)
    {
        m_Start = start;
        m_ByteLenght = end.Column() - m_Start.Column();
    };

    bool CharSourceRange::Valid() const { return m_Start.Valid() && m_ByteLenght > 0; };

    bool CharSourceRange::Invalid() const { return !Valid(); };

    SourceLocation CharSourceRange::Start() const { return m_Start; };

    SourceLocation CharSourceRange::End() const { return SourceLocation(m_Start.Line(), m_Start.Column() + m_ByteLenght); };

    bool CharSourceRange::operator==(const CharSourceRange &other) const { return m_Start == other.m_Start && m_ByteLenght == other.m_ByteLenght; };

    bool CharSourceRange::operator!=(const CharSourceRange &other) const { return !(*this == other); };

    template <typename T>
    Location<T>::Location(T item, SourceLocation loc)
        : m_Item(item), m_Loc(loc)
    {
    }

    template <typename T>
    T Location<T>::Item() const
    {
        return m_Item;
    }

    template <typename T>
    SourceLocation Location<T>::Located() const
    {
        return m_Loc;
    }

    template <typename T>
    bool operator==(const Location<T> &lhs, const Location<T> &rhs)
    {
        return lhs.Item() == rhs.Item() && lhs.Located() == rhs.Located();
    }

    template <typename T>
    bool operator!=(const Location<T> &lhs, const Location<T> &rhs)
    {
        return !(lhs == rhs);
    }

} // namespace cora::basic
