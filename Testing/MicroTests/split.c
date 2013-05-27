
#define USING_SPLIT_STACK
#ifdef USING_SPLIT_STACK

/* FIXME: These are not declared anywhere.  */

extern void __splitstack_getcontext(void *context[10]);

extern void __splitstack_setcontext(void *context[10]);

extern void *__splitstack_makecontext(size_t, void *context[10], size_t *);

extern void * __splitstack_resetcontext(void *context[10], size_t *);

extern void *__splitstack_find(void *, void *, size_t *, void **, void **,
			       void **);

extern void __splitstack_block_signals (int *, int *);

extern void __splitstack_block_signals_context (void *context[10], int *,
						int *);

#endif

#ifndef PTHREAD_STACK_MIN
# define PTHREAD_STACK_MIN 8192
#endif

#if defined(USING_SPLIT_STACK) && defined(LINKER_SUPPORTS_SPLIT_STACK)
# define StackMin PTHREAD_STACK_MIN
#else
# define StackMin 2 * 1024 * 1024
#endif

#if 0

// Switch context to a different goroutine.  This is like longjmp.
static void runtime_gogo(G*) __attribute__ ((noinline));
static void
runtime_gogo(G* newg)
{
#ifdef USING_SPLIT_STACK
	__splitstack_setcontext(&newg->stack_context[0]);
#endif
	g = newg;
	newg->fromgogo = true;
	fixcontext(&newg->context);
	setcontext(&newg->context);
	runtime_throw("gogo setcontext returned");
}



// Save context and call fn passing g as a parameter.  This is like
// setjmp.  Because getcontext always returns 0, unlike setjmp, we use
// g->fromgogo as a code.  It will be true if we got here via
// setcontext.  g == nil the first time this is called in a new m.
static void runtime_mcall(void (*)(G*)) __attribute__ ((noinline));
static void
runtime_mcall(void (*pfn)(G*))
{
	M *mp;
	G *gp;
#ifndef USING_SPLIT_STACK
	int i;
#endif

	// Ensure that all registers are on the stack for the garbage
	// collector.
	__builtin_unwind_init();

	mp = m;
	gp = g;
	if(gp == mp->g0)
		runtime_throw("runtime: mcall called on m->g0 stack");

	if(gp != nil) {

#ifdef USING_SPLIT_STACK
		__splitstack_getcontext(&g->stack_context[0]);
#else
		gp->gcnext_sp = &i;
#endif
		gp->fromgogo = false;
		getcontext(&gp->context);

		// When we return from getcontext, we may be running
		// in a new thread.  That means that m and g may have
		// changed.  They are global variables so we will
		// reload them, but the addresses of m and g may be
		// cached in our local stack frame, and those
		// addresses may be wrong.  Call functions to reload
		// the values for this thread.
		mp = runtime_m();
		gp = runtime_g();

		if(gp->traceback != nil)
			gtraceback(gp);
	}
	if (gp == nil || !gp->fromgogo) {
#ifdef USING_SPLIT_STACK
		__splitstack_setcontext(&mp->g0->stack_context[0]);
#endif
		mp->g0->entry = (byte*)pfn;
		mp->g0->param = gp;

		// It's OK to set g directly here because this case
		// can not occur if we got here via a setcontext to
		// the getcontext call just above.
		g = mp->g0;

		fixcontext(&mp->g0->context);
		setcontext(&mp->g0->context);
		runtime_throw("runtime: mcall function returned");
	}
}


void
runtime_tracebackothers(G * volatile me)
{
	G * volatile gp;
	Traceback tb;
	int32 traceback;

	tb.gp = me;
	traceback = runtime_gotraceback();
	for(gp = runtime_allg; gp != nil; gp = gp->alllink) {
		if(gp == me || gp->status == Gdead)
			continue;
		if(gp->issystem && traceback < 2)
			continue;
		runtime_printf("\n");
		runtime_goroutineheader(gp);

		// Our only mechanism for doing a stack trace is
		// _Unwind_Backtrace.  And that only works for the
		// current thread, not for other random goroutines.
		// So we need to switch context to the goroutine, get
		// the backtrace, and then switch back.

		// This means that if g is running or in a syscall, we
		// can't reliably print a stack trace.  FIXME.
		if(gp->status == Gsyscall || gp->status == Grunning) {
			runtime_printf("no stack trace available\n");
			runtime_goroutinetrailer(gp);
			continue;
		}

		gp->traceback = &tb;

#ifdef USING_SPLIT_STACK
		__splitstack_getcontext(&me->stack_context[0]);
#endif
		getcontext(&me->context);

		if(gp->traceback != nil) {
			runtime_gogo(gp);
		}

		runtime_printtrace(tb.locbuf, tb.c, false);
		runtime_goroutinetrailer(gp);
	}
}


