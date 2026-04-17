#ifndef CORA_COMPILER_BASIC_SOURCE_LOCATION_H
#define CORA_COMPILER_BASIC_SOURCE_LOCATION_H

namespace cora::compiler
{
    namespace basic
    {
        class SourceManager;

        class SourceLocation
        {
            friend class SourceRange;
            friend class SourceManager;
            friend class CharSourceRange;

        public:
            SourceLocation();

            SourceLocation(unsigned int line);

            SourceLocation(unsigned int line, unsigned int column);

            unsigned int Line() const;

            unsigned int Column() const;

            bool Valid() const;

            bool operator==(const SourceLocation &other) const;
            bool operator!=(const SourceLocation &other) const;

        private:
            unsigned int m_line;
            unsigned int m_Column;
        };

        class SourceRange
        {
        public:
            SourceRange();

            SourceRange(SourceLocation loc);

            SourceRange(SourceLocation start, SourceLocation end);

            SourceLocation GetStart() const;

            SourceLocation GetEnd() const;

            void SetStart(SourceLocation start) noexcept;
            void SetEnd(SourceLocation end) noexcept;

            bool Valid() const;
            bool Invalid() const;

            bool operator==(const SourceRange &other) const;

            bool operator!=(const SourceRange &other) const;

        private:
            SourceLocation m_Start;
            SourceLocation m_End;
        };

        class CharSourceRange
        {
        public:
            CharSourceRange();

            CharSourceRange(SourceLocation start, unsigned int bytelenght);

            CharSourceRange(SourceLocation start, SourceLocation end);

            bool Valid() const;

            bool Invalid() const;

            SourceLocation Start() const;

            SourceLocation End() const;

            bool operator==(const CharSourceRange &other) const;

            bool operator!=(const CharSourceRange &other) const;

        private:
            SourceLocation m_Start;
            unsigned int m_ByteLenght;
        };

        template <typename T>
        class Location
        {
        public:
            Location(T item, SourceLocation loc);

            T Item() const;

            SourceLocation Located() const;

        private:
            T m_Item;
            SourceLocation m_Loc;
        };

        template <typename T>
        bool operator==(const Location<T> &lhs, const Location<T> &rhs);

        template <typename T>
        bool operator!=(const Location<T> &lhs, const Location<T> &rhs);

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

    } // namespace basic

} // namespace cora::compiler

#endif // CORA_COMPILER_BASIC_SOURCE_LOCATION_H