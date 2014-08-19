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

<div class="hblock2">
[tl;dr] 
<p>Existing frameworks for writing interactive software
(threads, event loops, coroutines) have annoying problems that make it
too easy to write incomprehensible, unreliable and/or inefficient code.
This project is an exploration of a new idea called pseudo-preemptive
threads (or <em>activities</em>, for those who prefer fewer
syllables).</p>

<p>Under the hood activities look just like cooperative threads, but the
language definition includes frequent implicit yields, which give
activities more of a preemptive thread feel.  A simple synchronization
primitive (<em>unyielding</em>) helps multi-activity software avoid
atomicity violations, the bane of multithreaded software.</p>
</div>

<div class="hblock2">
[slightly longer tl;dr]
<p>
Color code:
</p>

<table border="1">
<tr>
<td class="color_good">Good</td>
</tr>
<tr>
<td class="color_pretty_good">Pretty good</td>
</tr>
<tr>
<td class="color_okay">Okay</td>
</tr>
<tr>
<td class="color_pretty_bad">Pretty bad</td>
</tr>
<tr>
<td class="color_bad">Bad</td>
</tr>
</table>

<table>
<tr>
<td width="15%"></td>
<td width="17%">Threads</td>
<td width="17%">Activities</td>
<td width="17%">Coop Threads</td>
<td width="17%">Coroutines</td>
<td width="17%">Events</td>
</tr>
<tr>
<td>Atomicity</td>
<td class="color_bad">Data races :(</td>
<td class="color_good"><span class="mono">unyielding</span>:
Like <span class="mono">synchronized</span> in Java, but way
better</td>
<td class="color_okay">Will that procedure Joe wrote
invoke <span class="mono">yield</span>?  Who knows!  Wheee</td>
<td class="color_pretty_good">Coroutines</td>
<td class="color_pretty_good">Events</td>
</tr>
<tr>
<tr>
<td>Starvation</td>
<td class="color_good">The only problem is all the synchronization you
need to avoid atomicity violations</td>
<td class="color_pretty_good">Just don't wrap a long-running block
in <span class="mono">unyielding</span> and you're golden</td>
<td class="color_bad">There's almost certainly some long-running loop
hiding somewhere that you forgot to put
a <span class="mono">yield</span> in</td>
<td class="color_pretty_bad">Coroutines</td>
<td class="color_pretty_bad">Events</td>
</tr>
<tr>
<td>Memory scalability</td>
<td class="color_bad">All those nasty stacks</td>
<td class="color_good">Heap-allocated call frames mean zero
per-activity stack overhead</td>
<td class="color_pretty_bad">Most mainstream implementations are just
like "normal" threads</td>
<td class="color_good">Only need special allocation for coroutines (aka
"async" procedures)</td>
<td class="color_good">So pleasantly simple</td>
</tr>
<tr>
<td>Overhead (spawn, context switch, etc.)</td>
<td class="color_bad">All those nasty stacks</td>
<td class="color_pretty_good">Just don't wrap a long-running block
in <span class="mono">unyielding</span> and you're golden</td>
<td class="color_okay">Will that library function
invoke <span class="mono">yield</span>?  Who knows!  Wheee</td>
<td class="color_good">Coroutines</td>
<td class="color_good">Events</td>
</tr>
<tr>
<td>Modularity</td>
<td class="color_bad">All those nasty stacks</td>
<td class="color_good">Just don't wrap a long-running block
in <span class="mono">unyielding</span> and you're golden</td>
<td class="color_bad">Will that library function
invoke <span class="mono">yield</span>?  Who knows!  Wheee</td>
<td class="color_pretty_bad">Coroutines</td>
<td class="color_pretty_bad">Events</td>
</tr>
<tr>
<td>Parallelism</td>
<td class="color_good">Finally, threads win something</td>
<td class="color_pretty_bad">See coop threads</td>
<td class="color_pretty_bad">"<a href="http://www.google.com/search?q=observationally+cooperative+multithreading">Observationally cooperative multithreading</a>".  Color me skeptical</td>
<td class="color_bad">See events</td>
<td class="color_bad">No one's crazy enough to even try</td>
</tr>


</table>
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

<p>Interactive software is generally considered harder to write and
maintain than procedural software, so software engineers have gone to
great lengths to make large-scale software "mostly procedural", with
interactivity carefully sequestered in as small a corner of the code as
possible.  Much to the chagrin of the programmers who prefer thinking
procedurally, the relative importance of interactivity in mainstream
software has increased in recent years.  This increase can primarily be
chalked up to three trends:
<ul>
<li>Network communication has become an integral part of most
applications.</li>
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

<p><strong>An aside about parallelism</strong>.  A major difference
between event handlers and threads is that threads can run in parallel
on separate processors, whereas event handlers cannot.  While this is
difference is significant in general, <em>from the perspective of
interactivity</em> it is mostly irrelevant.  Interactivity and
parallelism are completely different topics, so the fact that threads
can be run in parallel is beside the point when we are talking about how
to write interactive software.</p>

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
applications are responsible for getting yield invocations in just the
right places.  If a cooperative thread doesn't yield frequently enough
it can starve other threads of processor time and make the whole
application unresponsive.  If a programmer throws in yields blithely in
an attempt to avoid starvation they can violate atomicity assumptions,
creating potentially catastrophic concurrency bugs.</p>

<p>The
"<a href="http://en.wikipedia.org/wiki/Goldilocks_principle">Goldilocks</a>"
yield frequency is especially hard to maintain when a program is
composed of multiple libraries and frameworks (as most modern
applications are).  What assumptions did the authors of the various
components make about whether code in other components will yield or
not?  Answering this question is important and extremely hard in
general.  This problem has limited the adoption of cooperative threading
to the margins of the software industry.  However, in the name of making
interactive programming easier (especially for applications that run
across the internet), cooperative threads (and their cousins,
coroutines) have started showing up in some mainstream software
(<a href="http://code.activestate.com/recipes/466008-simple-cooperative-multitasking-using-generators/">Python</a>,
<a href="http://lua-users.org/wiki/MultiTasking">Lua</a>,
<a href="http://ruby-doc.org/core-2.0/Fiber.html">Ruby</a>, <a href="http://msdn.microsoft.com/en-us/library/hh191443.aspx">.NET</a>, etc.)</p>

<p>Activities are a hybrid of cooperative and preemptive threads, that I
believe neatly captures the advantages of both.  From a runtime system
perspective, activities are exactly cooperative threads.  However,
languages that support activities (like Charcoal) are defined to
implicitly insert yield invocations very frequently (as a first
approximation, think every iteration of every loop).  This means that
from the perspective of high-level application design, activities behave
more like preemptive threads.  The system still has to wait for a yield
to switch between activities, but in non-buggy Charcoal code it should
never have to wait very long.  Because of this I sometimes call
activities pseudo-preemptive threads.</p>

<p>These frequent yields could easily reintroduce the concurrency bugs
that bedevil multithreaded software.  The primary tool to prevent this
is the ability to declare any procedure or block of code unyielding.
Within the dynamic scope of an unyielding block the current activity
cannot be interrupted.</p>

<p>Frequent yields could also become a substantial drag on performance,
so one of the most important implementation challenges in Charcoal is
keeping the yield overhead extremely low and offering a convenient way
for the programmer to suppress yield insertion in tight
performance-critical loops.</p>

<p><strong>Summary</strong></p>

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

<p>I believe activities inhabit an interesting new part of the universe
of interactive software primitives.  In particular I aim to show that
multi-activity software can be:</p>

<ul>
<li><em>Simple</em>.  Like multithreaded software, interactivity can be
written in a natural, procedural style.  (i.e. no callback hell.)</li>
<li><em>Reliable</em>.  The careful balancing of frequent implicit
yields and simple synchronization make it easy (or at least easier) to
avoid the most problematic concurrency bugs.</li>
<li><em>Efficient</em>.  Activities have been very carefully designed to
allow implementations that offer lightning-fast interactivity primitives
(activity creation, context switching, synchronization, &hellip;) while
avoiding slowing down sequential code or introducing memory
overheads.</li>
</ul>

<p>If this sounds intriguing please explore the rest of this site, which
includes several examples, ruminations on concurrency in general,
detailed discussions of existing approaches to concurrent programming,
and information about implementing activities (and some day an actual
working implementation).</p>

<p><strong>An aside about shared memory versus message passing</strong>.
Some people (e.g. the designers of Go) believe that writing concurrent
software (including interactive software) can be made doable by a strong
focus on memory isolation between threads/&#8203;processes and rich
message passing primitives.  While minimizing shared memory is an
excellent rule of thumb, there are applications for which shared memory
is quite clear, safe and efficient.  In other words, I do not believe
that banning (or <em>very strongly</em> discouraging) the use of shared
memory is <em>the</em> solution to the interactive software challenge.
Charcoal and its core library have rich support for both shared memory
and message passing (a la CML), because I think they are both
useful.</p>

<?php include 'copyright.html'; ?>

</div>
</body>
</html>
