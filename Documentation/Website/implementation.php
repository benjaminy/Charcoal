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

<p>There are a few pieces to the Charcoal implementation:</p>

<ul>
<li>Charcoal to C translation
<li>Charcoal runtime library
<li>Syscalls and signals
</ul>

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
