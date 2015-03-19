#ifndef __CHARCOAL_BASE
#define __CHARCOAL_BASE

/* Should be included before absolutely anything else in Charcoal source
 * files. */

/* I suspect more stuff will find its way in here, but if not maybe
 * refactor this into some other file. */
#define crcl(n) __charcoal_ ## n
#define CRCL(n) __CHARCOAL_ ## n

#define RET_IF_ERROR(cmd) \
    do { int rc; if( ( rc = cmd ) ) { return rc; } } while( 0 )

/* XXX super annoying name collision on thread_t with Mach header.
 * Look into it more some day. */
typedef struct  cthread_t  cthread_t,  *cthread_p;
typedef struct activity_t activity_t, *activity_p;

#define assert_impl(a,b) assert( ( !( a ) ) || ( b ) )

#endif /* __CHARCOAL_BASE */
