#!/bin/sh

echo -n $0: 'PRINT USING... '

cat >test.bas <<'eof'
 10 print using "!";"abcdef"
 20 print using "\ \";"abcdef"
 30 print using "###-";-1
 40 print using "###-";0
 50 print using "###-";1
 60 print using "###+";-1
 70 print using "###+";0
 80 print using "###+";1
 90 print using "#####,";1000
100 print using "**#,##.##";1000.00
110 print using "+##.##";1
120 print using "+##.##";1.23400
130 print using "+##.##";123.456
140 print using "+##.";123.456
150 print using "+##";123.456
160 print using "abc def ###.## efg";1.3
170 print using "###.##^^^^^";5
180 print using "###.##^^^^";1000
190 print using ".##^^^^";5.0
200 print using "##^^^^";2.3e-9
210 print using ".##^^^^";2.3e-9
220 print using "#.#^^^^";2.3e-9
230 print using ".####^^^^^";-011466
240 print using "$*,***,***,***.**";3729825.24
250 print using "$**********.**";3729825.24
260 print using "$$###.##";456.78
270 print using "a!b";"S"
280 print using "a!b";"S","T"
290 print using "a!b!c";"S"
300 print using "a!b!c";"S","T"
eof

cat >test.ref <<'eof'
a
abc
  1-
  0 
  1 
  1-
  0+
  1+
 1,000
*1,000.00
 +1.00
 +1.23
+123.46
+123.
+123
abc def   1.30 efg
500.00E-002
100.00E+01
.50E+01
23E-10
.23E-08
2.3E-09
-.1147E+005
$***3,729,825.24
$**3729825.24
$456.78
aSb
aSbaTb
aSb
aSbTc
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
