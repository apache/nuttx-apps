on error goto 10
print "opening file"
open "/tmp/test.out" for output lock write as #1
print "open succeeded"
if command$<>"enough" then shell "sh ./test/runbas test.bas enough"
end
10 print "open failed"

