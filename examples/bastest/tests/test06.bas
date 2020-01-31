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
