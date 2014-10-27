#!/bin/sh

echo -n $0: 'Matrix assignment... '

cat >test.bas <<'eof'
dim a(3,4)
for i=0 to 3
  for j=0 to 4
    a(i,j)=i*10+j
    print a(i,j);
  next
  print
next
mat b=a
for i=0 to 3
  for j=0 to 4
    print b(i,j);
  next
  print
next
eof

cat >test.ref <<'eof'
 0  1  2  3  4 
 10  11  12  13  14 
 20  21  22  23  24 
 30  31  32  33  34 
 0  0  0  0  0 
 0  11  12  13  14 
 0  21  22  23  24 
 0  31  32  33  34 
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
