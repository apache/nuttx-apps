#!/bin/sh

echo -n $0: 'Multi assignment... '

cat >test.bas <<'eof'
a,b = 10
print a,b
dim c(10)
a,c(a) = 2
print a,c(2),c(10)
a$,b$="test"
print a$,b$
eof

cat >test.ref <<'eof'
 10            10 
 2             0             2 
test          test
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
