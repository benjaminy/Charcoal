<html>
<head>
<title>Why Charcoal?</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<?php include 'side_bar_basic.html'; ?>

<div class="main_div">

<header>
<h1>Why Charcoal?</h1>
<h3>... in 100 lines or less ... or more</h3>
</header>

<div>
[tl;dr] The main frameworks for writing interactive software (threads,
event loops) have annoying problems.  There are more exotic alternatives
(cooperative threads, coroutines, generators), but they have more
obscure problems.  This project is an exploration of a new idea called
pseudo-preemptive threads, a.k.a. <em>activities</em>.  Under the hood
activities look just like cooperative threads, but the compiler throws
in lots of yields
</div>

<p>Once upon a time, all software was functions.  By <em>once upon a
time</em> I mean around the dawn of modern computing in the 30s, 40s and
50s.  By <em>functions</em> I mean that the interface between a program
and its environment was very much like a mathematical function:
<ol>
<li>the environment provides an input;</li>
<li>the program runs;</li>
<li>if it finishes it provides an output to the environment.</li>
</ol>
This pattern is also called <em>batch</em> or <em>procedural</em>
software.  As early as the 1960s, interactive software was being
developed (for
example, <a href="http://en.wikipedia.org/wiki/Sketchpad">Ivan
Sutherland's Sketchpad</a>).  Interactive software has a dramatically
different interface with its environment.  Interactive software can:
<ul>
<li>have multiple logical tasks in progress simultaneously,</li>
<li>produce output and read input while it's running,</li>
<li>be interrupted by events in the environment.</li>
</ul>
</p>

<p>Interactive software is generally considered harder to deal with than
procedural software, so software engineers have gone to great lengths to
make large-scale software "mostly procedural", with interactivity
carefully sequestered in as small a corner of the code as possible.
Much to the chagrin of the programmers who prefer thinking procedurally,
the relative importance of interactivity in mainstream software has
increased in recent years.  This increase can primarily be chalked up to
three trends:
<ul>
<li>Previously most applications were mostly stand-alone, but now most
applications communicate with multiple hosts over the network as part of
their normal operation.</li>
<li>The richness of user interfaces has increased, especially (but not
exclusively) on mobile platforms.</li>
<li>Tiny computers have gotten cheap enough that the world of physically
interactive embedded software has opened up to a much wider
audience.</li>
</ul>
</p>

<p>The two dominant abstractions for interactive software are event
handlers and threads.  Both have serious drawbacks (the linked documents
have good summaries of events & threads, if you're not already
familiar):
<ul>
<li><a href="http://www.stanford.edu/~ouster/cgi-bin/papers/threads.pdf">Why
Threads Are A Bad Idea (for most purposes)</a></li>
<li><a href="http://www.stanford.edu/class/cs240/readings/vonbehren.pdf">Why
Events Are A Bad Idea (for high-concurrency servers)</a></li>
</ul>
My one sentence summary of these arguments: Event handlers are great for
simple interaction patterns, but scale very badly as program logic
becomes more complex (<a href="http://callbackhell.com/">"callback
hell"</a>).  Threads allow complex interaction patterns to be programmed
in a natural style, but make it extremely hard to avoid nasty
concurrency bugs (data races, deadlocks, and atomicity violations, oh
my).</p>

<p>Most modern software uses both event handlers and threads.  Popular
graphical user interface frameworks use event handlers almost
exclusively for things like mouse clicks and keyboard presses.  Yet
almost all modern desktop and mobile software has anywhere from a few to
several dozen threads at any given time.  These threads exist primarily
to handle long-running and complex asynchronous tasks like database
transactions and communicating with servers over the internet.</p>

<p><strong>An aside about parallelism</strong>.  One of the major
differences between event handlers and threads is that threads can run
in parallel on separate processors, whereas event handlers cannot.
While this is a significant difference, it is mostly irrelevant <em>from
the perspective of interactivity</em>.  Interactivity and parallelism
are completely different topics, so the fact that threads can be run in
parallel is not an advantage when we are talking about how to write
interactive software.</p>

<p><strong>The main idea(!)</strong>: If event handlers and threads both
have such serious problems, why are they used so heavily in modern
software?  Quite simply, it's because nothing better exists yet.  This
project is an attempt to create a new primitive (called an activity)
that combines the advantages and mitigates the disadvantages of event
handlers and threads for programming interactive software.</p>

<p>On the way to explaining what activities are, we are going to take a
brief detour through cooperative threads.  Cooperative threads are like
conventional (a.k.a. preemptive) threads, except only one is allowed to
run at a time and the active cooperative thread must explicitly invoke
a <em>yield</em> primitive to give the system the option of switching to
a peer cooperative thread.  The nicest feature of cooperative threads is
that between yield invocations threads cannot interrupt each other,
which makes avoiding concurrency bugs easier.</p>

<p>Cooperative threads are a kind of compromise between event handlers
and threads, and they do combine some of the advantages of both.
Unfortunately, cooperative threads come with their own nasty drawback:
the programmer is responsible for getting yield invocations in just the
right places.  If a cooperative thread doesn't yield frequently enough
it can starve other threads of processor time and make the whole
application unresponsive.  If the programmer throws in yields blithely
in an attempt to avoid starvation they can violate atomicity
assumptions, creating potentially catastrophic concurrency bugs.</p>

<p>The happy-medium yield frequency is especially hard to maintain when
a program is composed of multiple libraries and frameworks (and most
modern applications are).  What assumptions did the authors of the
various components make about whether code in other components will
yield or not?  Answering this question is important and extremely hard
in general.  This problem has limited the adoption of cooperative
threading to the margins of the software industry.  (There has been a
recent surge of enthusiasm for cooperative threads in various "scripting"
language
communities: <a href="http://code.activestate.com/recipes/466008-simple-cooperative-multitasking-using-generators/">Python</a>,
<a href="http://lua-users.org/wiki/MultiTasking">Lua</a>,
<a href="http://ruby-doc.org/core-2.0/Fiber.html">Ruby</a>, etc.)</p>

<p>Activities are a compromise between cooperative and preemptive
threads, that I believe neatly captures the advantages of both.  From a
runtime system perspective, activities are exactly cooperative threads.
However, languages that support activities (like Charcoal) are defined
to implicitly insert yield invocations very frequently.  This means that
from the perspective of high-level language semantics, activities behave
more like preemptive threads.  The system still has to wait for a yield
to switch between activities, but in non-buggy Charcoal code it should
never have to wait very long.  Because of this I sometimes call
activities pseudo-preemptive threads.</p>

<p>These frequent yields could easily become a substantial drag on
performance, so one of the most important implementation challenges in
Charcoal is keeping the yield overhead extremely low and offering a
convenient way for the programmer to suppress yield insertion in tight
performance-critical loops.</p>

<p>Relative to conventional threads, the advantages of activities are:
<ul>
<li>Data races between activities do not exist.</li>
<li>Avoiding other kinds of concurrency bugs is easier.  The programmer
can simply suppress yielding for certain sections of code to enforce
atomicity.</li>
</ul>
</p>

<p>Relative to cooperative threads, the advantages of activities are:
<ul>
<li>Frequent implicit yielding mean that starvation is relatively easy
to avoid (though still possible if the programmer suppresses yields for
too much time).</li>
<li>Composing libraries is just as easy as with sequential
programs.</li>
</ul>
</p>

<p>If this sounds intriguing please explore the rest of this site, which
includes several examples, ruminations on concurrency in general,
detailed discussions of existing approaches to concurrent programming,
and information about implementing activities (and some day an actual
working implementation).</p>

<?php include 'copyright.html'; ?>

</div>
</body>
</html>
