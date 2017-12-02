#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "config.h"

#include "log.h"

#include "data/linepositionarray.h"

using namespace std;
using namespace testing;

class LinePositionArraySmall: public testing::Test {
  public:
    LinePositionArray line_array;

    LinePositionArraySmall() {
        line_array.append( 4_offset );
        line_array.append( 8_offset );
        line_array.append( 10_offset );
        // A longer (>128) line
        line_array.append( 345_offset );
        // An even longer (>16384) line
        line_array.append( 20000_offset );
        // And a short one again
        line_array.append( 20020_offset );
    }
};

TEST_F( LinePositionArraySmall, HasACorrectSize ) {
    ASSERT_THAT( line_array.size(), Eq( 6_lcount ) );
}

TEST_F( LinePositionArraySmall, RememberAddedLines ) {
    ASSERT_THAT( line_array[0], Eq( 4_offset ) );
    ASSERT_THAT( line_array[1], Eq( 8_offset ) );
    ASSERT_THAT( line_array[2], Eq( 10_offset ) );
    ASSERT_THAT( line_array[3], Eq( 345_offset ) );
    ASSERT_THAT( line_array[4], Eq( 20000_offset ) );
    ASSERT_THAT( line_array[5], Eq( 20020_offset ) );

    // This one again to eliminate caching effects
    ASSERT_THAT( line_array[3], Eq( 345_offset ) );
}

TEST_F( LinePositionArraySmall, FakeLFisNotKeptWhenAddingAfterIt ) {
    line_array.setFakeFinalLF();
    ASSERT_THAT( line_array[5], Eq( 20020_offset ) );
    line_array.append( 20030_offset );
    ASSERT_THAT( line_array[5], Eq( 20030_offset ) );
}

class LinePositionArrayConcatOperation: public LinePositionArraySmall {
  public:
    FastLinePositionArray other_array;

    LinePositionArrayConcatOperation() {
        other_array.append( 150000_offset );
        other_array.append( 150023_offset );
    }
};

TEST_F( LinePositionArrayConcatOperation, SimpleConcat ) {
    line_array.append_list( other_array );

    ASSERT_THAT( line_array.size(), Eq( 8_lcount ) );

    ASSERT_THAT( line_array[0], Eq( 4_offset ) );
    ASSERT_THAT( line_array[1], Eq( 8_offset ) );

    ASSERT_THAT( line_array[5], Eq( 20020_offset ) );
    ASSERT_THAT( line_array[6], Eq( 150000_offset ) );
    ASSERT_THAT( line_array[7], Eq( 150023_offset ) );
}

TEST_F( LinePositionArrayConcatOperation, DoesNotKeepFakeLf ) {
    line_array.setFakeFinalLF();
    ASSERT_THAT( line_array[5], Eq( 20020_offset ) );

    line_array.append_list( other_array );
    ASSERT_THAT( line_array[5], Eq( 150000_offset ) );
    ASSERT_THAT( line_array.size(), Eq( 7_lcount ) );
}

class LinePositionArrayLong: public testing::Test {
  public:
    LinePositionArray line_array;

    LinePositionArrayLong() {
        // Add 255 lines (of various sizes
        int lines = 255;
        for ( int i = 0; i < lines; i++ )
            line_array.append( LineOffset( i * 4 ) );
        // Line no 256
        line_array.append( LineOffset( 255 * 4 ) );
    }
};

TEST_F( LinePositionArrayLong, LineNo128HasRightValue ) {
    // Add line no 257
    line_array.append( LineOffset( 255 * 4 + 10 ) );
    ASSERT_THAT( line_array[256], Eq( LineOffset( 255 * 4 + 10 ) ) );
}

TEST_F( LinePositionArrayLong, FakeLFisNotKeptWhenAddingAfterIt ) {
    for ( uint64_t i = 0; i < 1000; ++i ) {
        uint64_t pos = ( 257LL * 4 ) + i*35LL;
        LOG(logDEBUG2) << "Iteration " << i << ", pos " << pos;
        line_array.append( LineOffset( pos ) );
        line_array.setFakeFinalLF();
        ASSERT_THAT( line_array[256 + i].get(), Eq( pos ) );
        LOG(logDEBUG2) << "appended fake lf";
        line_array.append( LineOffset( pos + 21LL ) );
        LOG(logDEBUG2) << "appended pos " << pos + 21LL;
        ASSERT_THAT( line_array[256 + i].get(), Eq( pos + 21LL ) );
    }
}


