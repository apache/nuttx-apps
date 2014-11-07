#!/bin/sh

echo -n $0: 'FIELD, PUT and GET... '

cat >test.bas <<'eof'
a$="a"
open "r",1,"test.dat",128
print "before field a$=";a$
field #1,10 as a$
field #1,5 as b$,5 as c$
lset b$="hi"
rset c$="ya"
print "a$=";a$
put #1
close #1
print "after close a$=";a$
open "r",2,"test.dat",128
field #2,10 as b$
get #2
print "after get b$=";b$
close #2
kill "test.dat"
eof

cat >test.ref <<eof
before field a$=a
a$=hi      ya
after close a$=
after get b$=hi      ya
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
