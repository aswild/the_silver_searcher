Setup:

  $ . $TESTDIR/setup.sh
  $ echo hello >file1.txt
  $ echo hello >file2.log
  $ echo hello >file3.md

Search for lines matching "hello" in log files

  $ ag -G '\.log' hello
  file2.log:1:hello

Search for lines matching "hello" in non-log files

  $ ag -X '\.log' hello
  file3.md:1:hello
  file1.txt:1:hello
