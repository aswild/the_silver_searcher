Setup:

  $ . $TESTDIR/setup.sh
  $ printf 'Foo\n' >> ./sample
  $ printf 'bar\n' >> ./sample

Smart case by default:

  $ ag foo sample
  Foo
  $ ag FOO sample
  [1]
  $ ag 'f.o' sample
  Foo
  $ ag Foo sample
  Foo
  $ ag 'F.o' sample
  Foo

Case sensitive mode:

  $ ag -s foo sample
  [1]
  $ ag -s FOO sample
  [1]
  $ ag -s 'f.o' sample
  [1]
  $ ag -s Foo sample
  Foo
  $ ag -s 'F.o' sample
  Foo

Case insensitive mode:

  $ ag fOO -i sample
  Foo
  $ ag fOO --ignore-case sample
  Foo
  $ ag 'f.o' -i sample
  Foo

Case insensitive file regex

  $ ag -i  -g 'Samp.*'
  sample
