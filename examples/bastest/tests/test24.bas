10 dim b(2,3),c(3,2)
20 for i=1 to 2 : for j=1 to 3 : read b(i,j) : next : next
30 for i=1 to 3 : for j=1 to 2 : read c(i,j) : next : next
40 mat a=b*c
50 mat print b,c,a
60 data 1,2,3,3,2,1
70 data 1,2,2,1,3,3
