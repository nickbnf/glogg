---
title: "Version 20.1 preview"
date: 2019-12-06T01:46+03:00
anchor: "v20_1_preview1"
weight: 50
---

## Version 20.1 preview

I've decided to postpone next release to January 2020. Mainly because I like
20.1 version number better than 19.12. This allowed to add sevaral major features.

Preview builds can be downloaded from Github: 

{{< button href="https://github.com/variar/klogg/releases/tag/continuous-win" >}}Windows{{< /button >}}
{{< button href="https://github.com/variar/klogg/releases/tag/continuous-linux" >}}Linux{{< /button >}}
{{< button href="https://github.com/variar/klogg/releases/tag/continuous-osx" >}}Mac{{< /button >}}

Feedback is very welcome.

### New features

Favorites menu to save files that are opened not frequently enough to stay in recent
files history.

Opening archives (zip, 7z, tar) and compressed files (gzip, bzip2, xz, lzma). Files 
are extracted to temporary location and opened from there.

Opening files from network locations. Klogg will download file to temporary location
and open it.

Renaming tabs for opened files.

New encoding menu with support for more common encodings included with Qt.

Documentation updates.

### Bugfixes

Version 19.12 introduced multi-window mode. Thanks to bug reports on GitHub saving
configuration for this case now works as expected.

Several rendering bugs in highlighters dialog have been found that existed since glogg times.
They do not reproduce on Mac VM with software rendering. After some research these are now fixed.

Some single letter hot keys were broken in 19.9. Fixed.
