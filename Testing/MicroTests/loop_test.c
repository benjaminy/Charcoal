#include <charcoal_runtime.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

void instantiate_buffer(char* s1, int size){
    int i;
    for(i = 0; i < size; i++){
        s1[i] = '\1';
    }
}

void loop_strcpy( void *a )
{
    int i, j;
    int* args = (int*) a;
    char* s1 = malloc(args[2]);
    char* s2 = malloc(args[2]);
    instantiate_buffer(s1, args[2]);
    __charcoal_activity_t *my_activity = __charcoal_activity_self();
    for(i = 0; i < args[0]; i++){
        for(j  = 0; j < args[1]; j++){
            //printf("i=%d, j=%d\n", i, j);
            strcpy(s2, s1);
        }
        __charcoal_yield();
    }
}

void loop_memcpy( void *a )
{
    int i, j;
    int* args = (int*) a;
    char* s1 = malloc(args[2]);
    char* s2 = malloc(args[2]);
    instantiate_buffer(s1, args[2]);
    __charcoal_activity_t *my_activity = __charcoal_activity_self();
    for(i = 0; i < args[0]; i++){
        for(j  = 0; j < args[1]; j++){
            //printf("i=%d, j=%d\n", i, j);
            memcpy(s1, s2, args[2]);
        }
        __charcoal_yield();
    }
}

int __charcoal_replace_main( int argc, char **argv )
{
    int n = 100;
    int m = 100;
    int buffer_size = 128;
    int num_activities = 2;
    void* function = loop_strcpy;

    if (argc > 3){
        n = atoi(argv[1]);
        m = atoi(argv[2]); //Second argument is inner loop size
        num_activities = atoi(argv[3]);
        if(argc > 4){
            if(strcmp(argv[4], "memcpy")==0){
                function = loop_memcpy;
            }
        }
        if(argc > 5){
            buffer_size = atoi(argv[4]);
        }
    }
    printf("n=%d, m=%d\n", n, m);
    int i;
    __charcoal_activity_t* activities[num_activities];
    int args[3];
    args[0] = n; args[1] = m; args[2] = buffer_size;
    for( i = 0; i < num_activities; i++){
        activities[i] = __charcoal_activate(function, (void*) args );
        if(i==0){
            activities[i]->container->max_time = 0; //data races?
        }
    }

    for(i = 0; i < num_activities; i++){
        __charcoal_activity_join(activities[i]);
    }

    return 0;
}
