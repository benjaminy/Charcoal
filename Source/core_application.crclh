#ifndef __CHARCOAL_NON_C
#define __CHARCOAL_NON_C

/* This file is for the strange runtime library functions that should
 * be interpreted as Charcoal functions, not C function. */

#ifdef __CHARCOAL_CIL
#pragma cilnoremove( "__charcoal_activate_intermediate" )
#endif

/* calls to "intermediate" only exist temporarily, between stages of compilation */
int crcl(activate_intermediate)( activity_p activity, ... );

#endif
