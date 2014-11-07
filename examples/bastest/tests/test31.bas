10 for i=-8 to 8
20   x=1+1/3 : y=1 : j=i
30   for j=i to -1 : x=x/10 : y=y/10 : next
40   for j=i to 1 step -1 : x=x*10 : y=y*10 : next
50   print x,y
60 next

