
#include <charcoal_runtime.h>
#include <stdio.h>

void fa( void *a )
{
	double max_time = ((double*) a)[0];
	unsigned int sleeptime = 3*((int) max_time);
	__charcoal_yield();
	sleep(sleeptime);
	__charcoal_yield();
	printf("Ending current thread\n");
}

int __charcoal_replace_main( int argc, char **argv )
{
    int num_activities = 2;
	double max_time = 2; //seconds

    if (argc > 1){
        num_activities = atoi(argv[1]);
    }
    int i;
    __charcoal_activity_t* activities[num_activities];
    for( i = 0; i < num_activities; i++){
        char buffer[16];
        activities[i] = __charcoal_activate(fa, (void*) &max_time );
        if(i==0){
            activities[i]->container->max_time = max_time; //data races?
        }
    }

    for(i = 0; i < num_activities; i++){
        __charcoal_activity_join(activities[i]);
    }

    return 0;
}
