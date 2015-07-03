#ifndef __ATOMICS_WRAPPERS
#define __ATOMICS_WRAPPERS

/*
 * I'd really like to use C11 atomics, but cil doesn't seem to support
 * them.  So Here are some definitons that should be identical to those
 * in OpenPA.
 */

typedef struct { volatile int v; } atomic_int;

int atomic_load_int( atomic_int * );
void atomic_store_int( atomic_int *, int );

#endif /* __ATOMICS_WRAPPERS */
