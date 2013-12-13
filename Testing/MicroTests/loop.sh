FUNCTION_CALLS=1000000
DATA_POINTS=100
NUM_THREADS=2

for k in {1..100}
    do
        m=`echo "$FUNCTION_CALLS*$k/$DATA_POINTS" | bc`
        n=`echo "$FUNCTION_CALLS/$m" | bc`
        echo "Calling yield with n=$n and m=$m:"
        time ./loop_test_exe $n $m $NUM_THREADS
    done
