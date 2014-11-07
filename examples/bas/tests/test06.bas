#!/bin/sh

echo -n $0: 'Recursive function without arguments... '

cat >test.bas <<eof
10 def fnloop
20   if n=0.0 then
30     r=0.0
40   else
50     print n
60     n=n-1.0
70     r=fnloop()
80   end if
90 =r
100 n=10
110 print fnloop
eof

cat >test.ref <<eof
 10 
 9 
 8 
 7 
 6 
 5 
 4 
 3 
 2 
 1 
 0 
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
