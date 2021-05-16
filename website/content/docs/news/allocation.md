---
title: "Allocation matters"
date: 2021-04-16T01:46+03:00
anchor: "alloacation"
weight: 27
---

## Improving memory allocation

### Performance testing

Studying final perf tool report from previous post after switching to Hyperscan
I've noticed some strange lines. Here it is once again:
```
# Overhead  Command          Shared Object                  Symbol  
     4.84%  Thread (pooled)  klogg_portable                 [.] std::vector<`QString`, std::allocator<`QString`> >::~vector
     3.83%  Thread (pooled)  libklogg_tbbmalloc.so          [.] rml::internal::internalPoolMalloc
     3.15%  Thread (pooled)  klogg_portable                 [.] noodExec
     2.97%  klogg_portable   libc-2.32.so                   [.] 0x000000000015e01f
     2.92%  Thread (pooled)  klogg_portable                 [.] hs_scan
     2.78%  Thread (pooled)  libQt5Core.so.5.15.2           [.] `QString`::toUtf8_helper
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

It is easy to notice that top lines belong to the memory management code.
Top one is destroying a vector of `QString` objects and second one is from the 
internal TBB malloc code. There is also some impact from the __TBB_malloc_safer_free.

### Reducing memory allocations

Perf report shows that a lot of time is wasted destroying vectors of `QString`.
When search is executing there is one thread that reads raw data from the file,
and several threads that do searching through the blocks of lines. Block size is
configurable in settings. Typically it is 5000 or 10000 lines. When a thread
gets new raw data from file, it first transforms raw bytes to a vector of
`QString` objects converting each line from file text encoding to UTF16 (internal
Qt string representation). Finally each line is converted to UTF8 so it can be
passed to the Hyperscan regex matching engine.

So during search operation klogg creates and destroys `QString` and `QByteArray` 
objects for each line in the file. `QString` is used to do conversion from 
the file encoding to Qt internal representation and `QByteArray` holds
UTF8 data for the Hyperscan engine.

All these allocations can be avoided by modifying encoding conversion algorithm.
A thread receives a block of raw bytes. That block is guaranteed to
contain several full lines and end with `\n` symbol. That allows to convert
the whole block to a single `QString` as it is known that the block starts and
ends on a multi-byte encoding boundaries. Then `QString` can be converted
to the UTF8 `QByteArray` in one go. Now a searching thread has a single `QByteArray`
with multiple UTF8-encoded stings inside. Using very fast `std::memchr` function a 
vector of `std::string_view` objects is created. Each `string_view` corresponds to
single line of original data block. 

So number of allocations is reduced from two per line to two per single search block.
Search blocks containing at least 1000 lines leads to several orders of magnitude 
less memory allocations. On my test machine this provides about 20% performance
improvement.

### Switching application-wide memory allocator

Klogg has been using the scalable memory allocator provided by the Intel
TBB library for several years. It is designed to work well for multi-threaded
applications. Using it instead of the default system memory allocator resulted in
5-10% performance improvement. It is very easy to use the TBB malloc to override
all calls to malloc/free/new/delete in an application. A program needs to
link with the tbbmalloc_proxy dynamic library. And then all memory allocation
is done by the TBB allocator.

Although the TBB scalable allocator is well-built and has good performance
it is quite complex and designed for cases when there are a lot of threads 
running and competing for memory allocation. On the other hand klogg uses
one thread per CPU core and number of memory allocations is relatively low.

I've discovered an alternative general purpose allocator: [mimalloc](https://github.com/microsoft/mimalloc).
It is described as a general purpose allocator with excellent performance characteristics.
From [performance tests](https://github.com/microsoft/mimalloc#performance) provided
by authors it looks like mimalloc should be faster than the TBB malloc in many case so 
I've decided to try it.

Overriding global malloc/free/new/delete functions with mimalloc is easy on Windows.
The steps are nearly identical to the TBB malloc: link with mimalloc.dll and copy
mimalloc-redirect.dll provided in github repository near application executable.

On Unix-like systems the process is a little more difficult. Dynamic override requires
use of the LD_PRELOAD environment variable. So I used static override option. To do this
klogg executable is linked with the mimalloc-override.o file. This file must be provided first so
the linker chooses symbols from it for all global memory allocation functions.

After some more performance testing it looks like the mimalloc allocator is indeed faster in typical
klogg use cases. Both indexing and search are around 5% faster.

Final perf report now looks like this:
```
# Overhead  Command          Shared Object                  Symbol
     9.04%  Thread (pooled)  libQt5Core.so.5.15.2           [.] 0x00000000002d8920
     4.74%  Thread (pooled)  libhs.so.5.4.0                 [.] hs_scan
     4.53%  klogg_portable_  libc-2.32.so                   [.] 0x000000000015e01f
     4.43%  Thread (pooled)  libicuuc.so.68.2               [.] 0x00000000000e86d0
     3.47%  Thread (pooled)  klogg_portable                 [.] CompressedLinePositionStorage::at
     1.91%  Thread (pooled)  libc-2.32.so                   [.] 0x0000000000156f21
     1.88%  Thread (pooled)  libc-2.32.so                   [.] 0x000000000015e42d
     1.77%  Thread (pooled)  libc-2.32.so                   [.] 0x0000000000156f31
     1.45%  Thread (pooled)  klogg_portable                 [.] std::__detail::__variant::__gen_vtable_impl<std::__detail::__variant::_Multi_array<std::__detail::__variant::__deduce_visit_result<bool> (*)((anonymous namespace)::filterLines(std::variant<DefaultRegularExpressionMatcher, HsMatcher> const&, LogData::RawLines const&, fluent::NamedTypeImpl<unsigned int, line_number, fluent::ConvertWithRatio<unsigned int, std::ratio<1l, 1l> >, fluent::Incrementable, fluent::Decrementable, fluent::Comparable, fluent::Printable>)::{lambda(auto:1 const&)#1}&&, std::variant<DefaultRegularExpressionMatcher, HsMatcher> const&)>, std::integer_sequence<unsigned long, 1ul> >::__visit_invoke
     1.40%  Thread (pooled)  klogg_portable                 [.] (anonymous namespace)::block_next_pos<unsigned int>
     1.38%  klogg_portable_  libQt5Gui.so.5.15.2            [.] 0x000000000049c8ff
     1.35%  Thread (pooled)  libc-2.32.so                   [.] 0x0000000000156f42
     1.31%  Thread (pooled)  klogg_portable                 [.] LogData::getLinesRaw
     1.31%  Thread (pooled)  klogg_portable                 [.] tbb::detail::r1::task_dispatcher::receive_or_steal_task<false, tbb::detail::r1::outermost_worker_waiter>
     1.17%  Thread (pooled)  klogg_portable                 [.] SearchOperation::doSearch(SearchData&, fluent::NamedTypeImpl<unsigned int, line_number, fluent::ConvertWithRatio<unsigned int, std::ratio<1l, 1l> >, fluent::Incrementable, fluent::Decrementable, fluent::Comparable, fluent::Printable>)::{lambda(std::shared_ptr<(anonymous namespace)::SearchBlockData> const&)#3}::operator()
     1.10%  Thread (pooled)  libstdc++.so.6.0.28            [.] std::_Hash_bytes
     1.05%  Thread (pooled)  libicuuc.so.68.2               [.] 0x00000000000e870e
```

Unfortunately I didn't have any success using either mimalloc or TBB malloc as global
memory allocator on MacOS. Klogg crashes with segmentation fault right at the start. So
on MacOS the default system memory allocator is used.

Tweaking encoding conversion algorithm is a big change, so several next CI releases might be less
stable than usual. Please report any bugs and crashes either on [GitHub](https://github.com/variar/klogg/issues)
or in [Discord](https://discord.gg/DruNyQftzB).




