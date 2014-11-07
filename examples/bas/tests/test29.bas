#!/bin/sh

echo -n $0: 'TDL INSTR... '

cat >test.bas <<'eof'
print instr("123456789","456");" = 4?"
print INSTR("123456789","654");" = 0?"
print INSTR("1234512345","34");" = 3?"
print INSTR("1234512345","34",6);" = 8?"
print INSTR("1234512345","34",6,2);" = 0?"
print INSTR("1234512345","34",6,4);" = 8?"
eof

cat >test.ref <<'eof'
 4  = 4?
 0  = 0?
 3  = 3?
 8  = 8?
 0  = 0?
 8  = 8?
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
