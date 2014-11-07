#!/bin/sh

echo -n $0: 'DATA, READ and RESTORE... '

cat >test.bas <<eof
10 data "a",b
20 data "c","d
40 read j$
50 print "j=";j$
60 restore 20
70 for i=1 to 3
80 read j$,k$
90 print "j=";j$;" k=";k$
100 next
eof

cat >test.ref <<'eof'
j=a
j=c k=d
Error: end of `data' in line 80 at:
80 read j$,k$
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
