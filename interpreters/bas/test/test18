#!/bin/sh

echo -n $0: 'DO WHILE, LOOP... '

cat >test.bas <<'eof'
print "loop started"
x$=""
do while len(x$)<3
  print "x$ is ";x$
  x$=x$+"a"
  y$=""
  do while len(y$)<2
    print "y$ is ";y$
    y$=y$+"b"
  loop
loop
print "loop ended"
eof

cat >test.ref <<'eof'
loop started
x$ is 
y$ is 
y$ is b
x$ is a
y$ is 
y$ is b
x$ is aa
y$ is 
y$ is b
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
