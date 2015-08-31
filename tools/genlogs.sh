#!/bin/bash

# Generate a set of test logs

# 5 million lines - 1, 429Mb
perl -e 'for ($i = 0; $i < 4999999; $i++)
    { printf "LOGDATA is a part of LogCrawler, we are going to test it thoroughly, this is line %06d\n", $i; }'\
    >/tmp/verybiglog.txt

# 100 million lines - 1, Mb
# perl -e 'for ($i = 0; $i < 100000000; $i++)
#     { printf "LOGDATA is a part of LogCrawler, we are going to test it thoroughly, this is line %07d\n", $i; }'\
#     >~/stupidlybiglog.txt

