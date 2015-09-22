#include <charcoal_runtime.h>
#include <stdio.h>
#include <math.h>

void fa( void *a )
{
	double max_time = ((double*) a)[0];
	unsigned int sleeptime =(int) 2*( max_time);
	crcl(yield)();
	sleep(sleeptime);
    int j = 0;
    int i;
    /*for(i = 0; i < pow(10,10); i++){
        //printf("%d", i);
        j++;
        crcl(yield)();
    }*/
    crcl(yield)();
	printf("Ending current thread\n");
}

int crcl(replace_main)( int argc, char **argv )
{
    int num_activities = 2;
	double max_time = 1; //seconds

    if (argc > 1){
        num_activities = atoi(argv[1]);
    }
    int i;
    crcl(activity_t)* activities[num_activities];
    crcl(activity_self)()->container->max_time = max_time;
    printf("Unyielding? %d\n",OPA_load_int(&crcl(activity_self)()->container->unyielding));
    for( i = 0; i < num_activities; i++){
        char buffer[16];
        activities[i] = crcl(activate)(fa, (void*) &max_time );
    }

    for(i = 0; i < num_activities; i++){
        crcl(activity_join)(activities[i]);
    }

    return 0;
}
