# Examples / `bastest` Bas BASIC Tests

This directory contains a small program that will mount a ROMFS file system
containing the BASIC test files extracted from the BAS `2.4` release.

## Background

Bas is an interpreter for the classic dialect of the programming language BASIC.
It is pretty compatible to typical BASIC interpreters of the 1980s, unlike some
other UNIX BASIC interpreters, that implement a different syntax, breaking
compatibility to existing programs. Bas offers many ANSI BASIC statements for
structured programming, such as procedures, local variables and various loop
types. Further there are matrix operations, automatic LIST indentation and many
statements and functions found in specific classic dialects. Line numbers are
not required.

The interpreter tokenises the source and resolves references to variables and
jump targets before running the program. This compilation pass increases
efficiency and catches syntax errors, type errors and references to variables
that are never initialised. Bas is written in ANSI C for UNIX systems.

## License

BAS `2.4` is released as part of NuttX under the standard 3-clause BSD license
use by all components of NuttX. This is not incompatible with the original BAS
`2.4` licensing

Copyright (c) 1999-2014 Michael Haardt

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# Test Overview

## `test01.bas`

Scalar variable assignment

### Test File

```basic
10 a=1
20 print a
30 a$="hello"
40 print a$
50 a=0.0002
60 print a
70 a=2.e-6
80 print a
90 a=.2e-6
100 print a
```

### Expected Result

```
 1
hello
 0.0002
 0.000
 0.0000020
 0.0000002
```

### Notes

Output would differ on other platforms NttX does not use scientific notation in
floating point output.

## `test02.bas`

Array variable assignment

### Test File

```basic
10 dim a(1)
20 a(0)=10
30 a(1)=11
40 a=12
50 print a(0)
60 print a(1)
70 print a
```

### Expected Result

```
 10
 11
 12
```

## `test03.bas`

`FOR` loops

### Test File

```basic
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
```

### Expected Result

```
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
```

## `test04.bas`

`REPEAT` `UNTIL` loop

### Test File

```basic
10 a=1
20 repeat
30   print a
40   a=a+1
50 until a=10
```

### Expected Result

```
 1
 2
 3
 4
 5
 6
 7
 8
 9
```

## `test05.bas`

`GOSUB` `RETURN` subroutines

### Test File

```basic
10 gosub 100
20 gosub 100
30 end
100 gosub 200
110 gosub 200
120 return
200 print "hello, world":return
```

### Expected Result

```
hello, world
hello, world
hello, world
hello, world
```

## `test06.bas`

Recursive function without arguments

### Test File

```basic
10 def fnloop
20   if n=0.0 then
30     r=0.0
40   else
50     print n
60     n=n-1.0
70     r=fnloop()
80   end if
90 =r
100 n=10
110 print fnloop
```

### Expected Result

```
 10
 9
 8
 7
 6
 5
 4
 3
 2
 1
 0
```

## `test07.bas`

Recursive function with arguments

### Test File

```basic
10 def fna(x)
20   if x=0 then r=1 else r=x*fna(x-1)
30 =r
40 print fna(7)
```

### Expected Result

```
5040
```

## `test08.bas`

`DATA`, `READ` and `RESTORE`

### Test File

```basic
10 data "a",b
20 data "c","d
40 read j$
50 print "j=";j$
60 restore 20
70 for i=1 to 3
80 read j$,k$
90 print "j=";j$;" k=";k$
100 next
```

### Expected Result

```
j=a
j=c k=d
Error: end of `data' in line 80 at:
80 read j$,k$
        ^
```

## `test09.bas`

`LOCAL` variables

### Test File

```basic
10 def fna(a)
20   local b
30   b=a+1
40 =b
60 b=3
70 print b
80 print fna(4)
90 print b
```

### Expected Result

```
 3
 5
 3
