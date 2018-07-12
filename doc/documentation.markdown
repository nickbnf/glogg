
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

In addition to the filtered window, the match overview on the right hand side
of the screen offers a view of the position of matches in the log file. Matches
are showed as small red lines.

## Using highlighters

_Highlighters_ can colorize some lines of the log being displayed, for example to
draw attention to lines indicating an error, or to associate a color with each
sort of event. Any number of highlighter can be defined in the 'highlighters'
configuration dialog, each using a regexp against which lines will be matched.
For each line, all highlighters are tried in order and the fore and back colors of
the first successful highlighter are applied.

## Marking lines in the log file

In addition to regexp matches, _glogg_ enable the user to mark any interesting
line in the log. To do this, click on the round bullet in the left margin in
front of the line that needs to be marked.

Marks are combined with matches and showed in the filtered window. They also
appears as blue lines in the match overview.

## Browsing changing log files

_glogg_ can display and search through logs while they are written to disk, as
it might be the case when debugging a running program or server.
The log is automatically updated when it grows, but the 'Auto-refresh' option
must be enabled if you want the search results to be automatically refreshed as
well.

The 'f' key might be used to follow the end of the file as it grows (_a la_
`tail -f`).

## Settings
### Font

The font used to display the log file. A clear, monospace font (like the free,
open source, [DejaVu Mono](http://www.dejavu-fonts.org/) for example) is
recommended.

### Search options

Determines which type of regular expression _glogg_ will use when filtering
lines for the bottom window, and when using QuickFind.

* Extended Regexp: the default, uses regular expressions similar to those used by Perl
* Wildcards: uses wildcards (\*, ? and []) in a similar fashion as a Unix shell
* Fixed Strings: searches for the text exactly as it is written, no character is special

## Keyboard commands

_glogg_ keyboard commands try to approximatively emulate the default bindings
used by the classic Unix utilities _vi_ and _less_.

The main commands are:
<table>
<tr><td>arrows</td>
    <td>scroll one line up/down or one column left/right</td></tr>
<tr><td>[number] j/k</td>
    <td>move the selection 'number' (or one) line down/up</td></tr>
<tr><td>h/l</td>
    <td>scroll left/right</td></tr>
<tr><td>[number] g</td>
    <td>jump to the line number given or the first one if no number is entered</td></tr>
<tr><td>G</td>
    <td>jump to the last line of the file (selecting it)</td></tr>
<tr><td>/</td>
    <td>start a quickfind search in the current screen</td></tr>
<tr><td>n/N</td>
    <td>repeat the previous quickfind search forward/backward</td></tr>
<tr><td>*/#</td>
    <td>search for the next/previous occurence of the currently selected text</td></tr>
<tr><td>f</td>
    <td>activate 'follow' mode, which keep the display as the tail of the file (like "tail -f")</td></tr>
</table>
