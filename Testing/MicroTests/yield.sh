#!/bin/bash
make clean
make simple_test
make simple_test_noyield
n=1000000
numruns=100
yield_total=0
yieldnum=0
noyieldnum=0
for j in {1..$numruns}; do
		out=`./simple_test $n`
		yieldnum=`expr $yieldnum + $out`
done

echo $yieldnum
for j in {1..$numruns}; do
		out=`./simple_test_noyield $n`
		noyieldnum=`expr $noyieldnum + $out`
done
echo $noyieldnum

echo `python -c "print float($yieldnum - $noyieldnum)/(2*$n) + $yield_total"`
