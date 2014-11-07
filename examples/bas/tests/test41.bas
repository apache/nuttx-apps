#!/bin/sh

echo -n $0: 'EXIT FUNCTION... '

cat >test.bas <<'eof'
function f(c)
print "f running"
if (c) then f=42 : exit function
f=43
end function

print f(0)
print f(1)
eof

cat >test.ref <<'eof'
f running
 43 
f running
 42 
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
