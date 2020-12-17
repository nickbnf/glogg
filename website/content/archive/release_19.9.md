---
title: "Version 19.9 released"
date: 2019-09-21T15:42:17+01:00
anchor: "v19_9"
weight: 60
---

## Version 19.9

The main focus of this release was fixing bugs with reading changing files.
There were some usabiltiy improvements, UI became more responsive
and a few minor features sneaked in. And new icons by wiz0u.

Configuration file is now split in two parts:

 - user settings (they can be changed through UI and _klogg_ does not edit them)
 - application settings (history of opened files, search patterns, etc.,
 these are changed automatically by _klogg_)

This release has following changes:

 - fixes for reindexing files that are changing on disk
 - autorefresh of search results
 - quickfind searching on separate thread (avoiding ui freezes)
 - usabiltiy improvements (tab closing options, search bar layout)
 - option to paste data from clipboard and do usual searching (data is saved to temporary file)
 - abitlity to open several files from 'Open file' dialog
 - 3rdparty libraries updates


Download on Github: [klogg 19.9](https://github.com/variar/klogg/releases/tag/v19.9)

