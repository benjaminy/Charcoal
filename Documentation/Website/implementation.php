<html>
<head>
<title>Implementation</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<div class="side_links">
<a href="index.html">Charcoal</a><br/>
- <a href="short_version.html">Why Charcoal?</a><br/>
- <a href="some_examples.html">Examples</a><br/>
- <a href="concurrency.html">Concurrency</a><br/>
- <a href="big_four.html">vs. Threads, etc.</a><br/>
- <a href="implementation.html">Implementation</a><br/>
&mdash; <a href="implementation.html#translation">Translation to C</a><br/>
&mdash; <a href="implementation.html#runtime">Runtime Library</a><br/>
&mdash; <a href="implementation.html#syscalls">Syscalls and Signals</a><br/>
- <a href="faq.html">FAQ</a>
</div>

<div class="main_div">

<h1>Charcoal Implementation</h1>

<p>
The current Charcoal implementation consists of three pieces:
</p>
<ul>
<li>Translation</li>
<li>Runtime System</li>
<li>Standard Library</li>
</ul>

<p>
The target of the translation is plain C.  In principle the Charcoal
compiler could generate machine code (or some intermediate code like
LLVM), but there is nothing particularly unusual about code generation
for Charcoal.  During the translation, calls to some special runtime
system procedures are added so compiled Charcoal applications needs to
be linked with the Charcoal runtime.  Finally, Charcoal code can call
regular C libraries, but we provide slightly tweaked versions that are
better organized for interactive software.
</p>

<p>
The most interesting features of Charcoal from an implementation
perspective are:
</p>
<ul>
<li>Call frame management (a.k.a. coroutinization)</li>
<li>Activity extraction</li>
<li>Yield implementation</li>
<li>System interface</li>
</ul>

<div class="hblock2">
<a id="call_frames"/>
<h2 style="display:inline;">Call Frame Management</h2>

<p>One of the most important practical problems faced in the
implementation of any thread-like concurrency framework is the
management of procedure call frames.  The conventional strategy for
sequential software is to
use <a href="http://en.wikipedia.org/wiki/Call_stack">The Stack</a>
(briefly summarized below).  Stack-based call frame allocation leads to
some unpleasant implementation choices for multithreaded software, so we
are experimenting with a variant of heap-allocated call frames.</p>

<div class="hblock3">
<h3>Background: The Call Stack</h3>

<p>
The classical approach to call frame management is to use a single large
contiguous region of memory (often half of the entire virtual address
space) for <em>the stack</em>.  When one procedure calls another, the
callee's frame is allocated (pushed) directly adjacent to the caller's.
This strategy works well with a single thread of control, because if a
given call frame is still live that means its caller's frame (and its
caller's caller's &hellip;) must still be live.
</p>

<p>
Stack-based frame management is extremely efficient in terms of time and
space overhead.  Time overhead is low because allocation and
deallocation can be implemented with a single instruction: incrementing
or decrementing the <em>stack pointer</em> by the appropriate number of
bytes.  Space overhead is low because there is no need for allocation
metadata; the size of a frame can be encoded in the instruction stream.
More importantly, for most intents and purposes there is no
fragmentation.  Call frames use exactly the amount of space they need
and the frames are packed next to each other with no space in between.
</p>

<p>
Stack-based frame management runs into some problems with multithreaded
software.  Each thread needs its own region of memory for call frames,
which makes it impossible to use a single huge region for "the" stack.
The most conventional approach is to allocate a moderately large (on the
order of megabytes) region from the heap at thread spawn time.  The
sizing of these allocations is a tricky engineering problem.  If they
are too large, there will be a lot of internal memory fragmentation.  If
they are too small, a thread may overflow its stack memory, leading to a
hard crash (or even worse, unpredictable bizarre behavior).  In current
practice, it is common to make these allocations relatively large and
limit the number of concurrently active threads.
</p>

<p>
(Fun little tangent: Stack-based frame management does not work
particularly well for languages with higher-order first class functions.
Some implementations of such languages use heap-based strategies kinda
sorta like what the current Charcoal implementation does.)
</p>

</div> <!-- The call stack -->

