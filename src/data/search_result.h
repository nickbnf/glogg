#ifndef SEARCH_RESULT_H
#define SEARCH_RESULT_H

#include <vector>
// Line number are unsigned 32 bits for now.
typedef uint32_t LineNumber;

// Class encapsulating a single matching line
// Contains the line number the line was found in and its content.
class MatchingLine {
  public:
    MatchingLine() = default;
    MatchingLine( LineNumber line ) { lineNumber_ = line; }

    // Accessors
    LineNumber lineNumber() const { return lineNumber_; }

    bool operator <( const MatchingLine& other) const
    { return lineNumber_ < other.lineNumber_; }

    bool operator == (const MatchingLine& other) const
    { return lineNumber_ == other.lineNumber_; }

    void addOffset(int offset){ lineNumber_ += offset; }

  private:
    LineNumber lineNumber_;
};

// This is an array of matching lines.
// It shall be implemented for random lookup speed, so
// a fixed "in-place" array (vector) is probably fine.
typedef std::vector<MatchingLine> SearchResultArray;

#endif // SEARCH_RESULT_H
