#!/bin/sh

echo -n $0: 'SUB routines... '

cat >test.bas <<'eof'
PUTS("abc")
END

SUB PUTS(s$)
  FOR i=1 to LEN(s$) : print mid$(s$,i,1); : NEXT
  PRINT
END SUB
eof

cat >test.ref <<'eof'
abc
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
