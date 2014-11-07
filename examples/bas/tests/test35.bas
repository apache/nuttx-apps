#!/bin/sh

echo -n $0: 'Real to integer conversion... '

cat >test.bas <<'eof'
a%=1.2
print a%
a%=1.7
print a%
a%=-0.2
print a%
a%=-0.7
print a%
eof

cat >test.ref <<'eof'
 1 
 2 
 0 
-1 
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
