---
title: "Switching to Hyperscan"
date: 2021-04-16T01:46+03:00
anchor: "hyperscan"
weight: 29
---

## Swiching to Hyprescan regular expressions engine

### Performance testing

For years Klogg has been using regular expression engine provided by Qt library.
It is based on PCRE2 with JIT compilation. However, recent performance tests have proved
that regular expression matching is a bottleneck. For example, this is a report from `perf` tool
after running a simple string search in 1Gb text file:
```
# Overhead  Command          Shared Object                  Symbol  
    17.10%  Thread (pooled)  libpcre2-16.so.0.10.0          [.] 0x000000000005956f
     7.29%  Thread (pooled)  libpcre2-16.so.0.10.0          [.] 0x000000000005956c
     6.95%  Thread (pooled)  libpcre2-16.so.0.10.0          [.] 0x0000000000059560
     2.89%  Thread (pooled)  libQt5Core.so.5.15.2           [.] 0x0000000000167498
     2.48%  Thread (pooled)  libklogg_tbbmalloc.so          [.] rml::internal::internalPoolMalloc
     2.06%  Thread (pooled)  libklogg_tbbmalloc.so          [.] __TBB_malloc_safer_free
     2.01%  Thread (pooled)  klogg_portable_pcre            [.] std::vector<QString, std::allocator<QString> >::~vector
     1.80%  Thread (pooled)  libpcre2-16.so.0.10.0          [.] pcre2_match_16
     1.70%  Thread (pooled)  libQt5Core.so.5.15.2           [.] QMutex::lock
     1.47%  Thread (pooled)  libQt5Core.so.5.15.2           [.] QRegularExpression::QRegularExpression
     1.39%  klogg_portable_  libc-2.32.so                   [.] 0x000000000015e01f
     1.37%  Thread (pooled)  libQt5Core.so.5.15.2           [.] QMutex::unlock
     1.34%  Thread (pooled)  libpcre2-16.so.0.10.0          [.] pcre2_jit_match_16
     1.13%  Thread (pooled)  libQt5Core.so.5.15.2           [.] QThreadStorageData::get
     1.13%  Thread (pooled)  libQt5Core.so.5.15.2           [.] QRegularExpression::QRegularExpression
```

Most time is spent inside PCRE2 library. Moreover, there is a noticeable impact of QMutex.
Klogg does not use QMutex, so this must be from  QRegularExpression implementation details.

On my development PC the above search takes about 3.5 seconds. From Klogg own logs:
```
Searching done, overall duration 3570.39 ms
Line reading took 814.359 ms
Results combining took 112.078 ms
Matching took 3311.02 ms
Matching took 3343.09 ms
Matching took 3289.59 ms
Matching took 3320.46 ms
Searching perf 2548970 lines/s
Searching io perf 251.642 MiB/s
```


### Hyperscan regular expressions engine

I've done some research about existing regular expressions libraries. 
It seems like PCRE2 is the only one that can do matching directly on UTF-16 encoded strings.
This is important because Klogg uses Qt for text encoding conversions, and QTextCodec can 
only convert input data to UTF-16. In order to use other libraries UTF-16 strings have to
be encoded to UTF-8. That additional overhead has to be taken into account when evaluating
other regular expression engines.

Several articles pointed out that [Hyperscan](https://www.hyperscan.io/) library shows 
very promising results. For example, Rust Leipzig's [research](https://rust-leipzig.github.io/regex/2017/03/28/comparison-of-regex-engines/) claimed that Hyperscan can be 3 times faster than PCRE2.
This result appears in another [article](https://software.intel.com/content/www/us/en/develop/articles/why-and-how-to-replace-pcre-with-hyperscan.html) from Intel. 

I decided that the 3x speedup is so huge, that text re-encoding overhead can be ignored. 
Integrating Hyperscan was rather easy, thanks to good [documentation](http://intel.github.io/hyperscan/dev-reference/).
It is a C library, so some RAII had to be implemented to avoid memory leaks.

The results for the same file now looks like this:
```
Searching done, overall duration 1804.83 ms
Line reading took 907.165 ms
Results combining took 49.838 ms
Matching took 1428.87 ms
Matching took 1398.59 ms
Matching took 1369.43 ms
Matching took 1404.81 ms
Searching perf 5042484 lines/s
Searching io perf 497.81 MiB/s
```

And perf report:
```
# Overhead  Command          Shared Object                  Symbol  
     4.84%  Thread (pooled)  klogg_portable                 [.] std::vector<QString, std::allocator<QString> >::~vector
     3.83%  Thread (pooled)  libklogg_tbbmalloc.so          [.] rml::internal::internalPoolMalloc
     3.15%  Thread (pooled)  klogg_portable                 [.] noodExec
     2.97%  klogg_portable   libc-2.32.so                   [.] 0x000000000015e01f
     2.92%  Thread (pooled)  klogg_portable                 [.] hs_scan
     2.78%  Thread (pooled)  libQt5Core.so.5.15.2           [.] QString::toUtf8_helper
     2.37%  Thread (pooled)  klogg_portable                 [.] HsMatcher::hasMatch
     2.07%  Thread (pooled)  libklogg_tbbmalloc.so          [.] __TBB_malloc_safer_free
     2.05%  Thread (pooled)  klogg_portable                 [.] CompressedLinePositionStorage::at
     1.91%  Thread (pooled)  klogg_portable                 [.] IndexOperation::parseDataBlock
     1.71%  Thread (pooled)  libicuuc.so.68.2               [.] 0x00000000000e86b8
     1.59%  Thread (pooled)  libQt5Core.so.5.15.2           [.] 0x00000000002d8920
     1.49%  Thread (pooled)  libQt5Core.so.5.15.2           [.] QArrayData::allocate
     1.48%  Thread (pooled)  libicuuc.so.68.2               [.] ucnv_toUnicode
     1.26%  Thread (pooled)  klogg_portable                 [.] LogData::decodeLines

```

Now it looks like creating and destroying strings takes more time than actual matching.
Although text encoding overhead is visible (that `QString::toUtf8_helper` line), QMutex related
code is now gone, so that seems to be ok.

Overall search is now about 2 times faster. And there is still room for improvement.
However, there is a downside. Hyperscan library needs boost and ragel to compile.
And compilation takes a lot of time. CI builds now run 2 times slower.

One more drawback of Hyprescan library is that does not support full PCRE2 syntax. 
In particular these constructs are not supported:

 - Backreferences and capturing sub-expressions.
 - Arbitrary zero-width assertions.
 - Subroutine references and recursive patterns.
 - Conditional patterns.
 - Backtracking control verbs.
 - The \C “single-byte” directive (which breaks UTF-8 sequences).
 - The \R newline match.
 - The \K start of match reset directive.
 - Callouts and embedded code.
 - Atomic grouping and possessive quantifiers.

QRegularExpression also does not support full PCRE2 syntax. However, in case these patterns do work with Qt engine, there is an option in Advanced settings to switch back to it.

Although, switching regular expression engine seemed to be quite easy I expect next few
development builds to be somewhat less stable. So any feedback is very welcome.