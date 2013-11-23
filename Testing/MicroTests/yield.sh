#!/bin/bash
make clean
make simple_test
make simple_test_noyield
n=1000000
numruns=20
yieldnum=0
noyieldnum=0

for j in {1..$numruns}; do
    $yieldnum=`./simple_test $n`
    #`expr $yieldnum + $out`
#done

#echo $yieldnum
#for j in {1..$numruns}; do
    $noyieldnum=`./simple_test_noyield $n`
    #=`expr $noyieldnum + $out`
#done
#echo $noyieldnum

    echo `python -c "print float($yieldnum - $noyieldnum)/(2*$n)"`
done
