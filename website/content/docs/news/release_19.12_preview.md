---
title: "Version 19.12 preview"
date: 2019-12-06T01:46+03:00
anchor: "v19_12_preview1"
weight: 55
---

## Version 19.12 preview

There are several important bugfixes and new features for the next version.

### New features

A new scratchpad tool has beed added. It can be used to store text
and perform some transformations. For now it supports:
 - base64 decoding
 - json indenting
 - xml formatting

These transformations I use the most. Any suggestions are welcome.

Another big feature is new multi-window mode. It is experimental and 
you have to enable it in settings dialog. When enabled a new menu
item in File menu is added that allows to open more windows of klogg.
Can be used to monitor several changing files on one screen.

Tabs now have a menu item to open containing folder.

### Major bugfixes

Version 19.9 introduced splitting configuration into two files. This change resulted in
issues with configuration saving on MacOS. Moreover openening files 
either by double click or from context menu proved to be broken for several releases.  
Search bar redesign also resulted in ugly UI.

After obtaining new MacOS VM I've been able to fix all these issues as well as 
improve dmg generation to allow klogg to easily be copied to Applications.

### Internal changes
After attending ACCU Autumn C++ conference in Belfast I felt very inspired 
to do major refactoring of multithreaded indexing and search code.
Swithed from handwriten producer-consumer using lock-free queues to Intel
Threading Building Blocks Flow Graph API. Intel TBB was already used for memory
allocation and thread local storage, so using it for background tasks seemed natural.

It looks like perfomance remained on the same level but code has become simplier,
and I personally think this is a considerable improvement.

Next I'll look into internal structures for indexing. It seems there is room
for cache utilization improvements.


Preview builds can be downloaded from Github: 

{{< button href="https://github.com/variar/klogg/releases/tag/continuous-windows-x64" >}}Windows{{< /button >}}
{{< button href="https://github.com/variar/klogg/releases/tag/continuous-linux-x64" >}}Linux{{< /button >}}
{{< button href="https://github.com/variar/klogg/releases/tag/continuous-macos-x64" >}}Mac{{< /button >}}
