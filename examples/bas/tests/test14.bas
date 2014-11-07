#!/bin/sh

echo -n $0: 'SELECT CASE... '

cat >test.bas <<'eof'
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
eof

cat >test.ref <<eof
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
