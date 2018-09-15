dim a(2,2)
for i=0 to 2
  for j=0 to 2
    a(i,j)=i*10+j
  next
next
for j=1 to 2
  for i=1 to 2
    print using " ##.##";a(i,j),
  next
  print
next
mat print using " ##.##";a,a

