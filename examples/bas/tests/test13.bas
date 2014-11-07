#!/bin/sh

echo -n $0: 'Unnumbered lines... '

cat >test.bas <<'eof'
print "a"
goto 20
print "b"
20 print "c"
eof

cat >test.ref <<eof
a
c
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
