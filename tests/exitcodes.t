Setup:

  $ . $TESTDIR/setup.sh
  $ printf 'foo\n' > ./exitcodes_test.txt
  $ printf 'bar\n' >> ./exitcodes_test.txt

Normal matching:

  $ ag foo exitcodes_test.txt
  foo
  $ ag zoo exitcodes_test.txt
  [1]

Inverted matching:

  $ ag -v foo exitcodes_test.txt
  bar
  $ ag -v zoo exitcodes_test.txt
  foo
  bar
  $ ag -v "foo|bar" exitcodes_test.txt
  [1]
