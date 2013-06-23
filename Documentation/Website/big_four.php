<html>
<head>
<title>Conventional Concurrency Primitives/Frameworks</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<?php include 'code_examples.php'; ?>

<?php include 'side_bar_basic.html'; ?>

<div class="main_div">

<h1>Conventional Concurrency Primitives/Frameworks</h1>

<p>The classical interaction model between software and its environment
is that of mathematical functions: the environment provides an input,
the software spends some time computing, then makes an output available
to the environment.  Functions (and their programming cousins,
procedures, subroutines, methods) are astoundingly useful, but they do
not capture all of what software can do.  The (false) notion that
functions are <em>the</em> universal model of software is called
the <em>strong</em> Church-Turing thesis and is nicely critiqued by Dina
Goldin and Peter Wegner
(<a href="http://www.cse.uconn.edu/~dqg/papers/strong-cct.pdf">1</a>,
<a href="http://cs.brown.edu/people/pw/strong-cct.pdf">2</a>,
<a href="http://www.nachira.net/a/turing.pdf">3</a>).</p>

<p>The functional model leaves out any notion of interaction between
software and its environment as the software is running.  Some
applications, like compilers, come close to the functional ideal, but
all have some additional interactivity.</p>


Often programmers and computer scientists think of programs as just
big functions/procedures: There is a pile of input; the program runs; it
produces a pile of output.  However, all programs are also to some
degree reactive and/or interactive.  Some programs (e.g. compilers)
emphasize the function/procedure aspect and some programs
(e.g. operating systems/GUI applications) emphasize the
reactive/interactive aspect.  The Charcoal project was motivated by the
observation that existing approaches to interactive programming leave
something to be desired.  For the record, those approaches are:</p>

<p>(Vocabulary: In these documents we will informally refer to
interactive programs as being composed of multiple <em>tasks</em> that
can be in progress simultaneously.  Tasks may react to external events,
or may run on their own for a long time.  Tasks may share access to
resources and/or pass messages to each other.)</p>

<h1>1 Event Handlers</h1>

<p>In the event model, application code registers event handlers with an
event dispatcher.  Event handlers are composed of a procedure (function,
method, subroutine, whatever) plus some condition under which that
procedure should be run.  The event dispatcher monitors the environment
for events and runs handlers as appropriate.</p>

<h4>Strengths of Events</h4>

<p>The primary strength of the event model is that coordination between
tasks (i.e. event handlers) is simple.  Each handler runs to completion
before the next handler can run.  The programmer does not need to worry
about a handler being interrupted in the middle of some tricky
operation.  This makes the management of shared resources much easier
  than with other concurrency models.</p>

<p>The event model is very widely used.  All popular GUI frameworks use
the event model for defining the behavior of mouse clicks and key
presses.  Many networking frameworks use the event model for handling
incoming messages.  I believe that the popularity of the event model is
primarily a result of the (relative) ease with which shared resources
can be managemed.</p>

<p>Implementations of the event model tend to have very low overhead in
terms of memory wasted in the representation of task state and time
wasted in scheduling tasks.</p>

<p>Event dispatchers can be implemented in user code in any general
purpose programming language.  There are very few tricky implementation
issues to worry about.</p>

<h4>Weaknesses of Events</h4>

<p>The event model works best for simple tasks that do a small amount of
computational work in response to a single event.  A good example is the
handling of a key press in a text editor.  The event handler just has to
add the character to the document data structure and maybe indicate that
the screen should be redrawn.</p>

<p>Not all tasks are so simple, though.  If a task needs to make several
interactions with the environment in sequence or perform a long-running
computation, things get messy with the event model.  If a handler does
not return to the dispatcher quickly, the application will become
unresponsive, which is generally unacceptable.</p>

<p>Consequently, the application programmer must manually break complex
and long-running tasks up into a sequence of "smaller" event handlers.
This process makes the control flow of the application hard to follow.
Even worse, it forces the programmer to explicitly encode control flow
information as data that can be passed from one handler to the next.
This process of breaking up tasks into pieces is called control flow
inversion or stack ripping.  As the violent terminology suggests, stack
ripping makes code comprehension and maintenance much harder.</p>

<p>Event handlers cannot be run in parallel on multiple processors.  I
do not consider this to be a significant issue, because
reactive/interactive programming and parallel programming are two
entirely different topics.  I think it is unfortunate that they get
mushed together so often.</p>

<h3>Threads (Preemptive)</h3>