```

## `test10.bas`

`PRINT USING`

### Test File

```basic
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
```

### Expected Result

```
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
```

## `test11.bas`

`OPEN` and `LINE INPUT`

### Test File

```basic
10 open "i",1,"test.bas"
20 while not eof(1)
30 line input #1,a$
40 print a$
50 wend
```

### Expected Result

```basic
10 open "i",1,"test.bas"
20 while not eof(1)
30 line input #1,a$
40 print a$
50 wend
```

## `test12.bas`

Exception handling

### Test File

```basic
10 on error print "global handler 1 caught error in line ";erl : resume 30
20 print mid$("",-1)
30 on error print "global handler 2 caught error in line ";erl : end
40 def procx
50   on error print "local handler caught error in line";erl : goto 70
60   print 1/0
70 end proc
80 procx
90 print 1 mod 0
```

### Expected Result

```
global handler 1 caught error in line  20
local handler caught error in line 60
global handler 2 caught error in line  90
```

## `test13.bas`

Unnumbered lines

### Test File

```basic
print "a"
goto 20
print "b"
20 print "c"
```

### Expected Result

```
a
c
```

## `test14.bas`

`SELECT CASE`

### Test File

```basic
 10 for i=0 to 9
 20   for j=0 to 9
 30     print i,j
 40     select case i
 50       case 0
 60         print "i after case 0"
 70       case 1
 80         print "i after case 1"
 90         select case j
100           case 0
110             print "j after case 0"
120         end select
130       case 3 to 5,7
140         print "i after case 3 to 5, 7"
150       case is <9
160         print "is after case is <9"
170       case else
180         print "i after case else"
190     end select
200   next
210 next
```

### Expected Result

```
 0             0
i after case 0
 0             1
i after case 0
 0             2
i after case 0
 0             3
i after case 0
 0             4
i after case 0
 0             5
i after case 0
 0             6
i after case 0
 0             7
i after case 0
 0             8
i after case 0
 0             9
i after case 0
 1             0
i after case 1
j after case 0
 1             1
i after case 1
 1             2
i after case 1
 1             3
i after case 1
 1             4
i after case 1
 1             5
i after case 1
 1             6
i after case 1
 1             7
i after case 1
 1             8
i after case 1
 1             9
i after case 1
 2             0
is after case is <9
 2             1
is after case is <9
 2             2
is after case is <9
 2             3
is after case is <9
 2             4
is after case is <9
 2             5
is after case is <9
 2             6
is after case is <9
 2             7
is after case is <9
 2             8
is after case is <9
 2             9
is after case is <9
 3             0
i after case 3 to 5, 7
 3             1
i after case 3 to 5, 7
 3             2
i after case 3 to 5, 7
 3             3
i after case 3 to 5, 7
 3             4
i after case 3 to 5, 7
 3             5
i after case 3 to 5, 7
 3             6
i after case 3 to 5, 7
 3             7
i after case 3 to 5, 7
 3             8
i after case 3 to 5, 7
 3             9
i after case 3 to 5, 7
 4             0
i after case 3 to 5, 7
 4             1
i after case 3 to 5, 7
 4             2
i after case 3 to 5, 7
 4             3
i after case 3 to 5, 7
 4             4
i after case 3 to 5, 7
 4             5
i after case 3 to 5, 7
 4             6
i after case 3 to 5, 7
 4             7
i after case 3 to 5, 7
 4             8
i after case 3 to 5, 7
 4             9
i after case 3 to 5, 7
 5             0
i after case 3 to 5, 7
 5             1
i after case 3 to 5, 7
 5             2
i after case 3 to 5, 7
 5             3
i after case 3 to 5, 7
 5             4
i after case 3 to 5, 7
 5             5
i after case 3 to 5, 7
 5             6
i after case 3 to 5, 7
 5             7
i after case 3 to 5, 7
 5             8
i after case 3 to 5, 7
 5             9
i after case 3 to 5, 7
 6             0
is after case is <9
 6             1
is after case is <9
 6             2
is after case is <9
 6             3
is after case is <9
 6             4
is after case is <9
 6             5
is after case is <9
 6             6
is after case is <9
 6             7
is after case is <9
 6             8
is after case is <9
 6             9
is after case is <9
 7             0
i after case 3 to 5, 7
 7             1
i after case 3 to 5, 7
 7             2
i after case 3 to 5, 7
 7             3
i after case 3 to 5, 7
 7             4
i after case 3 to 5, 7
 7             5
i after case 3 to 5, 7
 7             6
i after case 3 to 5, 7
 7             7
i after case 3 to 5, 7
 7             8
i after case 3 to 5, 7
 7             9
i after case 3 to 5, 7
 8             0
