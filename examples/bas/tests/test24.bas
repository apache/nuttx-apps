#!/bin/sh

echo -n $0: 'Matrix multiplication... '

cat >test.bas <<'eof'
10 dim b(2,3),c(3,2)
20 for i=1 to 2 : for j=1 to 3 : read b(i,j) : next : next
30 for i=1 to 3 : for j=1 to 2 : read c(i,j) : next : next
40 mat a=b*c
50 mat print b,c,a
60 data 1,2,3,3,2,1
70 data 1,2,2,1,3,3
eof

cat >test.ref <<'eof'
 1             2             3            
 3             2             1            

 1             2            
 2             1            
 3             3            

 14            13           
 10            11           
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