class LinePositionArrayBig: public testing::Test {
  public:
    LinePositionArray line_array;

    LinePositionArrayBig() {
        line_array.append( 4_offset );
        line_array.append( 8_offset );
        // A very big line
        line_array.append( LineOffset(UINT32_MAX - 10) );
        line_array.append( LineOffset( (uint64_t) UINT32_MAX + 10LL ) );
        line_array.append( LineOffset( (uint64_t) UINT32_MAX + 30LL ) );
        line_array.append( LineOffset( (uint64_t) 2*UINT32_MAX ) );
        line_array.append( LineOffset( (uint64_t) 2*UINT32_MAX + 10LL ) );
        line_array.append( LineOffset( (uint64_t) 2*UINT32_MAX + 1000LL ) );
        line_array.append( LineOffset( (uint64_t) 3*UINT32_MAX ) );
    }
};

TEST_F( LinePositionArrayBig, IsTheRightSize ) {
    ASSERT_THAT( line_array.size(), 9_lcount );
}

TEST_F( LinePositionArrayBig, HasRightData ) {
    ASSERT_THAT( line_array[0].get(), Eq( 4 ) );
    ASSERT_THAT( line_array[1].get(), Eq( 8 ) );
    ASSERT_THAT( line_array[2].get(), Eq( UINT32_MAX - 10 ) );
    ASSERT_THAT( line_array[3].get(), Eq( (uint64_t) UINT32_MAX + 10LL ) );
    ASSERT_THAT( line_array[4].get(), Eq( (uint64_t) UINT32_MAX + 30LL ) );
    ASSERT_THAT( line_array[5].get(), Eq( (uint64_t) 2LL*UINT32_MAX ) );
    ASSERT_THAT( line_array[6].get(), Eq( (uint64_t) 2LL*UINT32_MAX + 10LL ) );
    ASSERT_THAT( line_array[7].get(), Eq( (uint64_t) 2LL*UINT32_MAX + 1000LL ) );
    ASSERT_THAT( line_array[8].get(), Eq( (uint64_t) 3LL*UINT32_MAX ) );
}

TEST_F( LinePositionArrayBig, FakeLFisNotKeptWhenAddingAfterIt ) {
    for ( uint64_t i = 0; i < 1000; ++i ) {
        uint64_t pos = 3LL*UINT32_MAX + 524LL + i*35LL;
        line_array.append( LineOffset( pos ) );
        line_array.setFakeFinalLF();
        ASSERT_THAT( line_array[9 + i].get(), Eq( pos ) );
        line_array.append( LineOffset( pos + 21LL ) );
        ASSERT_THAT( line_array[9 + i].get(), Eq( pos + 21LL ) );
    }
}


class LinePositionArrayBigConcat: public testing::Test {
  public:
    LinePositionArray line_array;
    FastLinePositionArray other_array;

    LinePositionArrayBigConcat() {
        line_array.append( 4_offset );
        line_array.append( 8_offset );

        other_array.append( LineOffset( (uint64_t) UINT32_MAX + 10LL )  );
        other_array.append( LineOffset( (uint64_t) UINT32_MAX + 30LL )  );
    }
};

TEST_F( LinePositionArrayBigConcat, SimpleBigConcat ) {
    line_array.append_list( other_array );

    ASSERT_THAT( line_array.size(), Eq( 4_lcount ) );

    ASSERT_THAT( line_array[0].get(), Eq( 4 ) );
    ASSERT_THAT( line_array[1].get(), Eq( 8 ) );
    ASSERT_THAT( line_array[2].get(), Eq((uint64_t)  UINT32_MAX + 10 ) );
    ASSERT_THAT( line_array[3].get(), Eq((uint64_t)  UINT32_MAX + 30 ) );
}
