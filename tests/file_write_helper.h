#pragma once

static const char* partial_line_begin = "123... beginning of line.";
static const char* partial_line_end = " end of line 123.\n";
static const int SL_LINE_LENGTH = 83; // Without the final '\n' !


enum class WriteFileModification
{
	None = 0,
	StartWithPartialLineEnd = 1,
	EndWithPartialLineBegin = 2,
	DelayClosingFile = 4,
};
