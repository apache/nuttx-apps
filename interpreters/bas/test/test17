#!/bin/sh

echo -n $0: 'DO, EXIT DO, LOOP... '

cat >test.bas <<'eof'
print "loop started"
i=1
do
  print "i is";i
  i=i+1
  if i>10 then exit do
loop
print "loop ended"
eof

cat >test.ref <<'eof'
loop started
i is 1 
i is 2 
i is 3 
i is 4 
i is 5 
i is 6 
i is 7 
i is 8 
i is 9 
i is 10 
loop ended
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
