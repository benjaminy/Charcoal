FUNCTION_CALLS=1000000
DATA_POINTS=5000
NUM_THREADS=2

for k in {1..5000}
    do

        m=`echo "$FUNCTION_CALLS*$k/$DATA_POINTS" | bc`
        n=`echo "$FUNCTION_CALLS/$m" | bc`
        #echo "Calling yield with n=$n and m=$m at k=$k:" >> loop_test5000.txt
        /usr/bin/time -a -o loop_test5000.txt ./loop_test_exe $n $m $NUM_THREADS
    done
