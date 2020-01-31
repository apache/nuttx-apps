10 open "i",1,"/mnt/romfs/test37.dat"
20 while not eof(1)
30 line input #1,a$
40 if a$="abc" then print a$; else print "def"
50 wend
