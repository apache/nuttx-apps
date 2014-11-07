#!/bin/sh

echo -n $0: 'Caller trace... '

cat >test.bas <<'eof'
 10 gosub 20
 20 gosub 30
 30 procb
 40 def proca
 50   print "hi"
 60   stop
 70 end proc
 80 def procb
 90   proca
100 end proc
eof

cat >test.ref <<'eof'
hi
Break in line 60 at:
60 stop
   ^
Proc Called in line 90 at:
90 proca
        ^
Proc Called in line 30 at:
30 procb
        ^
Called in line 20 at:
20 gosub 30
           ^
Called in line 10 at:
10 gosub 20
           ^
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
