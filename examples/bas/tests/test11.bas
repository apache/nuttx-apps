#!/bin/sh

echo -n $0: 'OPEN and LINE INPUT... '

cat >test.bas <<'eof'
10 open "i",1,"test.bas"
20 while not eof(1)
30 line input #1,a$
40 print a$
50 wend
eof

cat >test.ref <<eof
10 open "i",1,"test.bas"
20 while not eof(1)
30 line input #1,a$
40 print a$
50 wend
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
