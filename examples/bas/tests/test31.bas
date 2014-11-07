#!/bin/sh

echo -n $0: 'PRINT default format... '

cat >test.bas <<'eof'
10 for i=-8 to 8
20   x=1+1/3 : y=1 : j=i
30   for j=i to -1 : x=x/10 : y=y/10 : next
40   for j=i to 1 step -1 : x=x*10 : y=y*10 : next
50   print x,y
60 next
eof

cat >test.ref <<'eof'
 1.333333e-08                1e-08 
 1.333333e-07                1e-07 
 1.333333e-06                1e-06 
 1.333333e-05                1e-05 
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
 1.333333e+07                1e+07 
 1.333333e+08                1e+08 
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
