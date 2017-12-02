#pragma once
#include <named_type/named_type.hpp>
#include <limits>
#include <nonstd/optional.hpp>
#include <plog/Record.h>

#include <QMetaType>

template <typename T>
struct ConvertibleTo : fluent::crtp<T, ConvertibleTo>
{
    template<typename Destination>
    Destination as() const
    {
        return Destination ( this->underlying().get() );
    }
};

using LineOffset = fluent::NamedType<int64_t, struct line_offset,
                            fluent::Addable, fluent::Incrementable,
                            fluent::Subtractable,
                            fluent::Comparable, fluent::Printable>;

using LineNumber = fluent::NamedType<uint32_t, struct line_number,
                            fluent::Incrementable,
                            fluent::Decrementable,
                            fluent::Comparable, fluent::Printable>;

using LinesCount = fluent::NamedType<uint32_t, struct lines_count,
                            fluent::Addable, fluent::Incrementable,
                            fluent::Subtractable, fluent::Decrementable,
                            fluent::Comparable, fluent::Printable>;

using LineLength = fluent::NamedType<int, struct lines_count,
                            fluent::Comparable, fluent::Printable>;

inline LineOffset operator"" _offset( unsigned long long int value )
{ return LineOffset( static_cast<LineOffset::UnderlyingType>( value ) ); }
inline LineNumber operator"" _lnum (unsigned long long int value )
{ return LineNumber( static_cast<LineNumber::UnderlyingType>( value ) ); }
inline LinesCount operator"" _lcount( unsigned long long int value )
{ return LinesCount( static_cast<LinesCount::UnderlyingType>( value ) ); }
inline LineLength operator"" _length( unsigned long long int value )
{ return LineLength( static_cast<LineLength::UnderlyingType>( value ) ); }

template<typename StrongType>
StrongType maxValue()
{
    return StrongType( std::numeric_limits<typename StrongType::UnderlyingType>::max() );
}

using OptionalLineNumber = nonstd::optional<LineNumber>;

template <typename T, typename Parameter, template<typename> class... Skills>
plog::util::nstringstream& operator<<( plog::util::nstringstream& os, fluent::NamedType<T, Parameter, Skills...> const& object )
{
    os << object.get();
    return os;
}

namespace plog {

    inline Record& operator<<( Record& record, const OptionalLineNumber& t )
    {
        if (t) {
            t->print(record);
        }
        else {
            record << "none";
        }

        return record;
    }

}

// Represents a position in a file (line, column)
class FilePosition
{
  public:
    FilePosition() : column_{ -1 } {}
    FilePosition( LineNumber line, int column )
        : line_ {line}
        , column_ {column}
    {}

    LineNumber line() const { return line_; }
    int column() const { return column_; }

  private:
    LineNumber line_;
    int column_;
};

inline LineNumber operator+(const LineNumber& number, const LinesCount& count)
{
    uint64_t line = number.get() + count.get();
    return line > maxValue<LineNumber>().get()
            ? maxValue<LineNumber>()
            : LineNumber( static_cast<LineNumber::UnderlyingType>(line) );
}

inline LineNumber operator-(const LineNumber& number, const LinesCount& count)
{
    int64_t line = number.get() - count.get();
    return line >= 0
            ? LineNumber( static_cast<LineNumber::UnderlyingType>( line ) )
            : LineNumber( 0u );
}

inline LinesCount operator-(const LineNumber& n1, const LineNumber& n2)
{
    int64_t count = n1.get() - n2.get();
    return count >= 0
            ? LinesCount( static_cast<LinesCount::UnderlyingType>( count ) )
            : LinesCount( 0u );
}

inline bool operator<( const LineNumber& number, const LinesCount& count ) { return number.get() < count.get(); }
inline bool operator>( const LineNumber& number, const LinesCount& count ) { return count.get() < number.get(); }
inline bool operator<=( const LineNumber& number, const LinesCount& count ) { return !(count.get() < number.get());}
inline bool operator>=( const LineNumber& number, const LinesCount& count ) { return !(number < count); }
inline bool operator==( const LineNumber& number, const LinesCount& count ) { return !(number < count) && !(count.get() < number.get()); }
inline bool operator!=( const LineNumber& number, const LinesCount& count ) { return !(number == count); }

Q_DECLARE_METATYPE(LineLength);
Q_DECLARE_METATYPE(LineNumber);
Q_DECLARE_METATYPE(LinesCount);

// Use a bisection method to find the given line number
// in a sorted list.
// The T type must be a container containing elements that
// implement the lineNumber() member.
// Returns true if the lineNumber is found, false if not
// foundIndex is the index of the found number or the index
// of the closest greater element.
template <typename T> bool lookupLineNumber(
        const T& list, LineNumber lineNumber, uint32_t* foundIndex )
{
    auto minIndex = 0u;
    auto maxIndex = static_cast<uint32_t>( list.size() - 1 );

    if ( list.empty() ) {
        *foundIndex = 0;
        return false;
    }

    // If the list is not empty
    // First we test the ends
    if ( list[minIndex].lineNumber() == lineNumber ) {
        *foundIndex = minIndex;
        return true;
    }
    else if ( list[maxIndex].lineNumber() == lineNumber ) {
        *foundIndex = maxIndex;
        return true;
    }

    // Then we test the rest
    while ( (maxIndex - minIndex) > 1 ) {
        const int tryIndex = (minIndex + maxIndex) / 2;
        const auto currentMatchingNumber =
            list[tryIndex].lineNumber();
        if ( currentMatchingNumber > lineNumber )
            maxIndex = tryIndex;
        else if ( currentMatchingNumber < lineNumber )
            minIndex = tryIndex;
        else if ( currentMatchingNumber == lineNumber ) {
            *foundIndex = tryIndex;
            return true;
        }
    }

    // If we haven't found anything...
    // ... end of the list or before the next
    if ( lineNumber > list[maxIndex].lineNumber() )
        *foundIndex = maxIndex + 1;
    else if ( lineNumber > list[minIndex].lineNumber() )
        *foundIndex = minIndex + 1;
    else
        *foundIndex = minIndex;


    return false;
}

template<typename Iterator>
LineNumber lookupLineNumber( Iterator begin, Iterator end, LineNumber lineNum )
{
    const auto lowerBound = std::lower_bound( begin, end, lineNum );
    return LineNumber( static_cast<LineNumber::UnderlyingType>( std::distance( begin, lowerBound ) ) );
}
