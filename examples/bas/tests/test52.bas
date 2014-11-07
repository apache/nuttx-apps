#!/bin/sh

echo -n $0: 'MAT INPUT... '

cat >test.bas <<'eof'
dim a(2,2)
mat input a
mat print a
mat input a
mat print a
eof

cat >test.input <<'eof'
1,2,3,4,5
1
3,4
eof

cat >test.ref <<'eof'
? 
 1             2            
 3             4            
? ? 
 1             0            
 3             4            
eof

sh ./test/runbas test.bas <test.input >test.data

if cmp test.ref test.data
then
  rm -f test.*
  echo passed
else
  echo failed
  exit 1
fi
