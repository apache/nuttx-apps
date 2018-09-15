function f(c)
print "f running"
if (c) then f=42 : exit function
f=43
end function

print f(0)
print f(1)

