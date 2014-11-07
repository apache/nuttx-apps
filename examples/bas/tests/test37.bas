#!/bin/sh

echo -n $0: 'LINE INPUT reaching EOF... '

cat >test.bas <<'eof'
10 open "i",1,"test.ref"
20 while not eof(1)
30 line input #1,a$
40 if a$="abc" then print a$; else print "def"
50 wend
eof

awk 'BEGIN{ printf("abc") }' </dev/null >test.ref

sh ./test/runbas test.bas >test.data

if cmp test.ref test.data
then
  rm -f test.*
  echo passed
else
  echo failed
  exit 1
fi
