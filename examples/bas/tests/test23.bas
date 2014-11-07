#!/bin/sh

echo -n $0: 'Matrix addition and subtraction... '

cat >test.bas <<'eof'
dim a(2,2)
a(2,2)=2.5
dim b%(2,2)
b%(2,2)=3
mat print a
mat a=a-b%
mat print a
dim c$(2,2)
c$(2,1)="hi"
mat print c$
mat c$=c$+c$
mat print c$
eof

cat >test.ref <<'eof'
 0             0            
 0             2.5          
 0             0            
 0            -0.5          
                            
hi                          
                            
hihi                        
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