// Called to start an M.
void*
runtime_mstart(void* mp)
{
	m = (M*)mp;
	g = m->g0;

	initcontext();

	g->entry = nil;
	g->param = nil;

	// Record top of stack for use by mcall.
	// Once we call schedule we're never coming back,
	// so other calls can reuse this stack space.
#ifdef USING_SPLIT_STACK
	__splitstack_getcontext(&g->stack_context[0]);
#else
	g->gcinitial_sp = &mp;
	// Setting gcstack_size to 0 is a marker meaning that gcinitial_sp
	// is the top of the stack, not the bottom.
	g->gcstack_size = 0;
	g->gcnext_sp = &mp;
#endif
	getcontext(&g->context);

	if(g->entry != nil) {
		// Got here from mcall.
		void (*pfn)(G*) = (void (*)(G*))g->entry;
		G* gp = (G*)g->param;
		pfn(gp);
		*(int*)0x21 = 0x21;
	}
	runtime_minit();

#ifdef USING_SPLIT_STACK
	{
	  int dont_block_signals = 0;
	  __splitstack_block_signals(&dont_block_signals, nil);
	}
#endif

	// Install signal handlers; after minit so that minit can
	// prepare the thread to be able to handle the signals.
	if(m == &runtime_m0)
		runtime_initsig();

	schedule(nil);

	// TODO(brainman): This point is never reached, because scheduler
	// does not release os threads at the moment. But once this path
	// is enabled, we must remove our seh here.

	return nil;
}



// The goroutine g is about to enter a system call.
// Record that it's not using the cpu anymore.
// This is called only from the go syscall library and cgocall,
// not from the low-level system calls used by the runtime.
//
// Entersyscall cannot split the stack: the runtime_gosave must
// make g->sched refer to the caller's stack segment, because
// entersyscall is going to return immediately after.
// It's okay to call matchmg and notewakeup even after
// decrementing mcpu, because we haven't released the
// sched lock yet, so the garbage collector cannot be running.

void runtime_entersyscall(void) __attribute__ ((no_split_stack));

void
runtime_entersyscall(void)
{
	uint32 v;

	if(m->profilehz > 0)
		runtime_setprof(false);

	// Leave SP around for gc and traceback.
#ifdef USING_SPLIT_STACK
	g->gcstack = __splitstack_find(nil, nil, &g->gcstack_size,
				       &g->gcnext_segment, &g->gcnext_sp,
				       &g->gcinitial_sp);
#else
	g->gcnext_sp = (byte *) &v;
#endif

	// Save the registers in the g structure so that any pointers
	// held in registers will be seen by the garbage collector.
	getcontext(&g->gcregs);

	g->status = Gsyscall;

	// Fast path.
	// The slow path inside the schedlock/schedunlock will get
	// through without stopping if it does:
	//	mcpu--
	//	gwait not true
	//	waitstop && mcpu <= mcpumax not true
	// If we can do the same with a single atomic add,
	// then we can skip the locks.
	v = runtime_xadd(&runtime_sched.atomic, -1<<mcpuShift);
	if(!atomic_gwaiting(v) && (!atomic_waitstop(v) || atomic_mcpu(v) > atomic_mcpumax(v)))
		return;

	schedlock();
	v = runtime_atomicload(&runtime_sched.atomic);
	if(atomic_gwaiting(v)) {
		matchmg();
		v = runtime_atomicload(&runtime_sched.atomic);
	}
	if(atomic_waitstop(v) && atomic_mcpu(v) <= atomic_mcpumax(v)) {
		runtime_xadd(&runtime_sched.atomic, -1<<waitstopShift);
		runtime_notewakeup(&runtime_sched.stopped);
	}

	schedunlock();
}

