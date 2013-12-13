#!/usr/bin/python

import pylab

def main():

  f = open('simple-yield.txt', 'rb')
  nums = []
  for line in f:
    nums.append(float(line))
  pylab.plot(range(1,101), nums, 'mo')
  pylab.axis([0, 101, 3.5, 6.5]) # xmin, xmax, ymin, ymax
  pylab.xlabel('Iteration')
  pylab.ylabel('Yield time (microsec)')
  pylab.show()

  unyield = open('simple-unyield.txt', 'rb')
  n = []
  for line in unyield:
    n.append(float(line))
  pylab.plot(range(1,101), n, 'bo')
  pylab.axis([0, 101, 0.025, .05]) # xmin, xmax, ymin, ymax
  pylab.xlabel('Iteration')
  pylab.ylabel('Yield time (microsec)')
  pylab.show()
 


  return 0



main()
