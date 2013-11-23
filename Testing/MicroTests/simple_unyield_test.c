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
		OPA_store_int(&(__charcoal_activity_self()->container->unyielding), 1);
	#if YIELDING==1
		__charcoal_yield();
	#endif
	}
	//printf("%f", j);
}

int __charcoal_replace_main( int argc, char **argv )
{
	int n = 1000000;
	if (argc > 1){
		n = atoi(argv[1]);
	}
    //__charcoal_activity_self()->container->max_time = 0;
	struct timeval starttime;
	struct timeval endtime;
        OPA_store_int(&(__charcoal_activity_self()->container->unyielding), 2);
	gettimeofday(&starttime, NULL);
    __charcoal_activity_t* a = __charcoal_activate(fa, (void*) &n );
    __charcoal_activity_t* b = __charcoal_activate(fa, (void*) &n );

    __charcoal_activity_join( a );
    __charcoal_activity_join( b );
	gettimeofday(&endtime, NULL);
	double diff = (endtime.tv_sec)*1000000+endtime.tv_usec - (starttime.tv_sec)*1000000+starttime.tv_usec;
	printf("%d\n", (int) diff); //microseconds
}
