#!/bin/sh

echo -n $0: 'ELSEIF... '

cat >test.bas <<'eof'
for x=1 to 3
  if x=1 then
    print "1a"
  else
    if x=2 then
      print "2a"
    else
      print "3a"
    end if
  end if
next

for x=1 to 3
  if x=1 then
    print "1b"
  elseif x=2 then
    print "2b"
  elseif x=3 then print "3b"
next
eof

cat >test.ref <<'eof'
1a
2a
3a
1b
2b
3b
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
