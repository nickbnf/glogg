---
title: "Version 20.12 released"
date: 2020-12-16T15:42:17+01:00
anchor: "v20_12"
weight: 30
---

## Version 20.12

Finally new stable version is ready. This release has several major new features.

First of all highlighters have been reworked. Now it is possible to create several
sets of highlighting rules and choose active set to apply at the moment. Highlight rules
has become more flexible. It is possible to colorize only matching part of line (with
support for regex capture groups if present). And finally, highlighters configuration can be
exported to a file and shared with collaborators.

Another big feature is support for different window styles. Klogg can be configured to use
one of the default styles provided by Qt or use a separate dark theme base on [QDarkStyleSheet](https://github.com/ColinDuquesnoy/QDarkStyleSheet). If default styles are used then Klogg will adjust its palette
to current OS theme.

This release also introduces new error reporting features to help with bugfix. Using service
and SDK provided by [Sentry](https://sentry.io) Klogg will now collect minidumps when 
something crashes and send them to developers after asking user to review crash report. More
about this is covert in [crash reporting](/docs/news/crash_reporting).

DMG packages for Mac are now properly signed to make Gatekeeper happy.

Bugixes and minor improvements

 - Fixed RPM and DEB packaging
 - Fixed bug with case-insensitive search autocomplete
 - Fixed excessive reloading when follow mode is enabled 
 - Made some shortcut behavior more user-friendly
 - Use DejaVu fonts by default
 - Updated 3rdparty libraries

Download on Github: [klogg 20.12](https://github.com/variar/klogg/releases/tag/v20.12)
