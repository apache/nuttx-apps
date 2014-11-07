#!/bin/sh

echo -n $0: 'OPEN FOR BINARY... '

cat >test.bas <<'eof'
open "test.out" for binary as 1
put 1,1,"xy"
put 1,3,"z!"
put 1,10,1/3
put 1,20,9999
close 1
open "test.out" for binary as 1
s$="    "
get 1,1,s$
get 1,10,x
get 1,20,n%
close
print s$
print x
print n%
kill "test.out"
eof

cat >test.ref <<'eof'
xyz!
 0.333333 
 9999 
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