<p>
For the Charcoal implementation we are experimenting with a novel hybrid
heap/stack frame management strategy.  In the following two subsections
we discuss what's novel about the Charcoal approach and some of the
implementation details.
</p>

<div class="hblock3">
<h3>Charcoal's Hybrid Call Frame Management</h3>

<p>
"Normal" (i.e. maybe yielding) calls get translated into a
coroutine-like (or asynchronous in .NET jargon) form.  Here is a nice
blog post on some of the implementation ideas in the C#/CLR
context: <a href="http://weblogs.asp.net/dixin/understanding-c-async-await-1-compilation">Click
Here</a>
or <a href="https://www.simple-talk.com/dotnet/.net-tools/c-async-what-is-it,-and-how-does-it-work/">Here</a>.
Gabriel Kerneis has a more academic take on a very similar set of ideas:
<a href="http://gabriel.kerneis.info/software/cpc/">Click Here</a>.  The
important reason for doing this is that individual call frames end up
independently heap-allocated.  This means that we totally avoid the
memory overhead issues associated with allocating stack space for
threads.  The big down side is that procedure calls (and returns)
are <em>much</em> slower than with conventional stack allocation.
</p>

<p>
In Charcoal we can get something very near the best of both worlds,
because of the role that <em>unyielding</em> procedures play in the
language.  We start the argument by observing that procedure call
overhead is only an important performance issue if calls and returns are
happening quite frequently.  Frequent calls can only happen if most of
the procedure executions are very short-lived.  So we really need a way
to speed up short-lived procedure executions (relative to an
implementation where we pay the higher price for <em>every</em> call and
return).
</p>

<p>
This is where unyielding comes in.  In Charcoal it is good practice to
declare short-lived procedures to be unyielding.  This makes such
procedures activity-atomic.  Because unyielding procedures cannot be
interrupted, we do not need to do the coroutine transformation on them.
This means that calls to unyielding procedures can be implemented with
the regular old stack allocation strategy, which makes them fast
(compared to maybe-yielding calls).  A consequence of this strategy is
that in a Charcoal process each <em>thread</em> needs a pre-allocated
block of memory for stacks, not every <em>activity</em>.  Well-tuned
applications should have very few threads and potentially many
activities, so this works out perfectly.
</p>

<p>
The important observation here from a language design perspective is the
fortunate coincidence that the procedures that we want to implement with
stack-based frame allocation (i.e. the short-lived ones) are the very
same procedures that make sense to execute atomically for concurrency
bug avoidance reasons.  In Charcoal we get these two useful effects for
the price of a single programmer-visible annotation (unyielding).
</p>

<p>
It's interesting to notice that what we have here is very similar to the
C# async/await implementation, but with the defaults reversed.  In
Charcoal, "plain" calls are assumed to be maybe-yielding and are
translated into coroutine form.  In C#, only calls to "async" procedures
get translated.  I think Charcoal's default is better, but have very
little data to back up that claim.
</p>

<div class="hblock4">
<h4>Implications for API design</h4>

<p>
As described above, Charcoal strongly motivates programmers to declare
"fast" procedures to be unyielding.  This is fine as far as it goes, but
it's common for procedures to have a fast normal execution path and one
or more exceptional execution paths that are "slow" (e.g. might perform
blocking I/O operations).
</p>

<p>
It's not immediately obvious what the "declare fast procedures
unyielding" guideline means for such fast/slow procedures.  Simply
declaring them unyielding is not good, because hitting the uncommon slow
path might cause unresponsiveness.  On the other hand, <em>not</em>
declaring such procedures unyielding might add non-trivial overhead in
the common case.
</p>

<p>
Perhaps it's better to refactor such API procedures into two: One "best
effort" version that is unyielding and always fast, but might fail.  And
one "always works" version that client code can call when the best
effort version fails.  In fact, maybe there should be a macro that
automates the "if best-effort fails, call always-works" pattern.  I'm
not certain this pattern is a good idea, but it's worth investigating.
</p>

