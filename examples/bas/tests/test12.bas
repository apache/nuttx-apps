#!/bin/sh

echo -n $0: 'Exception handling... '

cat >test.bas <<'eof'
10 on error print "global handler 1 caught error in line ";erl : resume 30
20 print mid$("",-1)
30 on error print "global handler 2 caught error in line ";erl : end
40 def procx
50   on error print "local handler caught error in line";erl : goto 70
60   print 1/0
70 end proc
80 procx
90 print 1 mod 0
eof

cat >test.ref <<eof
global handler 1 caught error in line  20 
local handler caught error in line 60 
global handler 2 caught error in line  90 
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
