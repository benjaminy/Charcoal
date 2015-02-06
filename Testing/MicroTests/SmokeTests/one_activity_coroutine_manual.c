#include <charcoal.h>
#include <charcoal_runtime_coroutine.h>

#include <stdio.h>

union main_locals
{
    struct
    {
        int argc;
        char **argv, **env;
    } args;
    int return_value;
};

crcl(frame_p) crcl(fn_foo_yielding)( crcl(frame_p) frame );
crcl(frame_p) crcl(fn_foo_init)( crcl(frame_p) caller, float blah );
                                    
crcl(frame_p) crcl(fn_main_yielding)( crcl(frame_p) frame )
{
    union main_locals *locals = (union main_locals *)&frame->locals;
    printf( "Hello World %d %s\n", locals->args.argc, locals->args.argv[0] );
    /* XXX leak */
    activity_p a = (activity_p)malloc( sizeof( a[0] ) );
    crcl(frame_p) foo = crcl(fn_foo_init)( frame, 4.2 );
    assert( !crcl(activate)( frame, a, foo ) );
    locals->return_value = 42;
    return frame->caller;
}

crcl(frame_p) crcl(fn_main_init)( crcl(frame_p) caller, int argc, char **argv, char **env )
{
    union main_locals *locals_ptr;
    size_t s = sizeof( locals_ptr[0] );
    crcl(frame_p) self;
    __CHARCOAL_GENERIC_INIT(s, self, main);
    locals_ptr = (union main_locals *)&self->locals;
    locals_ptr->args.argc = argc;
    locals_ptr->args.argv = argv;
    locals_ptr->args.env  = env;

    return self;
}

union foo_locals
{
    float args;
    float return_value;
};

crcl(frame_p) crcl(fn_foo_yielding)( crcl(frame_p) frame )
{
    union foo_locals *locals = (union foo_locals *)&frame->locals;
    printf( "Activity!!! %f\n", locals->args );
    locals->return_value = locals->args + 1.7;
    return activity_finished( frame );
}

crcl(frame_p) crcl(fn_foo_init)( crcl(frame_p) caller, float blah )
{
    union foo_locals *locals;
    size_t s = sizeof( locals[0] );
    crcl(frame_p) self;
    __CHARCOAL_GENERIC_INIT(s, self, main);
    locals = (union foo_locals *)&self->locals;
    locals->args = blah;

    return self;
}
                                    
