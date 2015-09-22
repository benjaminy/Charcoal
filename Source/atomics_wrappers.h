#ifndef __ATOMICS_WRAPPERS
#define __ATOMICS_WRAPPERS

#include <opa_primitives.h>

#define atomic_load_int( p )     ( OPA_load_int( (OPA_int_t *)( p ) ) )
#define atomic_store_int( p, i ) ( OPA_store_int( (OPA_int_t *)( p ), i ) )
#define atomic_incr_int( p )     ( OPA_incr_int( (OPA_int_t *)( p ) ) )
#define atomic_decr_int( p )     ( OPA_decr_int( (OPA_int_t *)( p ) ) )

#endif /* __ATOMICS_WRAPPERS */
