# Klogg documentation

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

In addition to the filtered window, the match overview on the right hand
side of the screen offers a view of the position of matches in the log
file. Matches are showed as small red lines.

### Opening files

*klogg* provides several options for opening files:

* using dedicated item if `File` menu or toolbar
* dragging files from file manager
* downloading file from provided url
* providing one or many files using command line.

On Winodws and Mac OS *klogg* is configured to open `.log` files by
clicking them in file manager.

*klogg* can open files from remote urls. In that case *klogg* will
download file to temporary directory and open it from there.

*klogg* can open archives (`zip`, `7z`, and `tar`). Archive is extracted
to temporary directory and standard open file dialog is presented to
select files. Type of archive is determined automatically (by file
content or extension).

*klogg* can open compressed files (`gzip`, `bzip2`, `xz`, `lzma`). Such file is
decompressed to temporary folder and then opened. Compression type is
determined automatically (by file content or extension).

Pasting text from clipboard to *klogg* also works. In this case *klogg*
will save pasted text to temporary file and open that file for
exploring.

*klogg* saves history of recent opened files. Up to 5 recent files are
available from `File` menu.

### Favorites

Opened files can be added to `Favorites` menu either from
`Favorites->Add to Favorites` or from toolbar.

This menu is used to provide fast access to files that are opened less
often and don't end up in recent files.

### Encodings

*klogg* tries to guess encoding of opened file. If that guess happens to
be wrong then desired encoding can be selected from `Encoding` menu.

### Using highlighters

*Highlighters* can colorize some lines of the log being displayed, for
example to draw attention to lines indicating an error, or to associate
a color with each sort of event. Any number of highlighters can be
defined in the 'Highlighters' configuration dialog, each using a regexp
against which lines will be matched. For each line, all highlighters are
tried in order and the fore and back colors of the first successful
highlighter are applied.

### Marking lines in the log file

In addition to regexp matches, *klogg* enables the user to mark any
interesting line in the log. To do this, click on the round bullet in
the left margin in front of the line that needs to be marked or select
the line and press `'m'` hotkey.

Marks are combined with matches and showed in the filtered window. They
also appears as blue lines in the match overview.

To mark several lines at once select them and use `'m'` hotkey or
context menu.

### Browsing changing log files

*klogg* can display and search through logs while they are written to
disk, as it might be the case when debugging a running program or
server. The log is automatically updated when it grows, but the
'Auto-refresh' option must be enabled if you want the search results to
be automatically refreshed as well.

The `'f'` key might be used to follow the end of the file as it grows (a
la `tail -f`).

### Scratchpad

Sometimes in log files there are text in base64 encoding, unformatted
xml/json, etc. For such cases *klogg* provides Scratchpad tool. Text can
be copied to this window and transformed to human readable form.

New tabs can be opened in Scratchpad using `Ctrl+N` hotkey.

## Settings

### General

#### Font

The font used to display the log file. A clear, monospace font (like the
free, open source, [DejaVu Mono](http://www.dejavu-fonts.org) for
example) is recommended.

#### Search options

Determines which type of regular expression *klogg* will use when
filtering lines for the bottom window, and when using QuickFind.

*   Extended Regexp. The default, uses regular expressions similar to
    those used by Perl
*   Wildcards. Uses wildcards (\* and ?) in a similar fashion as a Unix
    shell
*   Fixed Strings. Searches for the text exactly as it is written, no
    character is special

If incremental quickfind is selected *klogg* will automatically restart
quickfind search when search pattern changes.

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
    more than one main window usin `File->New window`. In this mode last
    closed windows will be saved to open session on next *klogg* start.
    When exiting *klogg* using `File->Exit` all windows are saved and
    will be reopened.

### File

#### File change monitoring

If file change monitoring is enabled *klogg* will use facilities
provided by operating system to reload file when data is changed on
disk.

Sometimes this kind of monitoring does not work reliably, for example on
network shares or directories mounted via sftp. In that case polling can
be enabled to make *klogg* check for changes.

#### Archives

If extract archives is selected then *klogg* will detect if opened file
is of one of supported archives type or a single compressed file and
will ask user permisson to extract archives content to temporary folder.

If you do not want *klogg* to ask for permisson check extract archives
without confirmation option.

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

