#!/bin/sh

echo -n $0: 'Scalar variable assignment... '

cat >test.bas <<eof
10 a=1
20 print a
30 a$="hello"
40 print a$
50 a=0.0002
60 print a
70 a=2.e-6
80 print a
90 a=.2e-6
100 print a
eof

cat >test.ref <<eof
 1 
hello
 0.0002 
 2e-06 
 2e-07 
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
