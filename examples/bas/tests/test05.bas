#!/bin/sh

echo -n $0: 'GOSUB RETURN subroutines... '

cat >test5.bas <<eof
10 gosub 100
20 gosub 100
30 end
100 gosub 200
110 gosub 200
120 return
200 print "hello, world":return
eof

cat >test5.ref <<eof
hello, world
hello, world
hello, world
hello, world
eof

sh ./test/runbas test5.bas >test5.data

if cmp test5.ref test5.data
then
  rm -f test5.*
  echo passed
else
  echo failed
  exit 1
fi
