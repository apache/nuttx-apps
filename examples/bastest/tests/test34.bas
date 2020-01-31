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
