dim a(3,4)
for i=0 to 3
  for j=0 to 4
    a(i,j)=i*10+j
    print a(i,j);
  next
  print
next
mat write a