is after case is <9
 8             1
is after case is <9
 8             2
is after case is <9
 8             3
is after case is <9
 8             4
is after case is <9
 8             5
is after case is <9
 8             6
is after case is <9
 8             7
is after case is <9
 8             8
is after case is <9
 8             9
is after case is <9
 9             0
i after case else
 9             1
i after case else
 9             2
i after case else
 9             3
i after case else
 9             4
i after case else
 9             5
i after case else
 9             6
i after case else
 9             7
i after case else
 9             8
i after case else
 9             9
i after case else
```

## `test15.bas`

`FIELD`, `PUT` and `GET`

### Test File

```basic
a$="a"
open "r",1,"test.dat",128
print "before field a$=";a$
field #1,10 as a$
field #1,5 as b$,5 as c$
lset b$="hi"
rset c$="ya"
print "a$=";a$
put #1
close #1
print "after close a$=";a$
open "r",2,"test.dat",128
field #2,10 as b$
get #2
print "after get b$=";b$
close #2
kill "test.dat"
```

### Expected Result

```
before field a$=a
a$=hi      ya
after close a$=
after get b$=hi      ya
```

## `test16.bas`

`SWAP`

### Test File

```basic
a=1 : b=2
print "a=";a;"b=";b
swap a,b
print "a=";a;"b=";b
dim a$(1,1),b$(1,1)
a$(1,0)="a" : b$(0,1)="b"
print "a$(1,0)=";a$(1,0);"b$(0,1)=";b$(0,1)
swap a$(1,0),b$(0,1)
print "a$(1,0)=";a$(1,0);"b$(0,1)=";b$(0,1)
```

### Expected Result

```
a= 1 b= 2
a= 2 b= 1
a$(1,0)=ab$(0,1)=b
a$(1,0)=bb$(0,1)=a
```

## `test17.bas`

`DO`, `EXIT DO`, `LOOP`

### Test File

```basic
print "loop started"
i=1
do
  print "i is";i
  i=i+1
  if i>10 then exit do
loop
print "loop ended"
```

### Expected Result

```
loop started
i is 1
i is 2
i is 3
i is 4
i is 5
i is 6
i is 7
i is 8
i is 9
i is 10
loop ended
```

## `test18.bas`

`DO WHILE`, `LOOP`

### Test File

```basic
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
```

### Expected Result

```
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
```

## `test19.bas`

`ELSEIF`

### Test File

```basic
for x=1 to 3
  if x=1 then
    print "1a"
  else
    if x=2 then
      print "2a"
    else
      print "3a"
    end if
  end if
next

for x=1 to 3
  if x=1 then
    print "1b"
  elseif x=2 then
    print "2b"
  elseif x=3 then print "3b"
next
```

### Expected Result

```
1a
2a
3a
1b
2b
3b
```

## `test20.bas`

Caller trace

### Test File

```basic
 10 gosub 20
 20 gosub 30
 30 procb
 40 def proca
 50   print "hi"
 60   stop
 70 end proc
 80 def procb
 90   proca
100 end proc
```

### Expected Result

```
hi
Break in line 60 at:
60 stop
   ^
Proc Called in line 90 at:
90 proca
        ^
Proc Called in line 30 at:
30 procb
        ^
Called in line 20 at:
20 gosub 30
           ^
Called in line 10 at:
10 gosub 20
           ^
```

## `test21.bas`

Matrix assignment

### Test File

```basic
dim a(3,4)
for i=0 to 3
  for j=0 to 4
    a(i,j)=i*10+j
    print a(i,j);
  next
  print
next
mat b=a
for i=0 to 3
  for j=0 to 4
    print b(i,j);
  next
  print
next
```

### Expected Result

```
 0  1  2  3  4
 10  11  12  13  14
 20  21  22  23  24
 30  31  32  33  34
 0  0  0  0  0
 0  11  12  13  14
 0  21  22  23  24
 0  31  32  33  34
```

## `test22.bas`

`MAT PRINT`

### Test File

```basic
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
```

### Expected Result

```
 11.00 21.00
 12.00 22.00
 11.00 12.00
 21.00 22.00

 11.00 12.00
 21.00 22.00
