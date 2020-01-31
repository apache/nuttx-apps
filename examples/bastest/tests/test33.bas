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
