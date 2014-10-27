#!/bin/sh

echo -n $0: 'Matrix multiplication size checks... '

cat >test.bas <<eof
DIM a(3,3),b(3,1),c(3,3)
MAT READ a
MAT READ b
MAT c=a*b
MAT PRINT c
DATA 1,2,3,4,5,6,7,8,9
DATA 5,3,2

erase b
DIM b(3)
RESTORE
MAT READ a
MAT READ b
MAT c=a*b
MAT PRINT c
eof

cat >test.ref <<eof
 17           
 47           
 77           
Error: Dimension mismatch in line 14 at:
mat c=a*b
       ^
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