```

## `test23.bas`

Matrix addition and subtraction

### Test File

```basic
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
```

### Expected Result

```
 0             0
 0             2.5
 0             0
 0            -0.5

hi

hihi
```

## `test24.bas`

Matrix multiplication

### Test File

```basic
10 dim b(2,3),c(3,2)
20 for i=1 to 2 : for j=1 to 3 : read b(i,j) : next : next
30 for i=1 to 3 : for j=1 to 2 : read c(i,j) : next : next
40 mat a=b*c
50 mat print b,c,a
60 data 1,2,3,3,2,1
70 data 1,2,2,1,3,3
```

### Expected Result

```
 1             2             3
 3             2             1

 1             2
 2             1
 3             3

 14            13
 10            11
```

## `test25.bas`

Matrix scalar multiplication

### Test File

```basic
10 dim a(3,3)
20 for i=1 to 3 : for j=1 to 3 : read a(i,j) : next : next
30 mat print a
40 mat a=(3)*a
45 print
50 mat print a
60 data 1,2,3,4,5,6,7,8,9
80 dim inch_array(5,1),cm_array(5,1)
90 mat read inch_array
100 data 1,12,36,100,39.37
110 mat print inch_array
120 mat cm_array=(2.54)*inch_array
130 mat print cm_array
```

### Expected Result

```
 1             2             3
 4             5             6
 7             8             9

 3             6             9
 12            15            18
 21            24            27
 1
 12
 36
 100
 39.37
 2.54
 30.48
 91.44
 254
 99.9998
```

## `test26.bas`

MAT READ

### Test File

```basic
dim a(3,3)
data 5,5,5,8,8,8,3,3
mat read a(2,3)
mat print a
```

### Expected Result

```
 5             5             5
 8             8             8
```

## `test27.bas`

Matrix inversion

### Test File

```basic
data 1,2,3,4
mat read a(2,2)
mat print a
mat b=inv(a)
mat print b
mat c=a*b
mat print c
```

### Expected Result

```
 1             2
 3             4
-2             1
 1.5          -0.5
 1             0
 0             1
```

## `test28.bas`

TDL BASIC `FNRETURN`/`FNEND`

### Test File

```basic
def fnfac(n)
  if n=1 then fnreturn 1
fnend n*fnfac(n-1)

print fnfac(10)
```

### Expected Result

```
 3628800
```

## `test29.bas`

TDL INSTR

### Test File

```basic
print instr("123456789","456");" = 4?"
print INSTR("123456789","654");" = 0?"
print INSTR("1234512345","34");" = 3?"
print INSTR("1234512345","34",6);" = 8?"
print INSTR("1234512345","34",6,2);" = 0?"
print INSTR("1234512345","34",6,4);" = 8?"
```

### Expected Result

```
 4  = 4?
 0  = 0?
 3  = 3?
 8  = 8?
 0  = 0?
 8  = 8?
```

## `test30.bas`

Type mismatch check

### Test File

```basic
print 1+"a"
```

### Expected Result

```
Error: Invalid binary operand at: end of program
```

## `test31.bas`

PRINT default format

### Test File

```basic
10 for i=-8 to 8
20   x=1+1/3 : y=1 : j=i
30   for j=i to -1 : x=x/10 : y=y/10 : next
40   for j=i to 1 step -1 : x=x*10 : y=y*10 : next
50   print x,y
60 next
```

### Expected Result

```
 0.0000000     0.0000000
 0.0000001     0.0000001
 0.0000013     0.0000010
 0.0000133     0.0000100
 0.000133      0.0001
 0.001333      0.001
 0.013333      0.01
 0.133333      0.1
 1.333333      1
 13.33333      10
 133.3333      100
 1333.333      1000
 13333.33      10000
 133333.3      100000
 1333333       1000000
 13333333.3333333            10000000.0000000
 133333333.3333333           100000000.0000000
```

### Notes

Output would differ on other platforms NttX does not use scientific notation in
floating point output.

## `test32.bas`

`SUB` routines

### Test File

```basic
PUTS("abc")
END

SUB PUTS(s$)
  FOR i=1 to LEN(s$) : print mid$(s$,i,1); : NEXT
  PRINT
