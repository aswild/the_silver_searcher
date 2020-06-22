Setup:

  $ . $TESTDIR/setup.sh
  $ printf "hello world\n" >test.txt

Verify ag runs with an empty environment:

  $ env -i $AGPROG $AGOPTS hello
  test.txt:1:hello world
