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