<p>Threads are tasks that share memory (i.e. exist within a single
process) and can be run simultaneously (i.e. in parallel).  If the
number of processors available is smaller than the number of threads
that want to run, the system is responsible for interleaving the
execution of the threads.  In order to accomplish this interleaving, the
system reserves the right to interrupt (or preempt) a thread at any
time.

<p>A couple of popular multithreading interfaces are
<a href="http://en.wikipedia.org/wiki/POSIX_Threads">Posix</a> and 
<a href="http://msdn.microsoft.com/en-us/library/windows/desktop/ms684847(v=vs.85).aspx">Windows
(win32)</a>.</p>

<h4>Strengths of Threads</h4>

<p>In contract to event handlers, threads that run for a long time or
wait for some input from the environment do not block the progress of
other threads.  This is huge, because it avoids the stack ripping
nonsense that is necessary in the event model.  Tasks can be written
with normal control flow (loops, calls, exceptions, etc).</p>

<p>As I have mentioned, I am not terribly concerned with processor
parallelism, but threads can be run in parallel.</p>

<h4>Weaknesses of Threads</h4>

<p>By far the most important weakness of threads is that they make it
very easy to introduce concurrency bugs like race conditions and
deadlocks.  Specifically the combination of by-default shared memory and
preemption makes it extremely hard to guarantee that threads do not step
on each others' toes through reads and writes to shared memory.</p>

<p>Most thread model implementations introduce a huge source of
nondeterminism in the form of the thread scheduler.  This tends to make
it extra hard to debug concurrency problems with threads.  This topic
has been the subject of intense research over the decades and I am not
aware of any really satisfactory solutions yet.</p>

<p>Threads tend to have relatively high memory overhead.  Most popular
threading implementations force application code to pre-allocate a large
amount of stack space for each thread.  Context switching between
threads typically requires some interaction with the operating system
which adds some time overhead.</p>

<h3>Cooperative Threads</h3>

<p>Cooperative threads are a kind of compromise between events and
(preemptive) threads.  Like threads, multiple cooperative threads can be
in progress simultaneously.  Like events, only one cooperative thread
can be running at any particular time.  Cooperative threads run until
they invoke a special <em>yield</em> primitive.  Within a collection of
cooperating threads, one cannot be interrupted to run another.</p>

<p>Cooperative threads (or slight variations on the theme) go by lots of
different names, including
<a href="http://en.wikipedia.org/wiki/Coroutine">coroutines</a> and
<a href="http://msdn.microsoft.com/en-us/library/windows/desktop/ms682661(v=vs.85).aspx">fibers</a>.</p>

<h4>Strengths of Cooperative Threads</h4>

<p>It is generally much easier to avoid concurrency bugs with
cooperative threads than preemptive threads because between yield
invocations, cooperative threads do not need to worry about other
threads interfering with their work.</p>

<p>Good implementations of cooperative threads solve the problem in the
event model of one task waiting for some external action and blocking
the progress of other tasks.  What I mean by good implementations is
that some care needs to be taken with system primitives like "read", so
that only the calling thread is blocked, not all threads.  This is
doable, but requires a bit of fancy engineering.</p>

<h4>Weaknesses of Cooperative Threads</h4>

<p>The most important weakness of cooperative threads has to do with
long-running tasks.  With cooperative threads it is easy for such a task
to starve other running tasks.  The main way to combat this problem is
to add more yield invocations to give other threads a chance to run.
However this leads to a subtle problem.  Yield is both a synchronization
primitive and a scheduling primitive and these two things are in
conflict.  Getting a "just right" balance of yield frequency can be
tricky, esecially in the context of long-term code mantenance.</p>

<p>Also cooperative threads cannot be run in parallel.  Like I said
before, don't care much about this one.  There are a couple of research
projects aimed at running cooperative threads in parallel.  Seems like a
sort of strange goal to me.</p>

<h3>Coroutines</h3>

<p>Some people use <em>coroutine</em> and <em>cooperative thread</em>
synonymously, but there is a distinct that is also called coroutine that
I think is more useful.  In implementation terms these "shallow"
coroutines only allow yield to be invoked from the "coroutine entry
function".  This means that we only need one normal stack.  Each
"coroutine frame" can be allocated separately on the heap.  We can be
sure that a (shallow) coroutine will not yield to another coroutine
while it is in the middle of some deeply nested call.</p>

<h4>Strengths of (Shallow) Coroutines</h4>

<p>Simpler</p>

<h4>Weaknesses of (Shallow) Coroutines</h4>

<h3>Generators</h3>

Generators are restricted (shallow) coroutines.  Coroutines can yield to
any other coroutine.  Generators can only yield back to whoever created
them.

