---
title: "Version 17.12 released"
date: 2018-01-27T15:42:17+01:00
anchor: "v17_12"
weight: 80
bookhidden: true
---

## Version 17.12

This release is base on glogg 1.1.4. The aims of this release are performance improvement and code base modernization. Several feature requests from upstream have been implemented.

Versioning changed to calendar versioning scheme: Year.Month.Patch.

Changes:

 - Updated to Qt 5.9 (Qt 5.10 on Mac OS)
 - Automagic encoding detection based on uchardet library
 - Using Entropia File System Watcher on all platforms to monitor file changes
 - New index layout in memory to speed up file loading and searching
 - Saving matching lines from filtered view to a file
 - Saving follow mode and marks for opened files
 - Separate portable build
 - Some quick find hotkeys changed
  
Download on Github: [klogg 17.12](https://github.com/variar/klogg/releases/tag/17.12.0)