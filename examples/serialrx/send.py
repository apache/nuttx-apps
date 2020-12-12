
import sys
from time import sleep

f = open(sys.argv[1], "w")

s = ""
while len(s) < 11520:
  s += "1"

print("Sending to %s" % sys.argv[1])
while True:
  f.write(s)
  f.flush()
  #for i in range(len(s)):
  #  f.write(s[i])
  #  f.flush()
  #  #sleep(0.050)
  sys.stdout.write(".")
  sys.stdout.flush()
