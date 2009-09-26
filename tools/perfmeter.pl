#!/usr/bin/perl

# Take a debug log from logcrawler and output some perf statistics

while (<>) {
    strip;
    if (/(\d\d\.\d\d\d) DEBUG: paintEvent.*firstLine=(\d+) lastLine=(\d+) /) {
        if ( ($3 - $2) > 35 ) {
            $beginning = $1;
            $first_line = $2;
        }
    }
    elsif (/(\d\d\.\d\d\d) DEBUG: End/) {
        if ($beginning) {
            $time = $1 - $beginning;
            print "Line $first_line, Time: $time\n";
        }
    }
}
