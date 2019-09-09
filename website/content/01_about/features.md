---
title: "Features"
date: 2018-01-27T15:42:17+01:00
anchor: "features"
weight: 15
---

_klogg_ inherited a lot of features from _glogg_. It:

 - Runs on Unix-like systems, Windows and Mac thanks to Qt5
 - Displays search results separately from original file
 - Supports Perl-compatible regular expressions
 - Colorizes the log and search results
 - Displays a context view of where in the log the lines of interest are
 - Is fast and reads the file directly from disk, without loading it into memory
 - Watches for file changes on disk and reloads it (kind of like tail)
 - Is open source, released under the GPL


_klogg_ improves some basic features and brings new. It:

 - Builds initial file index significantly faster
 - Uses multiple CPU cores to do regular expression matching
 - Detects log file encoding using uchardet library (support utf8, utf16, cp1251 and more)
 - Allows to perform search in a portion of huge log file
 - Keeps in-memory cache of search results per search pattern
 - Allows to paste some text from clipboard for further analysis
 - Has portable version for Windows (no need to install)