ag(1) -- The Silver Searcher. Like ack, but faster.
=============================================

## SYNOPSIS

`ag` [_options_] _pattern_ [_path ..._]

## DESCRIPTION

Recursively search for PATTERN in PATH. Like grep or ack, but faster.

## OPTIONS

  * `--ackmate`:
    Output results in a format parseable by [AckMate](https://github.com/protocool/AckMate).

  * `--[no]affinity`:
    Set thread affinity (if platform supports it). Default is true.

  * `--agrc`=_FILE_:
    Use _FILE_ instead of $AGRC or $HOME/.agrc for default options.
    Specify `--no-agrc` to disable reading from an agrc file. See [AGRC][AGRC] for details.

  * `-a --all-types`:
    Search all files. This doesn't include hidden files, and doesn't respect any ignore files.

  * `-A --after`[=_LINES_]:
    Print lines after match. If not provided, _LINES_ defaults to 2.

  * `-B --before`[=_LINES_]:
    Print lines before match. If not provided, _LINES_ defaults to 2.

  * `--[no]break`:
    Print a newline between matches in different files. Enabled by default.

  * `-c --count`:
    Only print the number of matches in each file.
    Note: This is the number of matches, **not** the number of matching lines.
    Pipe output to `wc -l` if you want the number of matching lines.

  * `--[no]color`:
    Print color codes in results. Enabled by default.

  * `--color-line-number`:
    Color codes for line numbers. Default is 1;33.

  * `--color-match`:
    Color codes for result match numbers. Default is 30;43.

  * `--color-path`:
    Color codes for path names. Default is 1;32.

  * `--column`:
    Print column numbers in results.

  * `-C --context`[=_LINES_]:
    Print lines before and after matches. If not provided, _LINES_ defaults to 2.

  * `-D --debug`:
    Output ridiculous amounts of debugging info. Not useful unless you're actually debugging.

  * `--depth`=_NUM_:
    Search up to _NUM_ directories deep, -1 for unlimited. Default is 25.

  * `--[no]filename`:
    Print file names. Enabled by default, except when searching a single file.

  * `-f --[no]follow`:
    Follow symlinks. Default is false.

  * `-F --fixed-strings`:
    Alias for --literal for compatibility with grep.

  * `--[no]group`:
    The default, `--group`, lumps multiple matches in the same file
    together, and presents them under a single occurrence of the
    filename. `--nogroup` refrains from this, and instead places the
    filename at the start of each match line.

  * `-g --filename-pattern`=_PATTERN_:
    Print filenames matching _PATTERN_.

  * `-G --file-search-regex`=_PATTERN_:
    Only search files whose names match _PATTERN_.

  * `-H --[no]heading`:
    Print filenames above matching contents.

  * `--help-types`:
    Alias for --list-file-types for compatibility with ack.

  * `--hidden`:
    Search hidden files. This option obeys ignored files.

  * `-I --ignore`=_PATTERN_:
    Ignore files/directories whose names match _PATTERN_. Literal
    file and directory names are also allowed.

  * `--ignore-dir`=_PATTERN_:
    Alias for --ignore for compatibility with ack.

  * `-i --ignore-case`:
    Match case-insensitively.

  * `-j --just-filename`:
    Search only the file name, not the full path, when using a file search
    regex (such as with -g or -G).

  * `-l --files-with-matches`:
    Only print the names of files containing matches, not the matching
    lines. An empty query will print all files that would be searched.

  * `-L --files-without-matches`:
    Only print the names of files that don't contain matches.

  * `--list-file-types`:
    See `FILE TYPES` below.

  * `-m --max-count`=_NUM_:
    Skip the rest of a file after _NUM_ matches. Default is 0, which never skips.

  * `--[no]mmap`:
    Toggle use of memory-mapped I/O. Defaults to true on platforms where
    `mmap()` is faster than `read()`. (All but macOS.)

  * `--[no]multiline`:
    Match regexes across newlines. Enabled by default.

  * `-n --norecurse`:
    Don't recurse into directories.

  * `--[no]numbers`:
    Print line numbers. Default is to omit line numbers when searching streams or
    a single file.

  * `-o --only-matching`:
    Print only the matching part of the lines.

  * `--one-device`:
    When recursing directories, don't scan dirs that reside on other storage
    devices. This lets you avoid scanning slow network mounts.
    This feature is not supported on all platforms.

  * `-p --path-to-ignore`=_PATH_:
    Provide _PATH_ pointing to a specific .ignore file.

  * `-P --pager`=_COMMAND_:
    Use a pager such as `less`. Use `--nopager` to override. This option
    is also ignored if output is piped to another program.
    The pager selected is selected from (in order): the command line argument,
    the PAGER environment variable, the command "less"

  * `--parallel`:
    Parse the input stream as a search term, not data to search. This is meant
    to be used with tools such as GNU parallel. For example:
    `echo "foo\nbar\nbaz" | parallel "ag {} ."` will run 3 instances of ag,
    searching the current directory for "foo", "bar", and "baz".

  * `--print-long-lines`:
    Print matches on very long lines (> 2k characters by default).

  * `--passthrough --passthru`:
    When searching a stream, print all lines even if they don't match.

  * `-Q --literal`:
    Do not parse _PATTERN_ as a regular expression. Try to match it literally.

  * `-q --silent`:
    Suppress all log messages, including errors.

  * `-r --recurse`:
    Recurse into directories when searching. Default is true.

  * `-s --case-sensitive`:
    Match case-sensitively.

  * `-S --smart-case`:
    Match case-sensitively if there are any uppercase letters in _PATTERN_,
    case-insensitively otherwise. Enabled by default.

  * `--search-binary`:
    Search binary files for matches.

  * `--stats`:
    Print stats (files scanned, time taken, etc).

  * `--stats-only`:
    Print stats (files scanned, time taken, etc) and nothing else.

  * `-t --all-text`:
    Search all text files. This doesn't include hidden files.

  * `-u --unrestricted`:
    Search *all* files. This ignores .ignore, .gitignore, etc. It searches
    binary and hidden files as well.

  * `-U --skip-vcs-ignores`:
    Ignore VCS ignore files (.gitignore, .hgignore), but still
    use .ignore.

  * `-v --invert-match`:
    Match every line *not* containing the specified pattern.

  * `-V --version`:
    Print version info.

  * `--vimgrep`:
    Output results in the same form as Vim's `:vimgrep /pattern/g`

    Here is a ~/.vimrc configuration example:

    `set grepprg=ag\ --vimgrep\ $*`
    `set grepformat=%f:%l:%c:%m`

    Then use `:grep` to grep for something.
    Then use `:copen`, `:cn`, `:cp`, etc. to navigate through the matches.

  * `-w --word-regexp`:
    Only match whole words.

  * `--workers`=_NUM_:
    Use _NUM_ worker threads. Default is the number of CPU cores, with a max of 8.

  * `-W --width`=_NUM_:
    Truncate match lines after _NUM_ characters.

  * `-X --invert-file-search-regex`=_PATTERN_:
    Like -G, but only search files whose names do not match _PATTERN_.
    File-type searches are still used and not inverted.

  * `-z --search-zip`:
    Search contents of compressed files. Currently, gz and xz are supported.
    This option requires that ag is built with lzma and zlib.

  * `-Z --null-lines`:
    Use NULL (`\0`) as the newline separator instead of `\n`. This is primarily
    useful for searching special files such as /proc/pid/environ. It applies
    globally to all files searched, and `\n` is NOT treated as a newline. This
    means that if a normal text file (that contains no null bytes) contains
    a match, the entire file will be printed as though it were a single line.
    This setting does not affect how newlines in regex patterns are handled
    (i.e. it doesn't change PCRE matching).

  * `-0 --null --print0`:
    Separate the filenames with `\0`, rather than `\n`:
    this allows `xargs -0 <command>` to correctly process filenames containing
    spaces or newlines.


## FILE TYPES

It is possible to restrict the types of files searched. For example, passing
`--html` will search only files with the extensions `htm`, `html`, `shtml`
or `xhtml`. For a list of supported types, run `ag --list-file-types`.

## IGNORING FILES

By default, ag will ignore files whose names match patterns in .gitignore,
.hgignore, or .ignore. These files can be anywhere in the directories being
searched. Binary files are ignored by default as well. Finally, ag looks in
$HOME/.agignore for ignore patterns.

If you want to ignore .gitignore and .hgignore, but still take .ignore into
account, use `-U`.

Use the `-t` option to search all text files; `-a` to search all files; and `-u`
to search all, including hidden files.

## AGRC

To modify the "default" options, ag can read a list of command-line arguments
from an "agrc" file. One "agrc" file will be selected as the first of 1) the
_PATH_ in `--agrc`=_PATH_, 2) the environment variable `AGRC`, or 3) the
default `$HOME/.agrc`.

If `--noagrc` is specified, an "agrc" file will not be used.

The "agrc" file should contain a list of command-line arguments, one per line.
You don't need to quote arguments as you would in a shell as the entire line
will be considered a single argument. For long options which take a value, use
the form `--option=value` rather than `--option value`.

## EXAMPLES

`ag printf`:
  Find matches for "printf" in the current directory.

`ag foo /bar/`:
  Find matches for "foo" in path /bar/.

`ag -- --foo`:
  Find matches for "--foo" in the current directory. (As with most UNIX command
  line utilities, "--" is used to signify that the remaining arguments should
  not be treated as options.)

## ABOUT

ag was originally created by Geoff Greer. More information (and the latest
release) can be found at http://geoff.greer.fm/ag

This man page is part of Allen Wild's fork of ag, which uses version 2 of the
PCRE library and other additional features: https://github.com/aswild/the_silver_searcher

## SEE ALSO

grep(1)
