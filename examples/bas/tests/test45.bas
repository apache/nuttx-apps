#!/bin/sh

echo -n $0: 'MID$ on left side... '

cat >test.bas <<'eof'
10 mid$(a$,6,4) = "ABCD"
20 print a$
30 a$="0123456789"
40 mid$(a$,6,4) = "ABCD"
50 print a$
60 a$="0123456789"
70 let mid$(a$,6,4) = "ABCD"
80 print a$
eof

cat >test.ref <<'eof'

01234ABCD9
01234ABCD9
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