<p>
A simple example of this pattern shows up in many mutex APIs.  It is
common to have both a "might block" version that absolutely won't return
until the mutex is acquired and a "try" version that is guaranteed to
return very quickly, but might not actually acquire the mutex.
</p>

</div> <!-- API design -->

</div> <!-- Hybrid heap/stack -->

<div class="hblock4">
<h4>Implementing procedure calls in Charcoal</h4>

<p>
The compiler automatically translates procedures in to a
"coroutine-like" form.  Some blog posts refer to the result of this
transformation as a "finite state machine", which seems like an odd use
of that term to me, but whatever.
</p>

</div> <!-- Call protocol implementation -->

</div> <!-- Call frame management -->

<p>...</p>

<div class="hblock2">
<a id="activity_extraction"/>
<h2 style="display:inline;">Activity Extraction</h2>

</div> <!-- Activity extraction -->

<p>...</p>

<div class="hblock2">
<a id="yield_implementation"/>
<h2 style="display:inline;">Yield Implementation</h2>

</div> <!-- Yield implementation -->

<p>...</p>

<div class="hblock2">
<a id="system_interface"/>
<h2 style="display:inline;">System Interface</h2>

<p>
All Charcoal programs have an "extra" thread that is created at startup.
This thread handles I/O and is a timer that tells other threads when
they should switch from one activity to another.
</p>

</div> <!-- System interface -->

<p>...</p>

<div class="hblock2">
<a id="translation"/>
<h2 style="display:inline;">Charcoal to C Translation</h2>

<ul>
<li>activate extraction
<li>yield insertion
<li>unyielding translation
</ul>

<div class="hblock3">
<h3>Activate</h3>

<p>The core piece of new syntax added in Charcoal is the activate
expression.  It looks like:</p>

<div class="highlight mono">
<b><u>activate</b></u> (<b>x</b>,...,<b>z</b>) <i>[&lt;expression&gt;|&lt;statement&gt;]</i>
</div>

<p>When this expression executes it starts a new activity to evaluate
its body, which can be an arbitrary statement or expression.  The whole
expression evaluates to a pointer to the new activity (which is an
opaque type).</p>

<p>The body of an activate expression is free to refer to local
variables that are in scope wherever the expression appears.  The
programmer has to choose whether the new activity gets these variables
by-reference or by-value.  The default is by-reference; in this case,
changes to the variable in either the "main" code or the activated code
will be observed by the other.  The new activity gets its own private
copy of the variables that appear in the list directly following the
activate keyword.  The initial value of these by-value variables is
whatever the corresponding variable's value was at the point when the
activity was created.</p>

<p>(In general, Charcoal programmers need to be very careful about
accessing local variables from the enclosing scope in activities.  These
variables are stack-allocated, so they'll go away as soon at the
procedure call that created the activity returns.)</p>

<p>Here are the steps used to translate activate expressions to plain
C.</p>


<div class="hblock4">
<h4>Expression to Statement</h4>

<p>Activating an expression is defined to be the same as activating a
statement that returns that expression.  After performing this
transformation, all activate bodies are statements.</p>

<div class="highlight mono">
<b><u>activate</b></u> (<b>x</b>,...,<b>z</b>) <i>&lt;expression&gt;</i>
<br/><hr/>
<b><u>activate</b></u> (<b>x</b>,...,<b>z</b>) <b><u>return</b></u> <i>&lt;expression&gt;</i>
</div>
</div>

<br/>

<div class="hblock4">
<h4>"Returning" from an Activity</h4>

<p>Return statements in the scope of an activated statement do not cause
the enclosing function to return; it's not clear how that would even
work.  Returning from an activity has 2 effects: (1) the returned value
is saved in the activity's backing memory; (2) the activity stops
executing.  The returned value can then be accessed later by other
activities.  This is especially common when using the "future" pattern.
Exactly when an activity's backing memory gets deallocated is discussed
elsewhere.</p>

<p>(This translation only happens
within <span class="mono">activate</span> bodies.)</p>

<div class="highlight mono">
<b><u>return</b></u> <i>&lt;expression&gt;</i>
<br/><hr/>
{&nbsp;self->rv = <i>&lt;expression&gt;</i>;<br/>
&nbsp;&nbsp;exit_activity; }
</div>
</div>

