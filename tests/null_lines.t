Setup:

  $ . $TESTDIR/setup.sh
  $ printf 'hello world\0this is a test file with\0' >test.txt
  $ printf 'lines separated by\0null bytes instead of newlines\0' >>test.txt
  $ printf 'goodbye world\0' >>test.txt

Test searching null-delimited lines:

  $ ag --null-lines world
  test.txt:1:hello world
  test.txt:5:goodbye world
