---
title: "Version 20.9 preview"
date: 2020-04-26T01:46+03:00
anchor: "v20_9_preview"
weight: 40
---

## Version 20.9

As expected I didn't have much time to work on klogg this summer. However, there were several free weekends,
and I was able to do some bugfix and new feature development.

Preview builds can be downloaded from Github: 

{{< button href="https://github.com/variar/klogg/releases/tag/continuous-win" >}}Windows{{< /button >}}
{{< button href="https://github.com/variar/klogg/releases/tag/continuous-linux" >}}Linux{{< /button >}}
{{< button href="https://github.com/variar/klogg/releases/tag/continuous-osx" >}}Mac{{< /button >}}

Feedback is very welcome.

### Changelog

Major new features:

 - Highlighters reworked:
  - Several sets of highlighting rules can be configured and chosen using context menu
  - Highlighters can be configured to colorize only matching part of line
  - Added an option to match using simple strings
  - Highlighters configuration can be exported and imported on other machine
 - An option to choose UI style, including dark theme based on [QDarkStyleSheet](https://github.com/ColinDuquesnoy/QDarkStyleSheet)
 - A menu to switch between opened files
 - Some support for dark OS themes
 - Some support for HiDPI displays

Bugixes and minor improvements

 - Fixed RPM packaging
 - Fixed bug with case-insensitive search autocomplete
 - Fixed excessive reloading when follow mode is enabled 
 - Made some shortcut behavior more user-friendly

As usual 3rdparty libraries were updated, some bugs were fixed, and some more were introduced.

