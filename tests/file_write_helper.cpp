#include <QFile>

#include "file_write_helper.h"

int main( int argc, const char** argv )
{
	QFile file{ argv[1] };

	file.open( QIODevice::Unbuffered | QIODevice::WriteOnly | QIODevice::Append );

	if ( !file.isOpen() ) {
		return -1;
	}
	
	const int numberOfLines = atoi( argv[2] );
	const auto flag = static_cast<WriteFileModification>( atoi( argv[3] ) );

	if ( flag == WriteFileModification::StartWithPartialLineEnd )
	{
		file.write( partial_line_end, qstrlen( partial_line_end ) );
	}

	char newLine[90];
	for ( int i = 0; i < numberOfLines; i++ ) {
		snprintf( newLine, 89, sl_format, i );
		file.write( newLine, qstrlen( newLine ) );
	}

	if ( flag == WriteFileModification::EndWithPartialLineBegin )
	{
		file.write( partial_line_begin, qstrlen( partial_line_begin ) );
	}

	return 0;
}