#!/usr/bin/perl

use Time::Local;

while (<>) {
    chomp;
    print "$_\n";
    if (/^- (\d*):(\d*):(\d*)\.(\d*) /) {
        $time = timelocal($3, $2, $1, (localtime)[3,4,5]) + ($4/1000.0);
        # print "$time\n";
        if (/DEBUG: Starting the count\.\.\./) {
            $startcount = $time;
            print "$startcount\n";
        }
        elsif (/DEBUG: \.\.\. finished counting\./) {
            print "Counting: ",$time-$startcount,"\n";
        }
    }
}
