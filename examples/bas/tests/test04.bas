#!/bin/sh

echo -n $0: 'REPEAT UNTIL loop... '

cat >test.bas <<eof
10 a=1
20 repeat 
30   print a
40   a=a+1
50 until a=10
eof

cat >test.ref <<eof
 1 
 2 
 3 
 4 
 5 
 6 
 7 
 8 
 9 
eof

sh ./test/runbas test.bas >test.data

if cmp test.ref test.data
then
  rm -f test.*
  echo passed
else
  echo failed
  exit 1
fi
