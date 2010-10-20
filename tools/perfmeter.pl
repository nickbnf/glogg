#!/usr/bin/perl

# Take a debug log from logcrawler and output some perf statistics
# Can be plotted by echo "plot [ ] [0:0.1] 'foo.data'; pause mouse key;" | gnuplot -

# Or an histogram:
# plot './qvector.data' using ((floor($1/50)+0.5)*50):(1) smooth frequency w histeps, './qvar_default.data' using ((floor($1/50)+0.5)*50):(1) smooth frequency w histeps, './qvar_50000.data' using ((floor($1/50)+0.5)*50):(1) smooth frequency w histeps
# Better: plot './0.6.0-3.data' using ((floor($1/0.005)+0.1)*0.005):(0.1) smooth frequency w histeps

while (<>) {
    strip;
    if (/(\d\d\.\d\d\d) DEBUG4.*paintEvent.*firstLine=(\d+) lastLine=(\d+) /) {
        if ( ($3 - $2) > 35 ) {
            $beginning = $1;
            $first_line = $2;
        }
    }
    elsif (/(\d\d\.\d\d\d) DEBUG4.*End/) {
        if ($beginning) {
            $time = $1 - $beginning;
            # print "$first_line $time\n";
            if ($time > 0) {
                print "$time\n";
            }
        }
    }
}
