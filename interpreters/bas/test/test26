#!/bin/sh

echo -n $0: 'MAT READ... '

cat >test.bas <<'eof'
dim a(3,3)
data 5,5,5,8,8,8,3,3
mat read a(2,3)
mat print a
eof

cat >test.ref <<'eof'
 5             5             5            
 8             8             8            
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