// The goroutine g exited its system call.
// Arrange for it to run on a cpu again.
// This is called only from the go syscall library, not
// from the low-level system calls used by the runtime.
void
runtime_exitsyscall(void)
{
	G *gp;
	uint32 v;

	// Fast path.
	// If we can do the mcpu++ bookkeeping and
	// find that we still have mcpu <= mcpumax, then we can
	// start executing Go code immediately, without having to
	// schedlock/schedunlock.
	// Also do fast return if any locks are held, so that
	// panic code can use syscalls to open a file.
	gp = g;
	v = runtime_xadd(&runtime_sched.atomic, (1<<mcpuShift));
	if((m->profilehz == runtime_sched.profilehz && atomic_mcpu(v) <= atomic_mcpumax(v)) || m->locks > 0) {
		// There's a cpu for us, so we can run.
		gp->status = Grunning;
		// Garbage collector isn't running (since we are),
		// so okay to clear gcstack.
#ifdef USING_SPLIT_STACK
		gp->gcstack = nil;
#endif
		gp->gcnext_sp = nil;
		runtime_memclr(&gp->gcregs, sizeof gp->gcregs);

		if(m->profilehz > 0)
			runtime_setprof(true);
		return;
	}

	// Tell scheduler to put g back on the run queue:
	// mostly equivalent to g->status = Grunning,
	// but keeps the garbage collector from thinking
	// that g is running right now, which it's not.
	gp->readyonstop = 1;

	// All the cpus are taken.
	// The scheduler will ready g and put this m to sleep.
	// When the scheduler takes g away from m,
	// it will undo the runtime_sched.mcpu++ above.
	runtime_gosched();

	// Gosched returned, so we're allowed to run now.
	// Delete the gcstack information that we left for
	// the garbage collector during the system call.
	// Must wait until now because until gosched returns
	// we don't know for sure that the garbage collector
	// is not running.
#ifdef USING_SPLIT_STACK
	gp->gcstack = nil;
#endif
	gp->gcnext_sp = nil;
	runtime_memclr(&gp->gcregs, sizeof gp->gcregs);
}




// Allocate a new g, with a stack big enough for stacksize bytes.
G*
runtime_malg(int32 stacksize, byte** ret_stack, size_t* ret_stacksize)
{
	G *newg;

	newg = runtime_malloc(sizeof(G));
	if(stacksize >= 0) {
#if USING_SPLIT_STACK
		int dont_block_signals = 0;

		*ret_stack = __splitstack_makecontext(stacksize,
						      &newg->stack_context[0],
						      ret_stacksize);
		__splitstack_block_signals_context(&newg->stack_context[0],
						   &dont_block_signals, nil);
#else
		*ret_stack = runtime_mallocgc(stacksize, FlagNoProfiling|FlagNoGC, 0, 0);
		*ret_stacksize = stacksize;
		newg->gcinitial_sp = *ret_stack;
		newg->gcstack_size = stacksize;
		runtime_xadd(&runtime_stacks_sys, stacksize);
#endif
	}
	return newg;
}

/* For runtime package testing.  */

void runtime_testing_entersyscall(void)
  __asm__ (GOSYM_PREFIX "runtime.entersyscall");

void
runtime_testing_entersyscall()
{
	runtime_entersyscall();
}

void runtime_testing_exitsyscall(void)
  __asm__ (GOSYM_PREFIX "runtime.exitsyscall");

void
runtime_testing_exitsyscall()
{
	runtime_exitsyscall();
}

G*
__go_go(void (*fn)(void*), void* arg)
{
	byte *sp;
	size_t spsize;
	G *newg;
	int64 goid;

	goid = runtime_xadd64((uint64*)&runtime_sched.goidgen, 1);
	if(raceenabled)
		runtime_racegostart(goid, runtime_getcallerpc(&fn));

	schedlock();

	if((newg = gfget()) != nil) {
#ifdef USING_SPLIT_STACK
		int dont_block_signals = 0;

		sp = __splitstack_resetcontext(&newg->stack_context[0],
					       &spsize);
		__splitstack_block_signals_context(&newg->stack_context[0],
						   &dont_block_signals, nil);
#else
		sp = newg->gcinitial_sp;
		spsize = newg->gcstack_size;
		if(spsize == 0)
			runtime_throw("bad spsize in __go_go");
		newg->gcnext_sp = sp;
#endif
	} else {
		newg = runtime_malg(StackMin, &sp, &spsize);
		if(runtime_lastg == nil)
			runtime_allg = newg;
		else
			runtime_lastg->alllink = newg;
		runtime_lastg = newg;
	}
	newg->status = Gwaiting;
	newg->waitreason = "new goroutine";

	newg->entry = (byte*)fn;
	newg->param = arg;
	newg->gopc = (uintptr)__builtin_return_address(0);

	runtime_sched.gcount++;
	newg->goid = goid;

	if(sp == nil)
		runtime_throw("nil g->stack0");

	{
		// Avoid warnings about variables clobbered by
		// longjmp.
		byte * volatile vsp = sp;
		size_t volatile vspsize = spsize;
		G * volatile vnewg = newg;

		getcontext(&vnewg->context);
		vnewg->context.uc_stack.ss_sp = vsp;
#ifdef MAKECONTEXT_STACK_TOP
		vnewg->context.uc_stack.ss_sp += vspsize;
#endif
		vnewg->context.uc_stack.ss_size = vspsize;
		makecontext(&vnewg->context, kickoff, 0);

		newprocreadylocked(vnewg);
		schedunlock();

		return vnewg;
	}
}

#endif



int main( int argc, char **argv )
{
   return 0;
}
