<html>
<head>
<title>Conventional Concurrency Primitives/Frameworks</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<?php include 'code_examples.php'; ?>

<?php include 'side_bar_threads_etc.html'; ?>

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

<p>The function model leaves out any notion of interaction between
software and its environment as the software is running.  Some
applications, like compilers, come close to the pure function ideal, but
all applications have some additional interactivity.  Some application
domains, like games, network services and mobile robotics tend to have a
much heavier focus on interactivity.</p>

<p>The Charcoal project was motivated by the sense that existing
models/&#8203;frameworks leave something to be desired for specifying
interactive software behavior.  On this page we recap and critique the
most commonly used tools for interactivity.</p>

<p>(Vocabulary note: In these documents we informally refer to
interactive programs as being composed of multiple <em>tasks</em> that
can be in progress simultaneously.  Tasks may react to external events,
or may run on their own for a long time.  Tasks may share access to
resources like memory and/or pass messages to each other.)</p>

<div class="hblock2">
<h2>Event Handlers</h2>

<p>In the event model, the application starts a task called the event
dispatcher and registers event handlers with it.  Event handlers are
composed of a procedure (function, subroutine, method, whatever) plus
some condition under which that procedure should be run.  The event
dispatcher monitors the environment for events and runs handlers as
appropriate.</p>

<p>(Vocabulary note: Event handlers are a kind of <em>callback</em>; so
called, because the dispatcher "calls back" to the handler
procedure.)</p>

<div class="hblock3">
<h3>Events: Strengths</h3>

<p>The primary strength of the event model is that coordination between
tasks (i.e. event handlers) is simple.  Each handler runs to completion
before the next handler can run.  The programmer does not need to worry
about a handler being interrupted in the middle of some tricky sequence
of operations.  This makes the management of shared resources much
easier than with other concurrency models.</p>

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
purpose programming language.  There are relatively few tricky
implementation issues to worry about.</p>
</div>
<br/>
<div class="hblock3">
<h3>Events: Weaknesses</h3>

<p>The event model works best for simple tasks that do a small amount of
computational work in response to a single event.  A good example is the
handling of a key press in a text editor.  The event handler just has to
add the character to the document data structure and maybe request a
screen redraw.</p>

<p>Not all tasks are so simple, though.  If a task needs to make several
interactions with the environment in sequence or perform a long-running
computation, things get messy with the event model.  The
dispatcher <em>must</em> wait for each handler invocation to finish, so
if a handler does not return to the dispatcher quickly, the application
will become unresponsive, which is generally unacceptable.</p>

<p>Complex and long-running tasks must be broken into a sequence of
"smaller" event handlers.  This is generally a manual process that
application developers are responsible for.  This process makes the
control flow of the application hard to follow.  Even worse, it forces
the programmer to explicitly encode control flow information as data
that is saved by one handler and read by the next.  This process of
breaking up tasks into pieces is called control flow inversion or stack
ripping.  As the violent terminology suggests, stack ripping makes code
comprehension and maintenance much harder.</p>

<p>Event handlers cannot be run in parallel on multiple processors.  I
do not consider this to be a significant issue, because
reactive/interactive programming and parallel programming are two
entirely different topics.  I think it is unfortunate that they get
mushed together so often.</p>
</div>
<br/>
<div class="hblock3">
<h3>Events: Summary</h3>

<p>For applications that don't have much interaction complexity, the
simplicity of the event handler model is hard to beat.  As applications
get more complex (as many applications are wont to do), event
handler-based implementations tend to scale poorly in terms of code
complexity and maintainability.  Here is one more or less randomly
chosen site devoted to this phenomenon, sometimes
called <a href="http://callbackhell.com/">callback hell</a>.</p>
</div>
</div>
<div class="hblock2">
<h2>Threads (Preemptive)</h2>

