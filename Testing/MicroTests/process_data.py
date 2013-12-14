import string
from sys import argv

filename = 'loop_test_strcpy.txt'
if(len(argv) > 1):
    filename = argv[1]
f = open(filename, 'r')
lines = f.readlines()

functioncount = 1000000
if(len(argv)>2):
    functioncount = argv[2]

entries = {}

for i in range(0, len(lines), 3):
    j = string.find(lines[i], 'm=')
    n=int(lines[i][2:j-2])
    m=int(lines[i][j+2:len(lines[i])-1])
    print "n="+str(n)+", m="+str(m)
    j = string.find(lines[i+1], 'user')
    time = float(lines[i+1][0:j])
    try:
        entries[(n,m)] += time
    except:
        entries[(n,m)] = time

f.close()
outfilename = 'loop_strcpy_results.csv'
if(len(argv)>3):
    outfilename = argv[3]
f = open(outfilename, 'w')
f.write('n,m,time\n')

for n, m in entries.keys():
    time = entries[(n, m)]/10
    f.write(str(n)+","+str(m)+","+str(time)+"\n")
f.close()
