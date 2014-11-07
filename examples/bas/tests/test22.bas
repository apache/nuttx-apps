#!/bin/sh

echo -n $0: 'MAT PRINT... '

cat >test.bas <<'eof'
dim a(2,2)
for i=0 to 2
  for j=0 to 2
    a(i,j)=i*10+j
  next
next
for j=1 to 2
  for i=1 to 2
    print using " ##.##";a(i,j),
  next
  print
next
mat print using " ##.##";a,a
eof

cat >test.ref <<'eof'
 11.00 21.00
 12.00 22.00
 11.00 12.00
 21.00 22.00

 11.00 12.00
 21.00 22.00
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