END SUB
```

### Expected Result

```
abc
```

## `test33.bas`

`OPEN FOR BINARY`

### Test File

```basic
open "/tmp/test.out" for binary as 1
put 1,1,"xy"
put 1,3,"z!"
put 1,10,1/3
put 1,20,9999
close 1
open "/tmp/test.out" for binary as 1
s$="    "
get 1,1,s$
get 1,10,x
get 1,20,n%
close
print s$
print x
print n%
kill "/tmp/test.out"
```

### Expected Result

```
xyz!
 0.333333
 9999
```

### Notes

The logic in this test will fail if there is no writable file system mount at
`/tmp`.

## `test34.bas`

`OPTION BASE`

### Test File

```basic
option base 3
dim a(3,5)
a(3,3)=1
a(3,5)=2

print a(3,3)
print a(3,5)

option base -2
dim b(-1,2)
b(-2,-2)=10
b(-1,2)=20

print a(3,3)
print a(3,5)
print b(-2,-2)
print b(-1,2)
```

### Expected Result

```
 1
 2
 1
 2
 10
 20
```

## `test35.bas`

Real to integer conversion

### Test File

```basic
a%=1.2
print a%
a%=1.7
print a%
a%=-0.2
print a%
a%=-0.7
print a%
```

### Expected Result

```
 1
 2
 0
-1
```

## `test36.bas`

`OPEN` file locking

### Test File

```basic
on error goto 10
print "opening file"
open "/tmp/test.out" for output lock write as #1
print "open succeeded"
if command$<>"enough" then shell "sh ./test/runbas test.bas enough"
end
10 print "open failed"
```

### Expected Result

```
opening file
open succeeded
opening file
open failed
```

### Notes

1. The logic in this test will fail opening the test.out file if there is no
   writable file system mount at `/tmp`.
2. This test will still currently fail when try to fork the shell because
   support for that feature is not implemented. The following error message
   should be received:

   ```
   Error: Forking child process failed (Unknown error) in line 5 at:
   if command$<>"enough" then shell "sh ./test/runbas test.bas enough"
                             ^
   ```

## `test37.bas`

`LINE INPUT` reaching `EOF`

### Test File

```basic
10 open "i",1,"/mnt/romfs/test37.dat"
20 while not eof(1)
30 line input #1,a$
40 if a$="abc" then print a$; else print "def"
50 wend
```

## Data file (`/mnt/romfs/test37.dat`)

```
abc
```

## Result

```
abc
```

## `test38.bas`

`MAT REDIM`

### Test File

```basic
dim x(10)
mat read x
mat print x
mat redim x(7)
mat print x
mat redim x(12)
mat print x
data 1,2,3,4,5,6,7,8,9,10
```

### Expected Result

```
 1
 2
 3
 4
 5
 6
 7
 8
 9
 10
 1
 2
 3
 4
 5
 6
 7
 1
 2
 3
 4
 5
 6
 7
 0
 0
 0
 0
 0
```

## `test39.bas`

Nested function and procedure calls

### Test File

```basic
def proc_a(x)
print fn_b(1,x)
end proc

def fn_b(a,b)
= a+fn_c(b)

def fn_c(b)
= b+3

proc_a(2)
```

### Expected Result

```
 6
```

## `test40.bas`

`IMAGE`

### Test File

```basic
   d=3.1
   print using "#.#";d
   print using 10;d
10 image #.##
```

### Expected Result

```
3.1
3.10
```

## `test41.bas`

`EXIT FUNCTION`

### Test File

```basic
function f(c)
print "f running"
if (c) then f=42 : exit function
f=43
end function

print f(0)
print f(1)
```

### Expected Result

```
f running
 43
f running
 42
```

## `test42.bas`

Arithmetic

### Test File

```basic
10 print 4.7\3
20 print -2.3\1
30 print int(-2.3)
40 print int(2.3)
50 print fix(-2.3)
60 print fix(2.3)
70 print fp(-2.3)
80 print fp(2.3)
```

### Expected Result

```
 1
-2
-3
 2
-2
 2
-0.3
 0.3
```

## `test43.bas`

Matrix multiplication size checks

### Test File

```basic
DIM a(3,3),b(3,1),c(3,3)
MAT READ a
MAT READ b
MAT c=a*b
MAT PRINT c
DATA 1,2,3,4,5,6,7,8,9
DATA 5,3,2

