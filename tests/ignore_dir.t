Setup:

  $ . $TESTDIR/setup.sh
  $ printf 'hello\nworld\n' >hello.txt
  $ printf 'hello\nworld\n' >foo.txt
  $ printf 'goodbye\nworld\n' >goodbye.txt
  $ mkdir foo
  $ cp *.txt foo/

Test ignoring directory:

  $ ag --ignore foo hello | sort
  foo.txt:1:hello
  hello.txt:1:hello

Test ignoring directory with -I option:

  $ ag -I foo hello | sort
  foo.txt:1:hello
  hello.txt:1:hello
