import string
from sys import argv

filename = 'loop_test_strcpy.txt'
if(len(argv) > 1):
    filename = argv[1]
print filename
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
    #print "n="+str(n)+", m="+str(m)
    colon = string.find(lines[i+1], ':')
    j = string.find(lines[i+1], 'elapsed')
    time = float(lines[i+1][colon+1:j])
    try:
        entries[(n,m)] += time
    except:
        entries[(n,m)] = time

f.close()
runs = len(lines)/(3*len(entries))

f = open(filename[len(filename)-10:], 'r')

lines = f.readlines()
runtime = 0
for i in range(0, len(lines), 2):
    colon = string.find(lines[i], ':')
    j = string.find(lines[i], 'elapsed')
    runtime += float(lines[i][colon+1:j])
runtime /= len(lines)/2
print runtime
f.close()

outfilename = 'loop_strcpy_results.csv'
if(len(argv)>3):
    outfilename = argv[3]

print outfilename
f = open(outfilename, 'w')
f.write('n,m,time\n')

for n, m in entries.keys():
    print "n="+str(n)+", m="+str(m)
    total_time = entries[(n, m)]/runs
    print total_time
    time = (total_time - runtime)/total_time
    f.write(str(n)+","+str(m)+","+str(time)+"\n")
f.close()
