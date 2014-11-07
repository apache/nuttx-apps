#!/bin/sh

echo -n $0: 'Print items... '

cat >test.bas <<'eof'
PRINT "Line 1";TAB(78);1.23456789
eof

cat >test.ref <<'eof'
Line 1                                                                       
 1.234568 
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