<br/>

<div class="hblock4">
<h4>Variable Binding</h4>

<div class="highlight mono">
<b><u>activate</u></b> (<b>x</b>,...,<b>z</b>) <i>&lt;statement&gt;</i>
<br/><hr/>
args.a = &a; ... args.c = &c;<br/>
args.x = x; ... args.z = z;<br/>
<b><u>activate</b></u> {<br/>
&nbsp;&nbsp;x = args.x; ... z = args.z;<br/>
&nbsp;&nbsp;aptr = args.a; ... cptr = args.c;<br/>
&nbsp;&nbsp;<i>&lt;statement&gt;</i>[*aptr/a,...,*cptr/c]<br/>}
</div>
</div>

<br/>

<div class="hblock4">
<h4>Activate Body Extraction</h4>

<p>In order to get to plain C, we extract the body of an activate
statement out into a function.</p>

<div class="highlight mono">
<b><u>activate</b></u> <i>&lt;statement&gt;</i>
<br/><hr/>
void __charcoal_activity_NNN( args )<br/>
{ <i>&lt;statement&gt;</i> }<br/>
...<br/>
&nbsp;&nbsp;__charcoal_activate( __charcoal_activity_NNN, args )</span>
</div>
</div>
</div>

<div class="hblock3">
<h3>Yield Insertion</h3>

<p>The most novel part of Charcoal is that the language has implicit
yield invocations scattered about hither and yon.  This is what makes
the scheduling of Charcoal's activities a hybrid of preemptive and
cooperative.  They are cooperative in the sense that an activity must
yield in order for control to transfer to another activity (and it's
always possible to avoid/suppress yielding).  They are preemptive in the
sense that <em>by default</em> yield is implicitly invoked before
certain control flow transfers.  This gives activities a kind of "chunky"
preemptiveness that we hope will be easier for application programmers
to work with than conventional preemptive threads.</p>

<p>The intention is that most of the time you just don't have to worry
too much about either accidentally starving other activities (which can
be a problem with pure cooperative models) or invariants being violated
by another activity/thread interrupting at just the wrong time (which
can be a problem with pure preemptive models).</p>

<p>To be a little more precise, Charcoal has a yield on every
"backwards" control flow transition.  Important side note: this is
default behavior which can be overridden in variety of ways.  Every loop
(for, while) has a yield after every iteration.  gotos that go "up" in
the source have a yield.  Recursive procedure calls have a yield on both
call and return.</p>

<p>Indirect calls (e.g. through a function pointer) are a somewhat
tricky subject.  The easiest thing for the programmer is to assume that
all indirect calls have yields on call and return by default.</p>

<p>One huge difference between activities and conventional preemptive
threads is that data races
(<a href="http://blog.regehr.org/archives/490">ref 1</a>,
<a href="http://static.usenix.org/event/hotpar11/tech/final_files/Boehm.pdf">ref
2</a>) between activities simply do not exist.  Higher-level race
conditions are still possible, of course, but not having to worry about
low-level data races is a pretty big win.</p>
</div>

<br/>

<div class="hblock3">
<h3>Unyielding</h3>

<p>Charcoal comes with the usual set of synchronization primitives
(mutexes, condition variables, semaphores, barriers), also a nice set of
channel-based message passing primitives inspired by CML.  However,
quite often simpler and less error-prone synchronization patterns can
be used.</p>

<p>As we saw in the first example, simple shared memory accesses can
often go without any extra synchronization at all.  If you need a larger
section of code to execute atomically (relative to peer activities), you
can use the <span class="mono">unyielding</span>
keyword.  <span class="mono">unyielding</span> can be
applied to statements, expressions and procedure declarations.  In all
cases the effect is similar: the relevant statement/expression/procedure
will execute without yielding control to another activity, even if it
executes a yield statement.</p>