erase b
DIM b(3)
RESTORE
MAT READ a
MAT READ b
MAT c=a*b
MAT PRINT c
```

### Expected Result

```
 17
 47
 77
Error: Dimension mismatch in line 14 at:
mat c=a*b
       ^
```

## `test44.bas`

`DELETE`

### Test File

```basic
10 print 10
20 print 20
30 print 30
40 print 40
50 print 50
60 print 60
70 print 70
```

## Usage

```basic
load "test.bas"
delete -20
delete 60-
delete 30-40
delete 15
list
```

### Expected Result

```
Error: No such line at: 15
50 print 50
```

## `test45.bas`

`MID$` on left side

### Test File

```basic
10 mid$(a$,6,4) = "ABCD"
20 print a$
30 a$="0123456789"
40 mid$(a$,6,4) = "ABCD"
50 print a$
60 a$="0123456789"
70 let mid$(a$,6,4) = "ABCD"
80 print a$
```

### Expected Result

```

01234ABCD9
01234ABCD9
```

## `test46.bas`

`END` used without program

### Test File

```basic
for i=1 to 10:print i;:next i:end
```

### Expected Result

```
 1  2  3  4  5  6  7  8  9  10
```

## `test47.bas`

`MAT WRITE`

### Test File

```basic
dim a(3,4)
for i=0 to 3
  for j=0 to 4
    a(i,j)=i*10+j
    print a(i,j);
  next
  print
next
mat write a
```

### Expected Result

```
 0  1  2  3  4
 10  11  12  13  14
 20  21  22  23  24
 30  31  32  33  34
11,12,13,14
21,22,23,24
31,32,33,34
```

## `test48.bas`

Multi assignment

### Test File

```basic
a,b = 10
print a,b
dim c(10)
a,c(a) = 2
print a,c(2),c(10)
a$,b$="test"
print a$,b$
```

### Expected Result

```
 10            10
 2             0             2
test          test
```

## `test49.bas`

Matrix determinant

### Test File

```basic
width 120
dim a(7,7),b(7,7)
mat read a
mat print a;
print
data 58,71,67,36,35,19,60
data 50,71,71,56,45,20,52
data 64,40,84,50,51,43,69
data 31,28,41,54,31,18,33
data 45,23,46,38,50,43,50
data 41,10,28,17,33,41,46
data 66,72,71,38,40,27,69
mat b=inv(a)
mat print b
print det
```

### Expected Result

```
 58  71  67  36  35  19  60
 50  71  71  56  45  20  52
 64  40  84  50  51  43  69
 31  28  41  54  31  18  33
 45  23  46  38  50  43  50
 41  10  28  17  33  41  46
 66  72  71  38  40  27  69

 9.636025e+07                320206       -537449        2323650      -1.135486e+07                3.019632e+07
              -9.650941e+07
 4480          15           -25            108          -528           1404         -4487
-39436        -131           220          -951           4647         -12358         39497
 273240        908          -1524          6589         -32198         85625        -273663
-1846174      -6135          10297        -44519         217549       -578534        1849032
 1.315035e+07                43699        -73346         317110       -1549606       4120912      -1.31707e+07

-9.636079e+07               -320208        537452       -2323663       1.135493e+07               -3.019649e+07
               9.650995e+07
 1
```

### Notes

Output will differ because NuttX does not use scientific notation in output.
Some minor rounding differences may also be expected.

## `test50.bas`

`min` and `max` function

### Test File

```basic
print min(1,2)
print min(2,1)
print min(-0.3,0.3)
print min(-0.3,4)
print max(1,2)
print max(2,1)
print max(-0.3,0.3)
print max(-0.3,4)
```

### Expected Result

```
 1
 1
-0.3
-0.3
 2
 2
 0.3
 4
```

## `test51.bas`

Print items

### Test File

```basic
PRINT "Line 1";TAB(78);1.23456789
```

### Expected Result

```
Line 1                                                                        1.234568
```

## `test52.bas`

`MAT INPUT`

### Test File

```basic
dim a(2,2)
mat input a
mat print a
mat input a
mat print a
```

### Test File

```basic
1,2,3,4,5
1
3,4
```

### Expected Result

```
?
 1             2
 3             4
? ?
 1             0
 3             4
```
