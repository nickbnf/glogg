#!/bin/bash

FILES="logdata.cpp logfiltereddata.cpp logdataworkerthread.cpp logfiltereddataworkerthread.cpp"

for i in $FILES; do
    gcov -b $i | perl -e "while(<>) { if (/^File '.*$i'/) { \$print = 1; }
    if ( \$print ) { if (/:creating '/) { \$print = 0; print \"\n\" } else { print; } } }"
done

mkdir coverage
mv *.gcov coverage/
rm *.{gcda,gcno}
