#include <crcl_setjmp.h>

void crcl(setjmp_yielding)( crcl(framp_p) frm, void *return_addr, jmp_buf env, int *lhs )
{
    assert( frm );
    assert( return_addr );
    assert( env );
    env->yielding_tag           = 1;
    env->_.yielding.frm         = frm;
    env->_.yielding.return_addr = return_addr;
    env->_.yielding.lhs         = lhs;
}

/*
 * Note: Cannot make a no-yield mode wrapper procedure for setjmp
 * (returning from the wrapper would ruin the stack).  Here is
 * pseudocode for the compiler-generated code:
 *   env->yielding_tag = 0;
 *   lhs = setjmp( env->_.no_yield_env );
 */

/*
 * This helper is used whenever the destination call was made in yielding mode.
 */
static crcl(frame_p) longjmp_helper( activity_p act, jump_buf env, int val )
{
    if( env->_.yielding.lhs )
    {
        *env->_.yielding.lhs = val;
    }
    crcl(frame_p) dest_frm = env->_.yielding.frm;
    dest->return_addr = env->_.yielding.return_addr;
    while( act->newest_frame != dest_frm )
    {
        act->newest_frame = crcl(fn_generic_epilogue)( act->newest_frame );
    }
    return dest_frm;
}

crcl(frame_p) crcl(longjmp_yielding)( crcl(frame_p) frm, jump_buf env, int val )
{
    assert( frm );
    assert( env->yielding_tag ); /* jmp'ing from yielding to no-yield doesn't make sense */
    return longjmp_helper( frm->act, env, val );
}

void crcl(longjmp_no_yield)( jump_buf env, int val )
{
    if( env->yielding_tag )
    {
        activity_p act = crcl(get_self_activity)();
        longjmp_helper( act, env, val );
        longjmp( act->thread->thread_main_jmp_buf, 1 );
    }
    else
    {
        longjmp( env->_.no_yield_env, val );
    }
}
