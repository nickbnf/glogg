---
title: "About"
date: 2018-01-27T15:42:17+01:00
anchor: "about"
weight: 10
---

_klogg_ is a multi-platform GUI application to search through all kinds of text log files using regular expressions. It is fork of [glogg project](https://glogg.bonnefon.org/) created by [Nicolas Bonnefon](https://github.com/nickbnf).

Latest stable version:

[ ![Bintray](https://img.shields.io/bintray/v/variar/generic/klogg)](https://bintray.com/variar/generic/klogg/_latestVersion)
[ ![Chocolatey](https://img.shields.io/chocolatey/v/klogg)](https://chocolatey.org/packages/klogg)

Latest development builds can be downloaded from releases on Github: 
[windows](https://github.com/variar/klogg/releases/tag/continuous-win) | 
[linux](https://github.com/variar/klogg/releases/tag/continuous-linux)|
[mac](https://github.com/variar/klogg/releases/tag/continuous-osx).

_klogg_ is designed to:

 - be fast
 - handle huge log files
 - provide a clear view of the matches even in heavily cluttered files.

General usage can be described as simple loop:

 - Search for some regular expression
 - Mark interesting lines to keep in results window
 - Repeat search with other expression
  
 Using this pattern you can get full picture of what was happening at the time of logging.