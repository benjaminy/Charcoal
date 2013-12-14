FUNCTION_CALLS=1000000
NUM_THREADS=3

#k is inner loop calls
for k in {0..6}
do
    for i in {1..10}
    do
        m=`echo "10^$k" | bc`
        n=`echo "$FUNCTION_CALLS/$m" | bc`
        echo "n=$n, m=$m" >>loop_test_strcpy.txt
        /usr/bin/time -a -o loop_test_strcpy.txt ./loop_test_exe $n $m $NUM_THREADS


        echo "n=$n, m=$m" >>loop_test_memcpy.txt
        /usr/bin/time -a -o loop_test_memcpy.txt ./loop_test_exe $n $m $NUM_THREADS memcpy
    done
done
python process_data.py loop_test_strcpy.txt $FUNCTION_CALLS loop_strcpy_results.csv

python process_data.py loop_test_memcpy.txt $FUNCTION_CALLS loop_memcpy_results.csv
