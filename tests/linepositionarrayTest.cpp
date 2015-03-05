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
        line_array.append( 4 );
        line_array.append( 8 );
        line_array.append( 10 );
        // A longer (>128) line
        line_array.append( 345 );
        // An even longer (>16384) line
        line_array.append( 20000 );
        // And a short one again
        line_array.append( 20020 );
    }
};

TEST_F( LinePositionArraySmall, HasACorrectSize ) {
    ASSERT_THAT( line_array.size(), Eq( 6 ) );
}

TEST_F( LinePositionArraySmall, RememberAddedLines ) {
    ASSERT_THAT( line_array[0], Eq( 4 ) );
    ASSERT_THAT( line_array[1], Eq( 8 ) );
    ASSERT_THAT( line_array[2], Eq( 10 ) );
    ASSERT_THAT( line_array[3], Eq( 345 ) );
    ASSERT_THAT( line_array[4], Eq( 20000 ) );
    ASSERT_THAT( line_array[5], Eq( 20020 ) );
}

TEST_F( LinePositionArraySmall, FakeLFisNotKeptWhenAddingAfterIt ) {
    line_array.setFakeFinalLF();
    ASSERT_THAT( line_array[2], Eq( 10 ) );
    line_array.append( 15 );
    ASSERT_THAT( line_array[2], Eq( 15 ) );
}

class LinePositionArrayConcatOperation: public LinePositionArraySmall {
  public:
    LinePositionArray other_array;

    LinePositionArrayConcatOperation() {
        other_array.append( 15 );
        other_array.append( 23 );
    }
};

TEST_F( LinePositionArrayConcatOperation, SimpleConcat ) {
    line_array.append_list( other_array );

    ASSERT_THAT( line_array.size(), Eq( 5 ) );

    ASSERT_THAT( line_array[0], Eq( 4 ) );
    ASSERT_THAT( line_array[1], Eq( 8 ) );
    ASSERT_THAT( line_array[2], Eq( 10 ) );
    ASSERT_THAT( line_array[3], Eq( 15 ) );
    ASSERT_THAT( line_array[4], Eq( 23 ) );
}

TEST_F( LinePositionArrayConcatOperation, DoesNotKeepFakeLf ) {
    line_array.setFakeFinalLF();
    ASSERT_THAT( line_array[2], Eq( 10 ) );

    line_array.append_list( other_array );
    ASSERT_THAT( line_array[2], Eq( 15 ) );
    ASSERT_THAT( line_array.size(), Eq( 4 ) );
}

class LinePositionArrayBig: public testing::Test {
  public:
    LinePositionArray line_array;

    LinePositionArrayBig() {
        line_array.append( 4 );
        line_array.append( 8 );
        // A very big line
        line_array.append( UINT32_MAX - 10 );
        line_array.append( UINT32_MAX + 10 );
        line_array.append( UINT32_MAX + 30 );
    }
};

TEST_F( LinePositionArrayBig, IsTheRightSize ) {
    ASSERT_THAT( line_array.size(), 5 );
}

TEST_F( LinePositionArrayBig, HasRightData ) {
    ASSERT_THAT( line_array[0], Eq( 4 ) );
    ASSERT_THAT( line_array[1], Eq( 8 ) );
    ASSERT_THAT( line_array[2], Eq( UINT32_MAX - 10 ) );
    ASSERT_THAT( line_array[3], Eq( UINT32_MAX + 10 ) );
    ASSERT_THAT( line_array[4], Eq( UINT32_MAX + 30 ) );
}

class LinePositionArrayBigConcat: public testing::Test {
  public:
    LinePositionArray line_array;
    LinePositionArray other_array;

    LinePositionArrayBigConcat() {
        line_array.append( 4 );
        line_array.append( 8 );

        other_array.append( UINT32_MAX + 10 );
        other_array.append( UINT32_MAX + 30 );
    }
};

TEST_F( LinePositionArrayBigConcat, SimpleBigConcat ) {
    line_array.append_list( other_array );

    ASSERT_THAT( line_array.size(), Eq( 4 ) );

    ASSERT_THAT( line_array[0], Eq( 4 ) );
    ASSERT_THAT( line_array[1], Eq( 8 ) );
    ASSERT_THAT( line_array[2], Eq( UINT32_MAX + 10 ) );
    ASSERT_THAT( line_array[3], Eq( UINT32_MAX + 30 ) );
}