<div class="highlight mono">
<b><u>unyielding</u></b> <i>&lt;expression&gt;</i>
<br/><hr/>
({<pre> </pre>typeof( <i>&lt;expression&gt;</i> ) __charcoal_uny_var_NNN;<br/>
<pre>   </pre>__charcoal_unyielding_enter();<br/>
<pre>   </pre>__charcoal_uny_var_NNN = <i>&lt;expression&gt;</i>;<br/>
<pre>   </pre>__charcoal_unyielding_exit();<br/>
<pre>   </pre>__charcoal_uny_var_NNN })
</div>

<div class="highlight mono">
<b><u>unyielding</b></u> <i>&lt;statement&gt;</i>
<br/><hr/>
{&nbsp;unyielding_enter;<br/>
&nbsp;&nbsp;<i>&lt;statement&gt;</i>;<br/>
&nbsp;&nbsp;unyielding_exit&nbsp;}
</div>

<div class="highlight mono">
<b><u>unyielding</u></b> <i>&lt;fun decl&gt;</i> <i>&lt;fun body&gt;</i>
<br/><hr/>
<i>&lt;fun decl&gt;</i><br/>
{&nbsp;unyielding_enter;<br/>
&nbsp;&nbsp;<i>&lt;fun body&gt;</i>;<br/>
&nbsp;&nbsp;unyielding_exit&nbsp;}</span>
</div>

<p>In all three cases, the "body" of
the <span class="mono">unyielding</span> annotation
must be scanned for control transfers that might escape the body
(<span class="mono">return</span>,
<span class="mono">break</span>,
<span class="mono">goto</span>, ...).
An <span class="mono">unyielding_exit</span> is
inserted directly before such transfers.</p>

<div class="hblock4">
<h4>When Should Unyielding Be Used?</h4>

<p>Unyielding can be used to make arbitrary chunks of code
"activity-safe", but some care needs to be taken in its use.  A good
example of its use is in a binary tree library.  Procedures that modify
a tree can/should be marked unyielding.</p>

<p>Probably most simple leaf procedures should be marked unyielding.<p>

<p>You need to be a little careful with
the <span class="mono">unyielding</span> annotation.
It's easy to starve other activities by entering an unyielding
statement, then executing a long-running loop or making a blocking
syscall.  The situation to be especially vigilant about is using
unyielding on a block that usually executes reasonably quickly, but has
a path that is both infrequently executed and long-running/blocking.</p>

<p>Marking an expression/statement/procedure unyielding is very cheap.
In the worst case you add an increment and decrement of a thread-local
variable.  If the compiler can see nested unyielding statements, it can
optimize them out pretty easily, too.</p>
</div>
</div>

<div class="hblock3">
<h3>Synchronized</h3>

<div class="highlight mono">
<b><u>synchronized</u></b> (<i>&lt;expression&gt;</i>) <i>&lt;statement&gt;</i>
<br/><hr/>
({<br/>
<pre>    </pre>mutex_t *__charcoal_sync_mutex_ ## __COUNTER__ = <i>&lt;expression&gt;</i>;<br/>
<pre>    </pre>mutex_acquire( __charcoal_sync_mutex_NNN ) ? 1 :<br/>
<pre>        </pre>({ <i>&lt;statement&gt;</i>;<br/>
<pre>           </pre>mutex_release( __charcoal_sync_mutex_NNN ) ? 2 : 0 })<br/>
})
</div>
</div>
</div>

<div class="hblock2">
<a id="runtime"/>
<h2>Charcoal Runtime Library</h2>

<ul>
<li>Yield (and other concurrency primitives)
<li>Scheduler
<li>Stacks
</ul>

<div class="hblock3">
<h3>yield</h3>

<p>Well-behaved Charcoal programs invoke yield quite often (just for a
rough sense of scale, maybe a few times per microsecond on modern
processors).  This high frequency means that yield must be engineered
to:</p>

<ul>
<li>Execute extremely quickly in the common case.
<li>Not cause a context switch most of the time.
</ul>

<p>The first goal is important for basic cycle counting performance
reasons.  If yield takes more than 10 or 20 instructions in the common
case, avoiding yields will become more of a performance tuning thing in
Charcoal than I want it to be.  Need to do some testing here.</p>

