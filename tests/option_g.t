Setup:

  $ . $TESTDIR/setup.sh
  $ touch foobar
  $ mkdir txt
  $ touch txt/foo.txt txt/bar

Search for files matching name foobar:

  $ ag -g foobar
  foobar

Search for non-existent files matching name baz:
  $ ag -g baz
  [1]

Search for files with txt in the filename only:
  $ ag -jg txt
  txt/foo.txt
