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

