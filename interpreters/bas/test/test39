#!/bin/sh

echo -n $0: 'Nested function and procedure calls... '

cat >test.bas <<'eof'
def proc_a(x)
print fn_b(1,x)
end proc

def fn_b(a,b)
= a+fn_c(b)

def fn_c(b)
= b+3

proc_a(2)
eof

cat >test.ref <<'eof'
 6 
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
