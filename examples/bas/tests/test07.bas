#!/bin/sh

echo -n $0: 'Recursive function with arguments... '

cat >test.bas <<eof
10 def fna(x)
20   if x=0 then r=1 else r=x*fna(x-1)
30 =r
40 print fna(7)
eof

cat >test.ref <<eof
 5040 
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
