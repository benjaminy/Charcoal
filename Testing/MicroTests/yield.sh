#!/bin/bash
make clean
make simple_test
make simple_test_noyield
n=1000000
numruns=20
yieldnum=0
noyieldnum=0

for j in {1..100}
do
    yieldnum=`./simple_test $n`
    noyieldnum=`./simple_test_noyield $n`
    echo `python -c "print float($yieldnum - $noyieldnum)/(2*$n)"`
done
