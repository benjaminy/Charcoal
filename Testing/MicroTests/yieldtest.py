#Does not work
import os

os.system("make clean")
os.system("make simple_test")
os.system("make simple_test_noyield")
n=1000000
numruns=100
yield_total=0

for i in range(numruns):
	yieldnum=0
	noyieldnum=0
	for j in range(n):
		yieldnum += float(os.system("./simple_test "+str(n)))
	
	for j in range(n):
		noyieldnum+=float(os.system("./simple_test_noyield "+str(n)))
	yield_total+=(yieldnum - noyieldnum)/(2*n)

print yield_total/numruns
