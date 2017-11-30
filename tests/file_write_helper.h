#pragma once

static const char* sl_format = "LOGDATA is a part of glogg, we are going to test it thoroughly, this is line %06d\n";
static const int SL_LINE_LENGTH = 83; // Without the final '\n' !

static const char* partial_line_begin = "123... beginning of line.";
static const char* partial_line_end = " end of line 123.\n";

enum class WriteFileModification
{
	None = 0,
	StartWithPartialLineEnd = 1,
	EndWithPartialLineBegin = 2,
};