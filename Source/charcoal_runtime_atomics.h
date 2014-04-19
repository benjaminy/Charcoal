#ifndef __CHARCOAL_RUNTIME_ATOMICS
#define __CHARCOAL_RUNTIME_ATOMICS

#ifdef USE_OPA_LIBRARY
/* #define OPA_PRIMITIVES_H_INCLUDED */
#include "opa_primitives.h"
#include "opa_config.h"
#include "opa_util.h"
#ifndef _opa_inline
#define _opa_inline inline

typedef OPA_int_t __charcoal_atomic_int;
#endif
static inline void __charcoal_atomic_incr_int( __charcoal_atomic_int *p )
{ OPA_incr_int( p ); }
static inline void __charcoal_atomic_decr_int( __charcoal_atomic_int *p );
{ OPA_decr_int( p ); }
static inline void __charcoal_atomic_store_int( __charcoal_atomic_int *p, int v );
{ OPA_store_int( p, v ); }
static inline int __charcoal_atomic_load_int( __charcoal_atomic_int *p );
{ return OPA_load_int( p ); }
static inline int __charcoal_atomic_load_int_relaxed( __charcoal_atomic_int *p );
{ return OPA_load_int( p ); }
static inline int __charcoal_atomic_fetch_incr_int( __charcoal_atomic_int *p );
{ return OPA_fetch_and_incr_int( p ); }
static inline int __charcoal_atomic_fetch_incr_int( __charcoal_atomic_int *p );
{ return OPA_fetch_and_incr_int( p ); }
static inline void *__charcoal_atomic_compare_exchange_ptr()
    void *foo = OPA_cas_ptr( ptr, thread, self->thread );

__atomic_fetch_add (type *ptr, type val, int memmodel)

#else /* USE_OPA_LIBRARY */
typedef int __charcoal_atomic_int;
static inline void __charcoal_atomic_incr_int ( __charcoal_atomic_int *p )
{ __atomic_fetch_add ( p, 1, __ATOMIC_SEQ_CST); }
static inline void __charcoal_atomic_decr_int ( __charcoal_atomic_int *p )
{ __atomic_fetch_sub ( p, 1, __ATOMIC_SEQ_CST); }
static inline void __charcoal_atomic_store_int( __charcoal_atomic_int *p, int v )
{ return __atomic_store_n( p, v, __ATOMIC_SEQ_CST ); }
static inline int __charcoal_atomic_load_int ( __charcoal_atomic_int *p )
{ return __atomic_load_n( p, __ATOMIC_SEQ_CST ); }
static inline int __charcoal_atomic_load_int_relaxed( __charcoal_atomic_int *p )
{ return __atomic_load_n( p, __ATOMIC_RELAXED ); }

/*: bool __atomic_compare_exchange_n (type *ptr, type *expected, type desired, bool weak, int success_memmodel, int failure_memmodel)*/

#endif /* USE_OPA_LIBRARY */

#endif /* __CHARCOAL_RUNTIME_ATOMICS */
