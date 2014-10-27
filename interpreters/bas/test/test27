#!/bin/sh

echo -n $0: 'Matrix inversion... '

cat >test.bas <<'eof'
data 1,2,3,4
mat read a(2,2)
mat print a
mat b=inv(a)
mat print b
mat c=a*b
mat print c
eof

cat >test.ref <<'eof'
 1             2            
 3             4            
-2             1            
 1.5          -0.5          
 1             0            
 0             1            
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