<p>The second goal is important because context switching has a non-trivial
direct cost (smaller than threads, but still not zero).  More
importantly, context switching can have high indirect costs if different
activities evict each others' working data from the caches.</p>
</div>

<br/>

<div class="hblock3">
<h3>Scheduler</h3>

<p>Under programmer control?</p>

<p>Interesting things about determinism?</p>
</div>

<br/>

<div class="hblock3">
<h3>Stacks</h3>

<p>One of the reasons threads are relatively expensive in most
implementations is that a large amount of memory has to be pre-allocated
for each thread's stack.  I really wanted to avoid that, so for now
we're using gcc's <a href="http://gcc.gnu.org/wiki/SplitStacks">split
stacks</a> and/or
LLVM's <a href="http://lists.cs.uiuc.edu/pipermail/llvmdev/2011-April/039260.html">segmented
stacks</a>.  One could go really extreme and heap allocate every
procedure call record individually.  I'm not aware of an easy way to do
that today, so that's future work.</p>
</div>
</div>

<div class="hblock2">
<a id="syscalls"/>
<h2>Syscalls and Signals</h2>

<p>System calls and signals have to be handled with care in Charcoal.
System calls can block for an arbitrarily long time, which could render
a Charcoal application unresponsive if we are not careful.  Signal
delivery can interrupt a Charcoal application at an inopportune time and
violate its shared memory model if we are not careful.</p>

<div class="hblock3">
<h3>Syscalls</h3>

<p>Charcoal programs cannot be allowed to directly make syscalls that
might block for a long time, because multiple activities reside in a
single thread and a blocking syscall would block the whole thread.  When
an application makes a syscall, the Charcoal runtime system does the
following:</p>

<ol>
<li>Record the details of the syscall.</li>
<li>Arrange for the syscall to be made in a non-blocking fashion.</li>
<li>Yield.</li>
</ol>

<p>When the syscall has completed, the runtime system records the
returned value (if any) and marks the calling activity as ready to
run.</p>

<p>When application programmers really want a syscall to block all the
activities in a thread (which I imagine would be an unusual case), they
can simply wrap the call with <span class="mono">unyielding</span>.</p>

<p>Possible ways to implement syscall interception:</p>

<ul>
<li>Whole-program dynamic translation, a
la <a href="http://software.intel.com/en-us/articles/pin-a-dynamic-binary-instrumentation-tool">Pin</a>
or <a href="http://valgrind.org/">Valgrind</a></li>
<li>ptrace</li>
</ul>

<div class="hblock4">
<p>Why not just statically translate syscalls (e.g. during
compilation)?</p>

<ul>
<li>Charcoal code can call plain C functions, which the Charcoal
compiler would not have the opportunity to translate.</li>
<li>Syscalls can be made in embedded assembly, which might be annoying
to identify consistently.</li>
<li>And the really big hammer... As a C derivative, Charcoal is
reasonably friendly to nonsense like self-modifying code and dynamically
generated assembly.  No static solution will see syscalls in such
code.</li>
</ul>

<p>As a performance optimization it might make sense to statically
translate syscalls when possible and only dynamically translate in the
weird cases.</p>

</div>

<p>Ideas related to doing syscalls in a non-blocking fashion:</p>

<ul>
<li>The most straight-forward idea is for the Charcoal runtime to keep a
special OS thread (or a small pool of such threads) that are there just
to perform blocking syscalls.</li>
<li>It might be possible to leverage OS non-blocking I/O primitives to
improve performance in some cases.</li>
</ul>

</div>

<br/>

<div class="hblock3">
<h3>Signals</h3>

<p>Conventional signal handlers are not treated differently in Charcoal
than in C.  However, as discussed in one of the code examples, for
asynchronous signal handling (i.e. input from the real world) it's
better to have an activity that blocks on delivery with something
like <span class="mono">sig_wait</span>.  The Charcoal runtime has to
intercept the call to <span class="mono">sig_wait</span> and install its
own signal handler.</p>

<p>It's possible that the regular syscall interposition mechanism will
just take care of this, but it seems likely that some special treatment
will be required for signals.</p>

</div>

</div>

<?php include 'copyright.html'; ?>

</div>
</body>
</html>
