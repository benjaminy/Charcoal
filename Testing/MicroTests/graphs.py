#!/usr/bin/python

import pylab

def main():
  strcpy = open('loop_strcpy_results.csv', 'rb')
  m = []
  t=[]
  for line in strcpy.readlines()[1:]:
    entry = line.split(',')
    m.append(float(entry[1]))
    t.append(float(entry[2]))

  pylab.plot(m, t, 'bo')
  pylab.axis([0, 1000000, 0, .9]) # xmin, xmax, ymin, ymax
  pylab.semilogx()
  pylab.xlabel('Number of inner loop iterations')
  pylab.ylabel('% overhead')
  pylab.show()
  
  memcpy = open('loop_memcpy_results.csv', 'rb')
  m = []
  t=[]
  for line in memcpy.readlines()[1:]:
    entry = line.split(',')
    m.append(float(entry[1]))
    t.append(float(entry[2]))

  pylab.plot(m, t, 'bo')
  pylab.axis([0, 1000000, 0, .9]) # xmin, xmax, ymin, ymax
  pylab.semilogx()
  pylab.xlabel('Number of inner loop iterations')
  pylab.ylabel('% overhead')
  pylab.show()
 

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
