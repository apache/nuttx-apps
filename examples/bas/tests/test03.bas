#!/bin/sh

echo -n $0: 'FOR loops... '

cat >test.bas <<eof
 10 for i=0 to 10
 20   print i
 30   if i=5 then exit for
 40 next
 50 for i=0 to 0
 60   print i
 70 next I
 80 for i=1 to 0 step -1
 90   print i
100 next
110 for i=1 to 0
120   print i
130 next
140 for i$="" to "aaaaaaaaaa" step "a"
150   print i$
160 next
eof

cat >test.ref <<eof
 0 
 1 
 2 
 3 
 4 
 5 
 0 
 1 
 0 

a
aa
aaa
aaaa
aaaaa
aaaaaa
aaaaaaa
aaaaaaaa
aaaaaaaaa
aaaaaaaaaa
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
