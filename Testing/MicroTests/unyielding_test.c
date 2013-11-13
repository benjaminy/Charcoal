#include <charcoal_runtime.h>
#include <stdio.h>

void fa( void *a )
{
    int i, j;
    int* args = (int*) a;
    __charcoal_activity_t *my_activity = __charcoal_activity_self();
    for(i = 0; i < args[0]; i++){
        OPA_store_int(&(my_activity->container->unyield_depth), args[1]);
        for(j  = 0; j < args[1]; j++){
            printf("i=%d, j=%d\n", i, j);
            __charcoal_yield();
        }
        __charcoal_yield();
    }
}

int __charcoal_replace_main( int argc, char **argv )
{
    int n = 100;
    int m = 100;
    int num_activities = 10;
    

    if (argc > 3){
        n = atoi(argv[1]);
        m = atoi(argv[2]);
        num_activities = atoi(argv[3]);
    }
    int i;
    __charcoal_activity_t* activities[num_activities];
    int args[2];
    args[0] = n; args[1] = m;
    for( i = 0; i < num_activities; i++){
        char buffer[16];
        activities[i] = __charcoal_activate(fa, (void*) args );
        if(i==0){
            activities[i]->container->max_time = 0; //data races?
        }
    }

    for(i = 0; i < num_activities; i++){
        __charcoal_activity_join(activities[i]);
    }

    return 0;
}
