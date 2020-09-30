---
title: "Automatic crash reporting"
date: 2020-09-30T01:46+03:00
anchor: "v20_9_crash reporting"
weight: 35
---

## Automatic crash reporting

In response to several github issues about unexpected crashes I've implemented crash dump collection.
Thanks to [SDK](https://github.com/getsentry/sentry-native) provided by [Sentry](https://sentry.io) it
is fairly easy to integrate Breakpad/Crashpad to collect minidumps for application crashes and send them
to developers for investigation.

### What is included in crash report

Crash report provides information about:

 - operating system: name, version, architecture
 - Qt version
 - modules that were loaded into klogg process: filename, size and hashes for symbols 
 - stacktraces for all running threads in klogg process

These minidumps do not include content of klogg process memory during the crash. 

### A word abour privacy

Although crash dumps can make fixing bugs easier, privacy of klogg users is far more important.
So crash reporting is not automated. If during startup klogg finds minidumps from previous runs, 
it will show a dialog asking user to look through generated crash report and confirm sending it
to Sentry servers. Unsent reports are deleted. Crash reports are anonymous and do not include
any information to identify users or their computers (like hardware ids, hostames, usernames etc.). 

Please check new [privacy policy](/docs/privacy_policy) for more details.



