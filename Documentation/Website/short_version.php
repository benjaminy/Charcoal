<html>
<head>
<title>Why Charcoal?</title>
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
- <a href="faq.html">FAQ</a>
</div>

<div class="main_div">

<header>
<h1>Why Charcoal?</h1>
<h3>... in 100 lines or less ... or more</h3>
</header>

<p>Take a look at the processes running on your computer.  Many widely
used modern applications have dozens of threads active at any given
time.  Why so many threads?  In a few cases, it may be that the
application is actually taking advantage of the processor parallelism
available in most systems today.  However, probably most of those
threads spend the vast majority of their lives basically doing nothing,
waiting for some asynchronous event to occur.</p>

<p>These asynchronous programming patterns are clearly a common use case
for threads, but I claim that it is a far from perfect fit.  Threads are
highly bug-prone, and the fact that threads <em>can</em> run in
parallel is a significant contributor to their buggy tendencies.  As
circumstantial evidence that many developers recognize the dangers of
threads, I point to the continued popularity of the event loop/handler
pattern for asynchronous programming.</p>

<p>Code written with event handlers tends to have more complex control
flow than equivalent multithreaded code, because potentially
long-running tasks need to be manually broken up into multiple handler
invocations (and all intermediate state must be explicitly encoded).
Yet event handling remains extremely popular, especially for user
interface programming.  A major factor behind this enduring popularity
is that developers are <em>justifiably</em> afraid of the
nondeterministic and potentially catastrophic bugs that crop up so
easily with multithreading.</p>

<p>So applications these days use both threads and event handlers for
asynchronous programming, depending on the situation and the whims of
the developers.  I claim that both frameworks have serious flaws.  The
Charcoal project is an attempt to find a better way to do this kind of
asynchronous/concurrent programming.  In particular, Charcoal introduces
a new variant of cooperative threads that we
call <em>activities</em>.</p>

<p>The basic idea behind cooperative threads is that only one thread can
run at a time and the active thread must explicitly invoke
a <em>yield</em> primitive to allow control to transfer to another
thread.  Cooperative threads are more resistant to some important kinds
of concurrency bugs than conventional (preemptive/parallel) threads,
because it is impossible for cooperative threads to interfere with each
other between yield invocations.</p>

<p>Cooperative threads have existed on the fringes of software
engineering practice for decades.  In the last few years they seem to
have gained some popularity, especially in "scripting" language
communities
(<a href="http://code.activestate.com/recipes/466008-simple-cooperative-multitasking-using-generators/">Python</a>,
<a href="http://lua-users.org/wiki/MultiTasking">Lua</a>,
<a href="http://ruby-doc.org/core-2.0/Fiber.html">Ruby</a>, etc).
However, I believe that conventional cooperative threads have some
software engineering problems of their own.</p>

<p>Activities are an attempt to find a sweet spot between cooperative
and preemptive threads.  They are implemented much like cooperative
threads, but feel more like preemptive threads, because the compiler
implicitly sprinkles yield invocations around here and there.  If this
sounds intriguing please explore the rest of this site, which includes
several examples, ruminations on concurrency in general, detailed
discussions of existing approaches to concurrent programming, and
information about implementing activities (and some day an actual
working implementation).</p>

<?php include 'copyright.html'; ?>

</div>
</body>
</html>
