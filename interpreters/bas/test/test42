#!/bin/sh

echo -n $0: 'Arithmetic... '

cat >test.bas <<eof
10 print 4.7\3
20 print -2.3\1
30 print int(-2.3)
40 print int(2.3)
50 print fix(-2.3)
60 print fix(2.3)
70 print fp(-2.3)
80 print fp(2.3)
eof

cat >test.ref <<eof
 1 
-2 
-3 
 2 
-2 
 2 
-0.3 
 0.3 
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
