#ifndef ASSERT_EFC_H
#define ASSERT_EFC_H

/* The three dimensions of asserts: effects, fatality, cost
 * Effects:
 * - E: "Effectful". The expression must be evaluated for effects,
 *      whether it's checked or not.
 * - R: "Read-only". The expression doesn't change the state of the
 *      program.  Don't evaluate it if we're not enforcing it.
 * Fatality:
 * - F: "Fatal". Assertion failure causes the program to crash, because
 *      something _really bad_ might happen if the program continues.
 * - S: "Survivable". The consequences of continuing are tolerable.  Log
 *      instead of crashing in non-debug builds.
 * Cost:
 * - H: "High". Only appropriate for debug builds.
 * - L: "Low". Can be included in production builds.
 */

extern "C" {
extern void __assert_fail (__const char *__assertion, __const char *__file,
			   unsigned int __line, __const char *__function)
     __THROW __attribute__ ((__noreturn__));
}

/* Version 2.4 and later of GCC define a magical variable `__PRETTY_FUNCTION__'
   which contains the name of the function currently being defined.
   This is broken in G++ before version 2.6.
   C9x has a similar variable called __func__, but prefer the GCC one since
   it demangles C++ function names.  */
# if defined __cplusplus ? __GNUC_PREREQ (2, 6) : __GNUC_PREREQ (2, 4)
#   define __ASSERT_FUNCTION	__PRETTY_FUNCTION__
# else
#  if defined __STDC_VERSION__ && __STDC_VERSION__ >= 199901L
#   define __ASSERT_FUNCTION	__func__
#  else
#   define __ASSERT_FUNCTION	((__const char *) 0)
#  endif
# endif

#define assert_EF( e, ... ) \
    ({ typeof( e ) x = e; \
       if( !x ) \
           __assert_fail( __STRING( e ) "(" __VA_ARGS__")", \
                          __FILE__, __LINE__, __ASSERT_FUNCTION ); \
       x; })

#ifdef NDEBUG
/* XXX: Should really log, not crash: */
#define assert_ES( e, ... ) \
    ({ typeof( e ) x = e; \
       if( !x ) \
           __assert_fail( __STRING( e ) "(" __VA_ARGS__")", \
                          __FILE__, __LINE__, __ASSERT_FUNCTION ); \
       x; })
#ifdef __OPTIMIZE__
#define assert_RFL( e, ... ) (void)
#define assert_RFH( e, ... ) (void)
#define assert_RSL( e, ... ) (void)
#define assert_RSH( e, ... ) (void)
#else /* __OPTIMIZE__ */
#define assert_RFL( e, ... ) assert_EF( e, __VA_ARGS__ )
#define assert_RFH( e, ... ) (void)
#define assert_RSL( e, ... ) assert_ES( e, __VA_ARGS__ )
#define assert_RSH( e, ... ) (void)
#endif /* __OPTIMIZE__ */

#else /* NDEBUG */
#define assert_ES( e, ... ) assert_EF( e, __VA_ARGS__ )
#define assert_RFL( e, ... ) assert_EF( e, __VA_ARGS__ )
#define assert_RFH( e, ... ) assert_EF( e, __VA_ARGS__ )
#define assert_RSL( e, ... ) assert_EF( e, __VA_ARGS__ )
#define assert_RSH( e, ... ) assert_EF( e, __VA_ARGS__ )
#endif /* NDEBUG */

#define assert_if_EF( e1, e2, ... ) assert_EF( ( ( !e1 ) || e2 ), __VA_ARGS__ )
#define assert_if_ES( e1, e2, ... ) assert_ES( ( ( !e1 ) || e2 ), __VA_ARGS__ )
#define assert_if_RFL( e1, e2, ... ) assert_RFL( ( ( !e1 ) || e2 ), __VA_ARGS__ )
#define assert_if_RFH( e1, e2, ... ) assert_RFH( ( ( !e1 ) || e2 ), __VA_ARGS__ )
#define assert_if_RSL( e1, e2, ... ) assert_RSL( ( ( !e1 ) || e2 ), __VA_ARGS__ )
#define assert_if_RSH( e1, e2, ... ) assert_RSH( ( ( !e1 ) || e2 ), __VA_ARGS__ )

#endif
