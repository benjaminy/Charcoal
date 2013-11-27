#!/usr/bin/python

import pylab

def main():

  f = open('simple-yield-small.txt', 'rb')
  nums = []
  for line in f:
    print line
    nums.append(float(line))
  pylab.plot(range(1,6), nums, 'mo')
  pylab.axis([0, 101, 0, 10]) # xmin, xmax, ymin, ymax
  pylab.xlabel('Iteration')
  pylab.ylabel('Yield time')
  pylab.show()


  return 0



main()
