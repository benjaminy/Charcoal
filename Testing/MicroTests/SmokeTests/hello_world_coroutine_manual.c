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

crcl(frame_p) crcl(fn_main_yielding)( crcl(frame_p) frame )
{
    union main_locals *locals = (union main_locals *)&frame->locals;
    printf( "Hello World %d %s\n", locals->args.argc, locals->args.argv[0] );
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
