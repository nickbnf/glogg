---
title: "Version 20.4 released"
date: 2020-04-26T01:46+03:00
anchor: "v20_4"
weight: 45
---

## Version 20.4

During this months there were almost no new commits to klogg. For several years klogg has been my second most frequently used tool (first one being code editor) and I've been doing my best to improve it based on my daily experience and user feedback on github. However, since the beginning of 2020 I've switched to a more manager-like position. This is all very new to me, and doing this job requires tools not related to log file analysis. So I decided to make a release of klogg now to bring important fixes and new features to users of stable version. 

I will continue to work on klogg, but less frequent commits are expected. 

### Changelog

Firstly, klogg on MacOS is once again usable. Thanks for all users who reported problems:

 - MacOS dmg has more user friendly UI.
 - Fixed bug with saving user configuration.
 - Fixed bug with opening log files with klogg from Finder.
 - Fixed some UI rendering issues.

Major new features:

 - Scratchpad window. This window is used to do some text transformations that are sometimes needed during log analysis, like base64-decoding, xml/json formatting etc.
 - Open archives. Klogg can now open zip/gzip/bzip2/xz/7z archives. Files are extracted to temp directory.
 - Open files from network url. Klogg will download file to temp directory.
 - Multi-window mode. Allows to open several klogg windows. Still experimental feature. 
 - Menu options to create bug reports and feature requests on github in one click from application.

Several usability improvements for all versions:

 - New encoding menu layout.
 - New favorites menu to save files that are opened not frequently enough to stay in recent files history
 - More options for tabs (opening file location, copying file path to clipboard, tab renaming).
 - Autocomplete for search regexp.
 - Option to mark several lines at once.
 - Fixes for shortcuts.
 - Some icon changes.
 - More command line options.

As usual 3rdparty libraries were updated, some bugs were fixed, and some more were introduced.

Download on Github: [klogg 20.4](https://github.com/variar/klogg/releases/tag/v20.4)