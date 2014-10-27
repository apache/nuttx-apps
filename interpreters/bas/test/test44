#!/bin/sh

echo -n $0: 'DELETE... '

cat >test.bas <<eof
10 print 10
20 print 20
30 print 30
40 print 40
50 print 50
60 print 60
70 print 70
eof

cat >test.input <<eof
load "test.bas"
delete -20
delete 60-
delete 30-40
delete 15
list
eof

cat >test.ref <<eof
Error: No such line at: 15
50 print 50
eof

sh ./test/runbas <test.input >test.data

if cmp test.ref test.data
then
  rm -f test.*
  echo passed
else
  echo failed
  exit 1
fi
