#!/bin/sh

echo -n $0: 'TDL BASIC FNRETURN/FNEND... '

cat >test.bas <<'eof'
def fnfac(n)
  if n=1 then fnreturn 1
fnend n*fnfac(n-1)

print fnfac(10)
eof

cat >test.ref <<'eof'
 3628800 
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
