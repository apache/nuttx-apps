#!/bin/sh

echo -n $0: 'IMAGE... '

cat >test.bas <<'eof'
   d=3.1
   print using "#.#";d
   print using 10;d
10 image #.##
eof

cat >test.ref <<'eof'
3.1
3.10
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