<h4>Strengths of Generators</h4>

<p>Simpler</p>

<h4>Weaknesses of Generators</h4>

<h3>Processes</h3>

I'll throw processes in here, just because 

<h4>Strengths of Processes</h4>

Isolation.

<h4>Weaknesses of Processes</h4>

Harder to manage shared resources.

High overhead.

<h2>4.(2/2) New in Charcoal: Activities</h2>

<p>Charcoal introduces a new interactive programming primitive called
activities.  Like cooperative threads are a kind of compromise between
events and threads, activities are a kind of compromise between
preemptive and cooperative threads.  The goal is to avoid the awkward
problem of needing to put yields in just the right place, while not
reintroducing all the concurrency bug problems that come with true
preemptive threads.</p>


<h3>4.2.(1/3) Activities Compared to Threads</h3>

<p>Probably the most obvious difference between threads and activities
is one that I don't think is all that important in the end.  Threads can
be run in parallel; activities can't.  That seems like a hugely
consequential difference; how can I say it isn't important?  It's
because I don't think threads should be used to parallelize most
applications anyway, so it's not a uesful feature.  Parallelization
should be done with task-oriented frameworks like Cilk, Microsoft's PPL,
Intel's TBB, Apple's GCD, etc.</p>

<p>So if we set asside parallelization (for example, just pretend we're
running everything on a single-processor machine), what's the difference
between threads and activities?  Threads are fully preemptive, whereas
activities are only preemptive at a coarser granularity and it's easy to
temporarily "disable interrupts" in a safe way with activities.</p>

<p>Full preemption is the source of much evil for threads.  Optimizing
compilers and modern architectures like to rearrange memory accesses in
all kinds of funky ways, which is why data races are such a thorny
problem for high-level language memory models
(<a href="http://www.cs.umd.edu/~pugh/java/memoryModel/">Java</a>,
<a href="http://www.hpl.hp.com/personal/Hans_Boehm/c++mm/">C/C++</a>).
Between activities <em>there are no data races</em>.  The language
forbids the implementation from making it appear that memory accesses
have crossed yield boundaries.</p>

<p>It may seem like the prohibition against memory accesses crossing
yield boundaries would be a big performance problem, but I don't think
it will be.  (This is all very speculative at the moment.)  For example,
consider loop invariant code motion:</p>


