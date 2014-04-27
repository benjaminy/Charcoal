#include <charcoal_runtime.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>


void fa( void *a )
{
	int n = ((int*) a)[0];
	int i = 0;
	double j = 0;
	for(i = 0; i < n; i++){
		j+=i;
		OPA_store_int(&(crcl(activity_self)()->container->timeout), 1);
	#if YIELDING==1
		crcl(yield)();
	#endif
	}
	//printf("%f", j);
}

int crcl(replace_main)( int argc, char **argv )
{
	int n = 1000000;
	if (argc > 1){
		n = atoi(argv[1]);
	}
    //crcl(activity_self)()->container->max_time = 0;
	struct timeval starttime;
	struct timeval endtime;
        OPA_store_int(&(crcl(activity_self)()->container->unyielding), 1);
	gettimeofday(&starttime, NULL);
        crcl(activity_t)* a = crcl(activate)(fa, (void*) &n );
        crcl(activity_t)* b = crcl(activate)(fa, (void*) &n );

        crcl(activity_join)( a );
        crcl(activity_join)( b );
	gettimeofday(&endtime, NULL);
	double diff = (endtime.tv_sec)*1000000+endtime.tv_usec - (starttime.tv_sec)*1000000+starttime.tv_usec;
	printf("%d\n", (int) diff); //microseconds
}
