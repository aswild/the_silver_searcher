Setup:

  $ . $TESTDIR/setup.sh

Complain about nonexistent path:

  $ ag foo doesnt_exist
  ERR: Error resolving path doesnt_exist: No such file or directory
  [1]
