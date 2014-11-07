#!/bin/sh

echo -n $0: 'LOCAL variables... '

cat >test.bas <<eof
10 def fna(a)
20   local b
30   b=a+1
40 =b
60 b=3
70 print b
80 print fna(4)
90 print b
eof

cat >test.ref <<eof
 3 
 5 
 3 
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