<?php format_code(
'<i>void</i> <b>function_with_loop</b>( <i>int *</i><b>p</b>, <i>int *</i><b>q</b> )
{
    <i>size_t</i> <b>i</b>;
    <b><u>for</u></b>( i = 0; i &lt; ...; ++i )
    {
        ... *p ... *q = ...
    }
}' ); ?>

<p>If a compiler can satisfy itself
that <span class="mono">p</span>
and <span class="mono">q</span> aren't aliases for
anything else accessed by this code, it would love to transform it
into:</p>

<?php format_code(
'<i>void</i> <b>function_with_loop</b>( <i>int *</i><b>p</b>, <i>int *</i><b>q</b> )
{
    <i>size_t</i> <b>i</b>;
    <i>int</i> <b>pval</b> = *p, <b>qval</b> = *q;
    <b><u>for</u></b>( i = 0; i &lt; ...; ++i )
    {
        ... pval ... qval = ...
    }
    *q = qval
}' ); ?>

<p>But in Charcoal a for loop really looks like this:</p>

<?php format_code(
'<i>void</i> <b>function_with_loop</b>( <i>int *</i><b>p</b>, <i>int *</i><b>q</b> )
{
    <i>size_t</i> <b>i</b>;
    <i>int</i> <b>pval</b> = *p, <b>qval</b> = *q;
    <b><u>for_no_yield</u></b>( i = 0; i &lt; ...; ++i )
    {
        ... pval ... qval = ...
        yield;
    }
    *q = qval
}' ); ?>

<p>So it seems like the loop invariant code motion optimization might
violate the rules by making accesses
to <span class="mono">*p</span>
and <span class="mono">*q</span> appear to cross yield
boundaries.  My hope is that the compiler will be able to see into yield
and insert fix-up code like this:</p>

<?php format_code(
'<i>void</i> <b>function_with_loop</b>( <i>int *</i><b>p</b>, <i>int *</i><b>q</b> )
{
    <i>size_t</i> <b>i</b>;
    <i>int</i> <b>pval</b> = *p, <b>qval</b> = *q;
    <b><u>for_no_yield</u></b>( i = 0; i &lt; ...; ++i )
    {
        ... pval ... qval = ...
        if( "actually yield (very unlikely)" )
        {
            *q = qval;
            actually_yield;
            pval = *p;
            qval = *q;
        }
    }
    *q = qval;
}' ); ?>

<p>This does add complexity to the compiler and inflates code size
somewhat, but I don't think either of those costs should be huge.  It
should have very little impact on run time, because actually yielding
should happen so infrequently.</p>

<p>Interesting side note: Charcoal actually does have threads, though
most application code shouldn't use them most of the time.  In Charcoal
threads are containers for activities, similarly to how processes are
containers for threads in most modern systems.  Most activities should
be running in a single thread, but there are some reasons for using
multiple threads.  You need to be super careful.  Kind of like signal
handling in normal C/C++.</p>

<h3>4.2.(2/3) Activities Compared to Cooperative Threads</h3>

<p>In terms of implementation, activities are quite similar to
cooperative threads.</p>

<h3>4.2.(3/3) Activities Compared to Goroutines</h3>

<p>Goroutines are the primary concurrency primitive in the language Go,
designed and implemented by some folks at Google.  Out of all the dozens
of concurrency primitives that have been proposed and implemented over
the years, I decided to compare activities to goroutines first, because
they are both interesting hybrids of preemptive and cooperative threads
(and Go is getting a fair amount of attention these days).</p>

<p>Naturally, I think activities have some advantages compared to
goroutines.</p>

<p>Because goroutines can run in parallel, Go is (in principle) just as
exposed to data races as threads in C/C++.  I believe the main
counterpoint Go enthusiasts would make to this is that Go has a snazzy
message passing system and well-written Go programs <em>shouldn't</em>
use shared memory much/at all.  I do not believe there is anything in
the language that actually prevents memory from being shared between
goroutines (I'll have to revise this argument heavily if I'm wrong about
that).</p>

<p>I don't find this line of thinking compelling for two reasons:</p>

<ul>
<li>Some applications really need shared memory.  Of course shared
memory can be simulated by a "server goroutine" pattern, but that can
have non-trivial overhead and it obscures the logic of the application.
<li>In my experience the nastiest data race bugs are not on memory
locations that were designed to be shared, but rather little bits of
global or static memory in some odd corner of a library that don't get
accessed very often.  Expunging all such little bits of shared memory
from a large application is hard; nearly impossible if there is a lot of
old legacy code to contend with.
</ul>

<p>So in my view, shared memory is a fact of life that has to be dealt
with.  Activities have the following really nice properties:</p>

<ul>
<li>There are no data races, because memory reads and writes are not
allowed to "cross yield boudaries".  This really limits the damage that
some errant bit of shared memory can do.
<li>Making more complex sequences of shared memory reads and writes
atomic is easy.
</ul>

<h3>4.2.(3/3) Activities Compared to Concurrency Primitive X</h3>

Lots of variations on the "Big Four" (processes, events, threads,
cooperative threads) have been proposed over the years.  If you would
like to see my take on how activities compare to any of these, let me
know.  I'll do my best to write something and update here.


More Junk

<h3>For Language Designers/Enthusiasts</h3>

<h3>For Programmers</h3>

<h3>For Computer Architects</h3>

<p>Many researchers of various stripes, but especially architects,
assume that all programmers ever wanted from a concurrency framework was
sequential consistency
(<a href="https://www.google.com/search?q=End-to-end+sequential+consistency">e.g.</a>).
It is certainly the case that SC is more intuitive than the monstrous
relaxed memory models that have been invading programming language
definitions recently
(<a href="http://www.hpl.hp.com/personal/Hans_Boehm/c++mm/">e.g.</a>).
However, I think that SC is still too low-level to be useful for many
everyday programming situations.</p>

<p>SC says that individual memory operations can be arbitrarily
interleaved.  This is terrible.  Activities are more coarse-grained by
default and give the programmer simple tools to exert more control.</p>

<p>Note that I am not trying to take a shot at architecture researchers.
Rather, because of the nature of the problem they're interested in (how
to program shared-memory multi-processor architectures) they are
naturally drawn to (conventional/preemptive) threads, which combine
shared memory and parallel execution.  By giving up parallelism, we can
achieve a much more intuitive concurrency model.
(<a href="faq.html#parallel">Wait!  I care about parallelism!</a>)</p>

<h3>TBD</h3>

<ul>
  <li>Charcoal has a thread-like primitive called an <em>activity</em>.
  We believe that it will be easier to build robust concurrent
  applications with activities than existing alternatives.  Why a new
  concurrency primitive?  Let's look at what programmers use today.
  <ul>
    <li><em>Event handlers</em>
    (<a href="http://en.wikipedia.org/wiki/Event_loop">1</a>,
     <a href="http://msdn.microsoft.com/en-us/library/windows/desktop/dd377532(v=vs.85).aspx">2</a>,
     <a href="http://docs.wxwidgets.org/2.8/wx_eventhandlingoverview.html">3</a>, ) are very
    popular for user interface programming and are relatively easy to
    understand, debug, etc., because each handler runs from start to
    finish without interference from other handlers.  The big problem
    with event handlers is that complex and/or long-running tasks have
    to be manually broken up into lots of little separate handlers.
    This is a maintenance nightmare.  This problem is sometimes called
    "<a href="http://martinfowler.com/bliki/InversionOfControl.html">control flow inversion</a>" or "<a href="http://bryanpendleton.blogspot.com/2011/01/stack-ripping.html">stack ripping</a>".
    <li><em>(Preemptive) Threads</em>
    (<a href="https://computing.llnl.gov/tutorials/pthreads/">1</a>,
     <a href="http://msdn.microsoft.com/en-us/library/windows/desktop/ms684847(v=vs.85).aspx">2</a>,
     <a href="http://www.boost.org/doc/libs/1_53_0/doc/html/thread.html">3</a>,
     <a href="http://en.cppreference.com/w/cpp/thread">4</a>,
     <a href="http://www.javaworld.com/jw-04-1996/jw-04-threads.html">5</a>,
     <a href="http://cml.cs.uchicago.edu/pages/cml.html">6</a>)
    don't have the control flow inversion problem, but writing
    robust/reliable multithreaded code is <em>extremely</em> hard.  There
    is a whole cottage industry around trying to make it easier to
    implement reliable multithreaded programs.  The core of why
    multithreaded programming is hard is the combination of shared memory
    and preemption.  It's way too easy for one thread to interrupt another
    in the middle of a tricky sequence of operations and corrupt the state
    of the program.
    <li><em>Cooperative threads</em>
    (<a href="http://msdn.microsoft.com/en-us/library/windows/desktop/ms682661(v=vs.85).aspx">1</a>,
     <a href="http://dekorte.com/projects/opensource/libcoroutine/">2</a>,
     <a href="https://code.google.com/p/libtask/">3</a>,
     <a href="http://software.schmorp.de/pkg/libcoro.html">4</a>,
     <a href="http://lua-users.org/wiki/MultiTasking">5</a>,
     <a href="http://hackage.haskell.org/packages/archive/monad-coroutine/0.7.1/doc/html/Control-Monad-Coroutine.html">6</a>)
    are a kind of hybrid between event handlers and threads.  Like event
    handlers, only one cooperative thread can be running at a time.  The
    running cooperative thread must invoke a <em>yield</em> primitive to
    allow another to run.  This makes avoiding concurrency bugs easier.
    Like (conventional) threads, multiple cooperative threads can be
    in-progress simultaneously, avoiding the control flow inversion
    problem of event handlers.  The problem with cooperative threads is
    that getting the yield invocations in just the right places is a
    tricky problem that is left up to the programmer.  Too many yields
    tends to create concurrency bugs and too few leads to one
    cooperative thread hogging the system, potentially making the
    application unresponsive.
  </ul>
  <li>Charcoal introduces a new concurrency framework called
  <em>activities</em> that is a hybrid between cooperative and
  preemptive threads.  From an implementer's perspective the idea is to
  start with cooperative threads, then implicitly insert yield
  invocations relatively frequently.  The result is a kind of "chunky"
  preemption that we believe combines most of the software engineering
  benefits of the two multithreading models.  From a programmer's
  perspective activities behave mostly like preemptive threads, except
  that getting the synchronization "right" is easier.
  <li>Q: Why a new language?  Can't activities be implemented as a
  library?<br/>
  A: Implementing activities requires subtly redefining basic control
  flow mechanisms like loops and procedure calls.  This means activities
  cannot be implemented just as a library.  Since a new language is
  needed anyway, I took the opportunity to add some nice syntactic sugar
  for writing concurrent software.
  <li>Q: What about shared memory versus message passing?  Isn't that
  the most important thing in concurrent language design?<br/>
  A: The Charcoal authors view shared memory and message passing as
  complementary technologies, both of which match some application
  patterns naturally.  Charcoal has good support for both.
</ul>

<p>Note: For the time being, the designers of Charcoal are not
interested in processor parallelism at all.  Concurrency and parallelism
are different, and Charcoal is about concurrency.</p>

<?php include 'copyright.html'; ?>

</div>
</body>
</html>
