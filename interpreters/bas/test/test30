#!/bin/sh

echo -n $0: 'Type mismatch check... '

cat >test.bas <<'eof'
print 1+"a"
eof

cat >test.ref <<'eof'
Error: Invalid binary operand at: end of program
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
