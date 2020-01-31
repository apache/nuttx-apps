10 dim a(3,3)
20 for i=1 to 3 : for j=1 to 3 : read a(i,j) : next : next
30 mat print a
40 mat a=(3)*a
45 print
50 mat print a
60 data 1,2,3,4,5,6,7,8,9
80 dim inch_array(5,1),cm_array(5,1)
90 mat read inch_array
100 data 1,12,36,100,39.37
110 mat print inch_array
120 mat cm_array=(2.54)*inch_array
130 mat print cm_array
