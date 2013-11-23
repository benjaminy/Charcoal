#!/bin/bash
make clean
make simple_unyield_test
make simple_unyield_test_noyield
n=1000000
numruns=1000000000
yield_total=0
yieldnum=0
noyieldnum=0
for j in {1..$numruns}; do
		out=`./simple_unyield_test $n`
		yieldnum=`expr $yieldnum + $out`
done

echo $yieldnum
for j in {1..$numruns}; do
		out=`./simple_unyield_test_noyield $n`
		noyieldnum=`expr $noyieldnum + $out`
done
echo $noyieldnum
#Deleted +$yield_total because it doesn't seem to do anything?
echo `python -c "print float($yieldnum - $noyieldnum)/(2*$n*$numruns)"`
