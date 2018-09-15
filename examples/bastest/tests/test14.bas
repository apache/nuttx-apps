 10 for i=0 to 9
 20   for j=0 to 9
 30     print i,j
 40     select case i
 50       case 0
 60         print "i after case 0"
 70       case 1
 80         print "i after case 1"
 90         select case j
100           case 0
110             print "j after case 0"
120         end select
130       case 3 to 5,7
140         print "i after case 3 to 5, 7"
150       case is <9
160         print "is after case is <9"
170       case else
180         print "i after case else"
190     end select
200   next
210 next

