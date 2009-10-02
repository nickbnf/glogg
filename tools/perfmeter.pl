#!/usr/bin/perl

# Take a debug log from logcrawler and output some perf statistics
# Can be plotted by echo "plot [ ] [0:0.1] 'foo.data'; pause mouse key;" | gnuplot -

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
            print "$first_line $time\n";
        }
    }
}
