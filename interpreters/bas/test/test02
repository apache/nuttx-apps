#!/bin/sh

echo -n $0: 'Array variable assignment... '

cat >test.bas <<eof
10 dim a(1)
20 a(0)=10
30 a(1)=11
40 a=12
50 print a(0)
60 print a(1)
70 print a
eof

cat >test.ref <<eof
 10 
 11 
 12 
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
