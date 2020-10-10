# Klogg documentation

## Table of Contents

1. [Getting started](#Getting-started)
1. [Exploring log files](#Exploring-log-files)
1. [Settings](#Settings)
1. [Keyboard commands](#Keyboard-commands)
1. [Command line options](#Command-line-options)


## Getting started

*klogg* can be started from the command line, optionally passing the
file to open as an argument, or via the desktop environment's menu or
file association. If no file name is passed, *klogg* will initially open
the last used file.

The main window is divided in three parts : the top displays the log
file. The bottom part, called the "filtered view", shows the results of
the search. The line separating the two contains the regular expression
used as a filter.

Entering a new regular expression, or a simple search term, will update
the bottom view, displaying the results of the search. The lines
matching the search criteria are listed in order in the results, and are
marked with a red circle in both windows.

## Exploring log files

Regular expressions are a powerful way to extract the information you
are interested in from the log file. *klogg* uses *extended regular
expressions*.

One of the most useful regexp feature when exploring logs is the
*alternation*, using parentheses and the | operator. It searches for
several alternatives, permitting to display several line types in the
filtered window, in the same order they appear in the log file.

For example, to check that every connection opened is also closed, one
can use an expression similar to:

`Entering (Open|Close)Connection`

Any 'open' call without a matching 'close' will immediately be obvious
in the filtered window. The alternation also works with the whole search
line. For example if you also want to know what kind of connection has
been opened:

`Entering (Open|Close)Connection|Created a .* connection`

`.*` will match any sequence of character on a single line, but *klogg*
will only display lines with a space and the word `connection` somewhere
after `Created a`

*klogg* keeps history of used search patterns and provides autocomplete
for them. This history can be cleared from search text box context menu.
Autocomplete is case sensitive if this option is selected for matching 
regular expression.

In addition to the filtered window, the match overview on the right hand
side of the screen offers a view of the position of matches in the log
file. Matches are showed as small red lines.

In addition to regexp matches, *klogg* enables the user to mark any
interesting line in the log. To do this, click on the round bullet in
the left margin in front of the line that needs to be marked or select
the line and press `'m'` hotkey.
To mark several lines at once select them and use `'m'` hotkey or context menu.

Marks are combined with matches and are always visible
in the filtered window. They also appears as blue lines in the match overview.

### Opening files

*klogg* provides several options for opening files:

* using dedicated open file item in `File` menu or toolbar
* dragging files from file manager
* downloading file from provided url
* providing one or many files via command line
* using recent files or favorites menu items.

On Winodws and Mac OS *klogg* installer configures operating system to open `.log` files by
clicking them in file manager.

#### Archives

*klogg* can open archives (`zip`, `7z`, and `tar`). Archive is extracted
to temporary directory and standard open file dialog is presented to
select files. Type of archive is determined automatically (by file
content or extension).

*klogg* can open compressed files (`gzip`, `bzip2`, `xz`, `lzma`). Such file is
decompressed to temporary folder and then opened. Compression type is
determined automatically (by file content or extension).

#### Urls

*klogg* can open files from remote urls. In that case *klogg* will
download file to temporary directory and open it from there.

#### Recent files

*klogg* saves history of recent opened files. Up to 5 recent files are
available from `File` menu.

#### Favorites

Opened files can be added to `Favorites` menu either from
`Favorites->Add to Favorites` or from toolbar.

This menu is used to provide fast access to files that are opened less
often and don't end up in recent files.

#### Clipboard

Pasting text from clipboard to *klogg* also works. In this case *klogg*
will save pasted text to temporary file and open that file for
exploring.

#### Switching between opened files

Switching from one opened file to another can be done from 
`View->Opened files` menu or using `Ctrl+Shift+O` shortcut 
that shows special dialog to choose between opened files.

### Encodings

*klogg* tries to guess encoding of opened file. If that guess happens to
be wrong then desired encoding can be selected from `Encoding` menu.

### Using highlighters

*Highlighters* can colorize some lines of the log being displayed, for
example to draw attention to lines indicating an error, or to associate
a color with each sort of event. 

Highlighters are grouped into sets. One set of highlighters can be active
at any given time. Current active set can be selected using either 
context menu or `Tools->Highlighters` menu.

Any number of highlighters can be defined in a single set.
Highlighter configuration includes a regular expression to match
and color options. Another option is to use plain text patterns
in cases when complex regular expression are not needed.

Each highlighter can be configured to apply fore and 
back colors either to the whole line that matched its regular
expression or only to matching parts of the line. In the latter case 
if regular expression contains capture groups then only the captured
parts of matching line are highlighted.

Order of highlighters in the set does matter. For each line all
highlighters are tried from bottom to top. Each new matching 
highlighter overrides colors for current line. 

Highlighters configuration can be exported to a file and 
imported for example on another machine. Each set is identified
by unique guid. Only new sets are imported from file.

### Browsing changing log files

*klogg* can display and search through logs while they are written to
disk, as it might be the case when debugging a running program or
server. The log is automatically updated when it grows, but the
'Auto-refresh' option must be enabled if you want the search results to
be automatically refreshed as well.

The `'f'` key might be used to follow the end of the file as it grows (a
la `tail -f`).

*klogg* detects if new lines have been appended to the file or it has
been overwritten. In former case search results will be updated as new
matching lines appear in the file. If file is overwritten then
search results will be cleared. 

*klogg* has two options to distinguish appends from overwrites.
General and more stable options is to recalculate hash of indexed part
of the file and check if it matches current file on disk. This is
reliable but can be slow for large files and for slow filesystem
(e.g. network shares). The other option is to check hashes for only
first and last part of the file. This usually works fast 
but can miss a change in the middle of the file. Appropriate option
can be selected in `Settings->File` tab.

Following file requires monitoring filesystem for changes. If native
monitoring or polling are both disabled in settings, then following file
is also disabled.

### Scratchpad

Sometimes in log files there are text in base64 encoding, unformatted
xml/json, etc. For such cases *klogg* provides Scratchpad tool. Text can
be copied to this window and transformed to human readable form.

New tabs can be opened in Scratchpad using `Ctrl+N` hotkey.

## Settings

### General

#### Search options

Determines which type of regular expression *klogg* will use when
filtering lines for the bottom window, and when using QuickFind.

*   Extended Regexp. The default, uses regular expressions similar to
    those used by Perl
*   Fixed Strings. Searches for the text exactly as it is written, no
    character is special

If incremental quickfind is selected *klogg* will automatically restart
quickfind search when search pattern changes. For performance reasons
incremental quickfind can use only fixed strings patterns.

#### Session options

*   Load last session -- if enabled *klogg* will reopen files that were
    opened when *klogg* was closed. View configuration, marked lines and
    `follow` mode settings are restored for each file.
*   Follow file on load -- if enabled *klogg* will enter `follow` mode
    for for all new opened files.
*   Minimize to tray -- if enabled *klogg* will minimize to tray instead
    of closing main window. Use tray icon context menu of `File->Exit`
    to exit application. This option is not available on Mac OS.
*   Enable multiple windows -- if enabled *klogg* will allow to open
    more than one main window using `File->New window`. In this mode last
    closed windows will be saved to open session on next *klogg* start.
    When exiting *klogg* using `File->Exit` all windows are saved and
    will be reopened.

#### Version checking options

If version checking is enabled then *klogg* will once a week try to grab a version
information file from Github repository and see if new version has been released.

Stable builds will check if new stable version is available and pop a dialog about it.
Testing builds will check for new testing versions.

### View

#### Font

The font used to display the log file. A clear, monospace font (like the
free, open source, [DejaVu Mono](http://www.dejavu-fonts.org) for
example) is recommended.

Font antialiasing can be forced if autodeteced options result in bad
text rendering.

#### Style

Qt usually comes with several options of drawing application widgets.
By default *klogg* uses style that matches current operating system.
Other styles can be chosen from the dropdown.

*klogg* will try to respect current display manager theme and 
use white icons for dark themes. 

Another option is to select Dark style. In this case *klogg*
will use custom dark stylesheet.

#### High DPI

Options in this group can be used in case *klogg* window looks 
ugly on High DPI monitors. Usually Qt detects correct settings.
However, especially for non-integer scale factors manual 
overrides might be useful.

### File

#### File change monitoring

If file change monitoring is enabled *klogg* will use facilities
provided by operating system to reload file when data is changed on
disk.

Sometimes this kind of monitoring does not work reliably, for example on
network shares or directories mounted via sftp. In that case polling can
be enabled to make *klogg* check for changes.

*klogg* tries to detect if file was changed in the already indexed
area. This mechanism involves hash recalculation and can be slow for
large files and network filesystems. If fast modification detection
is enabled *klogg* will check hash for the first and last part of
changed files. This is faster but can miss changes in the middle of
the file, so should be used with caution.

#### Archives

If extract archives is selected then *klogg* will detect if opened file
is of one of supported archives type or a single compressed file and
will ask user permisson to extract archives content to temporary folder.

If you do not want *klogg* to ask for permisson check extract archives
without confirmation option.

#### File download

By default *klogg* will not download files using https if certificates
can't be checked. In some development environments self-signed 
certificates are used. In this case *klogg* can be instructed to ignore
SSL errors.

### Advanced options

These options allow to customize performance related settings.

If parallel search is enabled *klogg* will try to use several cpu cores
for regular expression matching. This does not work with quickfind.

If search results cache is enabled *klogg* will store numbers of lines
that matched search pattern in memory. Repeating search for the same
pattern will not go through all file but use cached line numbers
instead.

When using *klogg* to monitor updating files this option should be
disabled.

In case there is some problem with *klogg* logging can be enabled with
desired level of verbosity. Log files are save to temporary directory.
Log level of 4 or 5 is usually enough.

## Crash reporting

*klogg* uses Crashpad crash handler to collect minidump files in case of 
unexpected crashes. At startup *klogg* checks for new minidumps and asks user
if these files should be sent to developers.

Crash report provides information about:

* operating system: name, version, architecture
* Qt version
* modules that were loaded into *klogg* process: filename, size and hashes for symbols
* stacktraces for all running threads in *klogg* process

These minidumps do not include full content of *klogg* process memory during the crash.

## Keyboard commands

*klogg* keyboard commands try to approximatively emulate the default
bindings used by the classic Unix utilities *vi* and *less*.

The main commands are:

|Keys            |Actions                                                           |
|----------------|------------------------------------------------------------------|
|arrows          |scroll one line up/down or one column left/right                  |
|\[number\] j/k  |move the selection 'number' (or one) line down/up                 |
|h/l             |scroll left/right                                                 |
|\^ or \$        |scroll to beginning or end of selected line                       |
|\[number\] g    |jump to the line number given or the first one if no number is    |
|                |entered                                                           |
|G               |jump to the first line of the file (selecting it)                 |
|Shift+G         |jump to the last line of the file (selecting it)                  |
|Alt+G           |show jump to line dialog                                          |
|' or "          |start a quickfind search in the current screen                    |
|                |(forward and backward)                                            |
|n or N          |repeat the previous quickfind search forward/backward             |
|\* or .         |search for the next occurence of the currently selected text      |
|/ or ,          |search for the previous occurence of the currently selected text  |
|f               |activate 'follow' mode, which keep the display as the tail of the |
|                |file (like "tail -f")                                             |
|m               |put a mark on current selected line                               |
|\[ or \]        |jump to previous or next marked line                              |
|+ or -          |decrease/increase filtered view size                              |
|v               |switch filtered view visibilty mode                               |
|                |(Marks and Matches -&gt; Marks -&gt; Matches)                     |
|F5              |reload current file                                               |
|Ctrl+S          |Set focus to search string edit box                               |
|Ctrl+Shift+O    |Open dialog to switch to another file                             |

## Command line options

|Switch             |Actions                                                   |
|-------------------|----------------------------------------------------------|
|-h, --help         |print help message and exit                               |
|-v, --version      |print version information                                 |
|-m,--multi         |allow multiple instance of klogg to run simultaneously (use together with -s)|                                    |
|-s,--load-session  |load the previous session (default when no file is passed)|
|-n,--new-session   |do not load the previous session (default when a file is passed) |
|-l,--log           |save the log to a file                                    |
|-f,--follow        |follow initial opened files                               |
|-d,--debug         |output more debug (include multiple times for more verbosity e.g. -dddd) |