<p>Threads are tasks that can physically execute simultaneously.  When
the number of threads running is greater than the number of physical
processors, the system is responsible for rapidly switching back and
forth between different threads to give them all a chance to run.  In
this mode, the system can interrupt a running thread at absolutely any
time.  This interruption is often called preemption.</p>

<p>Multiple threads can share a single virtual memory space (i.e. exist
within a single process).  The interaction between threads that are
simultaneously reading and writing overlapping sets of memory addresses
is one of the trickiest issues with threads.</p>

<p>A couple of popular multithreading interfaces are
<a href="http://en.wikipedia.org/wiki/POSIX_Threads">POSIX</a> and 
<a href="http://msdn.microsoft.com/en-us/library/windows/desktop/ms684847(v=vs.85).aspx">Windows
(win32)</a> (<a href="http://software.intel.com/en-us/blogs/2006/10/19/why-windows-threads-are-better-than-posix-threads">fight</a>).</p>

<div class="hblock3">
<h3>Threads: Strengths</h3>

<p>In contract to event handlers, threads that run for a long time or
wait for some input from the environment do not block the progress of
other threads.  This is huge, because it avoids the stack ripping
nonsense that is necessary in the event model.  Tasks implemented as
threads can be written with normal control flow (loops, calls,
exceptions, etc).</p>

<p>As I have mentioned, I am not terribly concerned with processor
parallelism, but threads can be run in parallel.</p>
</div>
<br/>
<div class="hblock3">
<h3>Threads: Weaknesses</h3>

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
</div>
<br/>
<div class="hblock3">
<h3>Threads: Summary</h3>

<p></p>

</div>
</div>
<div class="hblock2">
<h2>Cooperative Threads</h2>

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

<div class="hblock3">
<h3>Cooperative Threads: Strengths</h3>

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
</div>
<br/>
<div class="hblock3">
<h3>Cooperative Threads: Weaknesses</h3>

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
</div>
<br/>
<div class="hblock3">
<h3>Cooperative Threads: Summary</h3>

<p></p>
</div>
</div>
<div class="hblock2">
<h2>Coroutines</h2>

<p>Some people use <em>coroutine</em> and <em>cooperative thread</em>
synonymously, but there is a distinct concept that is also called
coroutine that I think is more useful.  In implementation terms these
"shallow" coroutines only allow yield to be invoked from the "coroutine
entry function".  This means that we only need one normal stack.  Each
"coroutine frame" can be allocated separately on the heap.  We can be
sure that a (shallow) coroutine will not yield to another coroutine
while it is in the middle of some deeply nested call.</p>

<div class="hblock3">
<h3>(Shallow) Coroutines: Strengths</h3>

<p>Simpler</p>
</div>
<br/>
<div class="hblock3">
<h3>(Shallow) Coroutines: Weaknesses</h3>

<p>They're a bit more like event handlers in the sense that it's harder
to avoid starvation.</p>

</div>
</div>
<div class="hblock2">
<h2>Generators</h2>

<p>Generators are restricted (shallow) coroutines.  Coroutines can yield
to any other coroutine.  Generators can only yield back to whoever
created them.</p>

<div class="hblock3">
<h3>Generators: Strengths</h3>

<p>Simpler</p>
</div>
<br/>
<div class="hblock3">
<h3>Generators: Weaknesses</h3>

<p>Compared to coroutines, generators elegantly support yet fewer
application patterns.</p>

</div>
</div>
<div class="hblock2">
<h2>Processes</h2>

<p>I'll throw processes in here, just because.</p>

<div class="hblock3">
<h3>Processes: Strengths</h3>

<p>Isolation.</p>
</div>
<br/>
<div class="hblock3">
<h3>Processes: Weaknesses</h3>

<p>Harder to manage shared resources.</p>

<p>High overhead.</p>
</div>
</div>
<div class="hblock2">
<h2>Activities</h2>

<a href="activities_compared.html">See here</a>.
</div>

<?php include 'copyright.html'; ?>

</div>
</body>
</html>
