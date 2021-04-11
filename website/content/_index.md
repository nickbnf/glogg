---
title: "About"
type: docs
---

## Faster log explorer

_klogg_ is a multi-platform GUI application to search through all kinds of text log files using regular expressions. It is fork of [glogg project](https://glogg.bonnefon.org/) created by [Nicolas Bonnefon](https://github.com/nickbnf).

_klogg_ is designed to:

 - be fast
 - handle huge log files
 - provide a clear view of the matches even in heavily cluttered files.

Here is what _klogg_ looks like:

<div id="gallery" style="display:none;">
<img src="/screenshots/mainwindow.png" alt="Klogg main window" />
<img src="/screenshots/dark.png" alt="Klogg dark theme" />
<img src="/screenshots/scratchpad.png" alt="Klogg scratchpad" />
<img src="/screenshots/highlighters.png" alt="Klogg highlighters configuration" />
</div>

## Features
{{< columns >}}
 _klogg_ inherited a lot of features from _glogg_

 - Runs on Unix-like systems, Windows and Mac thanks to Qt5
 - Displays search results separately from original file
 - Supports Perl-compatible regular expressions
 - Colorizes the log and search results
 - Displays a context view of where in the log the lines of interest are
 - Is fast and reads the file directly from disk, without loading it into memory
 - Watches for file changes on disk and reloads it (kind of like tail)
 - Is open source, released under the GPL

<--->

_klogg_ improves and brings more

 - Builds initial file index significantly faster
 - Uses multiple CPU cores to do regular expression matching
 - Detects log file encoding using uchardet library (support utf8, utf16, cp1251 and more)
 - Allows to perform search in a portion of huge log file
 - Keeps in-memory cache of search results per search pattern
 - Allows to paste some text from clipboard for further analysis
 - Has portable version for Windows (no need to install)

{{< /columns >}}

## Downloads

Latest stable version:

[ ![GitHub Release](https://img.shields.io/github/v/release/variar/klogg?label=GitHub%20Release&style=for-the-badge)](https://github.com/variar/klogg/releases/latest)
[ ![Chocolatey](https://img.shields.io/chocolatey/v/klogg?style=for-the-badge)](https://chocolatey.org/packages/klogg)
[ ![Homebrew](https://img.shields.io/homebrew/cask/v/klogg?style=for-the-badge)](https://formulae.brew.sh/cask/klogg)


Latest development builds can be downloaded from releases on Github: 

{{< button href="https://github.com/variar/klogg/releases/tag/continuous-win" >}}Windows{{< /button >}}
{{< button href="https://github.com/variar/klogg/releases/tag/continuous-linux" >}}Linux{{< /button >}}
{{< button href="https://github.com/variar/klogg/releases/tag/continuous-osx" >}}Mac{{< /button >}}

