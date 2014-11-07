#!/bin/sh

echo -n $0: 'SWAP... '

cat >test.bas <<'eof'
a=1 : b=2
print "a=";a;"b=";b
swap a,b
print "a=";a;"b=";b
dim a$(1,1),b$(1,1)
a$(1,0)="a" : b$(0,1)="b"
print "a$(1,0)=";a$(1,0);"b$(0,1)=";b$(0,1)
swap a$(1,0),b$(0,1)
print "a$(1,0)=";a$(1,0);"b$(0,1)=";b$(0,1)
eof

cat >test.ref <<'eof'
a= 1 b= 2 
a= 2 b= 1 
a$(1,0)=ab$(0,1)=b
a$(1,0)=bb$(0,1)=a
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
