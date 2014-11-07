#!/bin/sh

echo -n $0: 'Min and max function... '

cat >test.bas <<'eof'
print min(1,2)
print min(2,1)
print min(-0.3,0.3)
print min(-0.3,4)
print max(1,2)
print max(2,1)
print max(-0.3,0.3)
print max(-0.3,4)
eof

cat >test.ref <<'eof'
 1 
 1 
-0.3 
-0.3 
 2 
 2 
 0.3 
 4 
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
