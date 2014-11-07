#!/bin/sh

echo -n $0: 'MAT REDIM... '

cat >test.bas <<'eof'
dim x(10)
mat read x
mat print x
mat redim x(7)
mat print x
mat redim x(12)
mat print x
data 1,2,3,4,5,6,7,8,9,10
eof

cat >test.ref <<'eof'
 1            
 2            
 3            
 4            
 5            
 6            
 7            
 8            
 9            
 10           
 1            
 2            
 3            
 4            
 5            
 6            
 7            
 1            
 2            
 3            
 4            
 5            
 6            
 7            
 0            
 0            
 0            
 0            
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
