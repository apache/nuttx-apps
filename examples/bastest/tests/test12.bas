10 on error print "global handler 1 caught error in line ";erl : resume 30
20 print mid$("",-1)
30 on error print "global handler 2 caught error in line ";erl : end
40 def procx
50   on error print "local handler caught error in line";erl : goto 70
60   print 1/0
70 end proc
80 procx
90 print 1 mod 0

