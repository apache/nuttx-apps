#!/bin/sh

echo -n $0: 'OPEN file locking... '

cat >test.bas <<'eof'
on error goto 10
print "opening file"
open "test.out" for output lock write as #1
print "open succeeded"
if command$<>"enough" then shell "sh ./test/runbas test.bas enough"
end
10 print "open failed"
eof

cat >test.ref <<'eof'
opening file
open succeeded
opening file
open failed
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
