#!/bin/sh

echo -n $0: 'END used without program... '

cat >test.bas <<'eof'
for i=1 to 10:print i;:next i:end
eof

cat >test.ref <<'eof'
 1  2  3  4  5  6  7  8  9  10 
eof

sh ./test/runbas <test.bas >test.data

if cmp test.ref test.data
then
  rm -f test.*
  echo passed
else
  echo failed
  exit 1
fi
