
## Getting started

_glogg_ can be started from the command line, optionally passing the file to
open as an argument, or via the desktop environment's menu or file
association.
If no file name is passed, _glogg_ will initially open the last used file.

The main window is divided in three parts : the top displays the log file. The
bottom part, called the "filtered view", shows the results of the search. The
line separating the two contains the regular expression used as a filter.

Entering a new regular expression, or a simple search term, will update the
bottom view, displaying the results of the search. The lines matching the
search criteria are listed in order in the results, and are marked with a red
circle in both windows.

## Exploring log files

Regular expressions are a powerful way to extract the information you are
interested in from the log file. _glogg_ uses _extended regular expressions_.

One of the most useful regexp feature when exploring logs is the
_alternation_, using parentheses and the | operator. It searches for several
alternatives, permitting to display several line types in the filtered window,
in the same order they appear in the log file.

For example, to check that every connection opened is also closed, one can use
an expression similar to:

`Entering (Open|Close)Connection`

Any 'open' call without a matching 'close' will immediately be obvious in the
filtered window.
The alternation also works with the whole search line. For example if you also
want to know what kind of connection has been opened:

`Entering (Open|Close)Connection|Created a .* connection`

`.*` will match any sequence of character on a single line, but _glogg_ will only
display lines with a space and the word `connection` somewhere after `Created a`

## Using filters

_Filters_ can colorize some lines of the log being displayed, for example to
draw attention to lines indicating an error, or to associate a color with each
sort of event. Any number of filter can be defined in the 'Filters'
configuration dialog, each using a regexp against which lines will be matched.
For each line, all filters are tried in order and the fore and back colors of
the first successful filter are applied.

## Browsing changing log files

_glogg_ can display and search through logs while they are written to disk, as
it might be the case when debugging a running program or server.
The log is automatically updated when it grows, but the search must be
manually restarted for the new matches to be found.

## Settings
### Font

The font used to display the log file. A clear, monospace font (like the free,
open source, [DejaVu Mono](http://www.dejavu-fonts.org/) for example) is
recommended.

## Keyboard commands

_glogg_ keyboard commands try to emulate roughly the default bindings used by
the classic Unix utilities _vi_ and _less_.

The main commands are:
<table>
<tr><td>arrows</td>
    <td>scroll one line up/down or one column left/right</td></tr>
<tr><td>j/k</td>
    <td>move the selection one line down/up</td></tr>
<tr><td>h/l</td>
    <td>scroll left/right</td></tr>
<tr><td>g/G</td>
    <td>jump to the first/last line of the file (moving the selection)</td></tr>
</table>