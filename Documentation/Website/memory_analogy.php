<html>
<head>
<title>Memory/Concurrency Analogy</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<?php include "google_analytics_script.html"; ?>

<?php include 'side_bar_basic.html'; ?>

<div class="main_div">

<header>
<h1>Memory/Concurrency Analogy</h1>
</header>

<div class="hblock2">

<p>
This page will make a lot more sense if you are familiar with Dan
Grossman's
<a href="http://www.google.com/search?q=The+Transactional+Memory+Garbage+Collection+Analogy">
memory management/concurrency analogy</a>.
</p>

[tl;dr]:

<p>
Concurrency in software should really be treated as two largely
unrelated ideas: parallelism and interactivity.  Interactivity should be
handled with frameworks that prohibit parallel execution.  Parallelism
should be handled with frameworks that severely limit interaction.  If
these frameworks are designed well it should be possible to use them to
construct software that is both parallel and interactive (if that's what
you're into).  This page is an attempt to extend Dan's memory/concurrency
analogy to support this vision of concurrent software.
</p>

</div>


<div class="hblock2" style="background-color:rgb(255,225,235)">

<p>
Memory in software is about having a place to save data.  When we unpack
memory we find two distinct ideas: physical memory and logical memory.
</p>

<p>
Physical memory is a finite resource that in general must
be <em>reused</em> in time to store different values.
</p>

<p>
Logical memory is about <em>naming</em> (or <em>references</em>
or <em>pointers</em>).  References are important when we want a small
easy-to-move and copy handle on something that we can't or don't want to
move and copy.  References are useful both when the referent is an
internal memory location and when it is an external resource of some
kind (file, website, &hellip;).
</p>

</div>

<div class="hblock2" style="background-color:rgb(225,235,255)">

<p>
Concurrency in software is about multiple tasks or actions happening at
the same time.  When we unpack concurrency we find two distinct ideas:
physical simultaneity and logical simultaneity.
</p>

<p>
Physical simultaneity (<em>parallelism</em>) is useful for running
software on multiple processors.  Using multiple processors is useful
for speeding up applications.  (And other things like redundancy that
we're going to ignore.)
</p>

<p>
Logical simultaneity is order flexibility.  Action A might happen before
or after action B.  Order flexibility is important when there is
<em>interaction</em> (or <em>communication</em>) between agents.  One
agent can't necessarily know when it will observe a given action from
another agent, so it has to be able to react to multiple possibilities.
Those agents might be components of a single application or they might
be the application and the external world.
</p>

</div>

<div class="hblock2" style="background-color:rgb(255,225,235)">

<p>
The core memory management problem from which all evil flows is the
dangling pointer dereference:
<ol>
<li>Value X is stored in memory location L</li>
<li>A reference to L is saved in some other place</li>
<li>The program erroneously believes it is done with value X, so reuses
L to store some other value Y</li>
<li>The program uses its stored reference to look in location L,
expecting to find X but gets some Y nonsense instead</li>
</ol>
</p>

<p>
This problem depends on both <em>reuse</em> and <em>references</em>.  If
we ban reuse, step 3 is impossible.  If we ban references, step 2 is
impossible.
</p>

<p>
In practice, banning either reuse or references is sufficiently painful
that very little software does either.  For a long time manual memory
techniques were widely used.  Unfortunately, programmer mistakes cause
problems like dangling pointer dereferences and memory leaks, which
negatively impact the reliability, security and performance of software.
</p>

<p>
Luckily we have a fantastic technology called garbage collection.
Garbage collection makes it appear that there is no reuse by keeping
track of all references and only reusing a location when no references
to it exist anymore.
</p>

</div>

<div class="hblock2" style="background-color:rgb(225,235,255)">

<p>
The core concurrency problem from which all evil flows is the atomicity
violation:
<ol>
<li>Task 1 checks if the balance is high enough to withdraw money from a
bank account</li>
<li>Task 2 checks if the balance is high enough to withdraw money from a
bank account</li>
<li>Task 1 performs the withdrawal (i.e. reduces the balance of the
account)</li>
<li>Task 2 performs the withdrawal, but there is no longer a large
enough balance; chaos ensues</li>
</ol>
</p>

<p>
This problem depends on both <em>parallelism</em>
and <em>interaction</em>.  If we ban parallelism it is easy for each
task to perform an arbitrary collection of actions without allowing
another task to sneak in and screw things up.  If we ban interaction the
tasks would not be able to access the same account (observing and
modifying a shared resource is a form of communication/interaction).
</p>

<p>
In practice, banning either parallelism or interaction is painful
(though I argue below it is exactly what we should do).  Programmers
soldier on with manual concurrency management techniques that are
extremely hard to use correctly.  Most concurrent software is full of
problems like atomicity violations and deadlocks that negatively impact
the reliability, security and performance.
</p>

<p>
The analog of garbage collection is transactional memory, which makes it
appear that there is no parallelism by keeping track of potentially
conflicting actions from different tasks and only allowing particular
collections of actions to take effect when they do not actually conflict
with the actions of other tasks.  Unfortunately, TM has not been widely
adopted for technical reasons that are beyond the scope of the current
discussion.  It is far too early to declare TM dead, but the first
significant wave of efforts to build TM systems has crashed with little
impact on mainstream software development.  I think it's worth exploring
more extreme measures.
</p>

</div>

<div class="hblock2" style="background-color:rgb(255,225,235)">

<p>
The "bans" on reuse and references discussed earlier need not be
universal.  In fact it is common for software to have some memory
locations that are effectively never reused (e.g. global variables).
References to such locations can be used freely without fear of
low-level memory management bugs.
</p>

<p>
Complementarily, if memory is only ever used directly (i.e. references
are never taken and passed around), it is easy to determine when it is
safe to reuse a location.  In practice, absolutely never using
references is extremely burdensome, so a great many informal rules have
been developed regarding limiting the use of references to make memory
bugs less likely.  Some of these rules can be and have been formalized
and automatically enforced in various ways.
</p>

<p>
One technique for avoiding references is making copies of data where one
might otherwise pass around references.  This is obviously not always
practical: what if the referred-to data is enormous?  Even worse,
copying breaks programs that intend for modifications made through one
reference to be observed through another reference.  Nevertheless,
copying is sometimes a good choice.
</p>

</div>

<div class="hblock2" style="background-color:rgb(225,235,255)">

<p>
The "bans" on parallelism and interaction discussed earlier need not be
universal.  In fact serial interaction frameworks like event loops and
coroutines are quite common.  Such frameworks can be used without fear
of certain low-level concurrency bugs.
</p>

<p>
Complementarily, if tasks running in parallel never interact there is no
danger of concurrency bugs.  In practice, absolutely never interacting
is extremely burdensome, so a great many informal rules have been
developed regarding limiting interaction to make bugs less likely.  Some
of these rules can be and have been formalized and automatically
enforced in various ways.
</p>

<p>
One technique for avoiding interaction is making copies of the resource
that parallel tasks need to use.  This is obviously not always
practical: what if the resource is enormous?  Even worse, copying breaks
programs that intend for actions performed by one task to be observed by
the other.  Nevertheless, copying is sometimes a good choice.
</p>

</div>

<p>
I think an interesting way forward for concurrent software is to admit
that it's simply too hard (at least for now) to make a single framework
for both parallel and interactive software that can be used effectively
by regular programmers.  In other words, instead of thinking of
concurrent <em>tasks</em> as a single concept, we should think about
parallel <em>workers</em> and interactive <em>activities</em> as
distinct things.  This thought leads to three questions:
<ul>
<li>What is the most convenient and efficient framework for interactive
software we can make if we explicitly exclude parallelism?</li>
<li>What is the most convenient and efficient framework for parallel
software we can make if we severely limit interactivity?</li>
<li>Can a single application use both frameworks together?</li>
</ul>
</p>

<p>
<a href="http://charcoal-lang.org/short_version.html">Charcoal</a> is
my attempt to answer the first question.
</p>

<p>
I haven't thought as much about the second question, but here's the
direction I'd like to explore.  Start with the worker pool idea, as
embodied by Cilk, TPL, PPL, GCD, TBB.  But instead of
using <em>threads</em> as workers (as I believe all of those frameworks
do), use <em>processes</em>.  This idea is not entirely novel, but I am
not aware of any attempts at general-purpose process-based worker pool
frameworks.
</p>

<!--

<div class="hblock2" style="background-color:rgb(225,235,255)">

<p>
Memory is a fundamental facet of software.  Managing memory for storing
data is a challenging part of software construction that can cause poor
performance and failures if it is done badly.  Most applications written
today exist in a framework that assumes garbage collection will manage
internal memory use.
</p>

<p>
Garbage collection has some performance drawbacks relative to more
manual approaches to memory management and even comes with some of its
own weird problems that need to be programmed around (e.g. I/O of
pointers).  However, the software engineering benefits of garbage
collection are widely believed to outweigh these drawback in the
majority of cases.
</p>

</div>

<div class="hblock2" style="background-color:rgb(255,225,235)">

<p>
Concurrency is a fundamental facet of software.  Managing the
interleaving of actions from different concurrent tasks is a challenging
part of software construction that can cause poor performance and
failures if it is done badly.  Most applications written today avoid
concurrency if at all possible.  When retreat is not possible, most
software uses a motley collection of techniques like event loops,
threads, locks, &hellip; that have widely-acknowledged failings.
</p>

<p>
Transactional memory is one of the more appealing strategies developed
to ease the burden of constructing concurrent software.  TM has pretty
severe performance drawbacks relative to more manual approaches to
concurrency management.  It also has some of its own weird problems that
can be programmed around, but are major roadblocks to wider use of TM
(e.g. I/O in transactions).  Because of these issues, TM is barely used
in mainstream software development.
</p>

<p>
In search of a simple, reliable way to help developers write concurrent
software, we observe that concurrency is really two mostly distinct
concepts: parallel execution and interaction.  Parallelism is about
using multiple processors to speed up software.  Interaction is about
overlapping and interleaving tasks in time to give the appearance of
distinct computations happening concurrently.  While there are
abstractions like threads that can be used for both interaction and
parallelism, it's much easier to make reliable interactive software
without parallelism and it's much easier to make parallel software when
interaction is severely limited.
</p>

</div>

<div class="hblock2" style="background-color:rgb(225,235,255)">

<p>
Though it is not often thought of this way, memory can be seen as having
two distinct dimensions: reuse and reference.  Reuse is about maximizing
efficiency by overwriting a location with new data when its previous
value is no longer needed.  Reference is about passing around the name
of a memory location instead of its contents.  Conventional memory
abstractions provide both reuse and reference.

 necessary, it is possible to think of data as coming in two
distinct flavors: transient and persistent.  Smart management of
transient data is all about reusing finite memory resources to maximize
performance.  Persistent data is often outside the control of a
particular application (e.g. secondary storage or network resources),
but we can also think of global variables as a kind of persistent data.
</p>

<p>
Avoiding data management bugs with persistent data is relatively easy.
Because the storage in question will never (to first approximation) go
away, there are no problems with passing references to it around.
Though it is impossible to get low-level bugs like dangling pointer
dereferences with pointers to memory that is never deallocated, it is
possible for applications to recreate similar problems by storing one
piece of data and then later overwriting it with something else.
</p>

</div>

<div class="hblock2" style="background-color:rgb(255,225,235)">

<p>
Avoiding concurrency bugs for interactive software can be relatively
easy.  Because there is no need for physical simultaneity, we can use
serial abstractions like event handlers or coroutines.  In these
frameworks, low-level bugs like data races simply do not exist.
However, it is possible for enterprising developers to create similar
problems like high-level atomicity violations.
</p>

</div>

<div class="hblock2" style="background-color:rgb(225,235,255)">

<p>
Only allow a single pointer to transient data.  Can copy, but that has
performance and other issues so sometimes it's useful to break the
rules.
</p>

</div>

<div class="hblock2" style="background-color:rgb(255,225,235)">

<p>
Do not allow parallel tasks to refer to the same memory location.  Can
copy, but that has performance and other issues, so sometimes it's
useful to break the rules.
</p>

</div>
-->

<!--

<div class="hblock2" style="background-color:lightblue">

<p>
Resource allocation and deallocation is an important part of how
software operates.  Resources can be divided fairly cleanly into two
categories: internal and external.  External resources are things like
secondary storage space and network connections.  Allocation and
deallocation of external resources by one application is directly
visible to other applications and the general environment.  "Internal
resources" is really just one thing: memory.

The environment generally does not care how an application manages
memory, with a couple of exceptions.  The effectiveness of an
application's memory management can impact its speed and reliability.
If an application tries to use too much memory, it can have an impact on
the performance of other applications.  However, operating systems
provide memory virtualization and do their best to get applications to
share physical memory fairly.

Sharing of memory locations between different applications or an
application and the environment is fairly unusual.  When it does happen,
it must be handled with extreme care.

The difference between allocation of internal and external resources is
widely understood to be sufficiently large that different strategies are
appropriate for allocating and deallocating them.  While automatic
techniques like garbage collection have been extremely successful for
memory management, allocation and deallocation of external resources is
largely done manually.  The issue is that no single entity has a broad
enough view of the system to automatically judge when it's safe to
deallocate external resources.  Nevertheless, some people do try to
automate the management of external resources by linking them to memory
in one way or another.  Such efforts have not been very widely adopted.

The main simplification that makes management of external resources
tractable is insisting on a single "name" for the resources (such as a
file path).  Adding the ability to make multiple aliases to a single
resource complicates the allocation and deallocation process
dramatically.

The main simplification that makes management of internal resources
(i.e. memory) tractable is insisting on the ability to track all aliases
to a single memory location.  It is occasionally useful to make "secret"
aliases (i.e. "weak" pointers).




The environment generally does not care how an application manages
parallel execution, with a couple of exceptions.  The effectiveness of
an application's parallel execution management can impact its speed and
reliability.  If an application tries to use too much parallelism, it
can have an impact on the performance of other applications.  However,
operating systems provide processor virtualization and do their best to
get applications to share physical processors fairly.

Sharing of the instruction execution stream on a processor between
different applications or an application and the environment is very
unusual (though possible).  When it does happen, it must be handled with
extreme care.

The difference between internal and external concurrency is not as
widely appreciated as the resource allocation case.  There are
frameworks, such as event dispatchers, that deal exclusively with
external concurrency.  However threads, which can be used to implement
both external and internal concurrency, are also widely used.

The main simplification that makes management of external concurrency
tractable is insisting on serial execution.  Adding the ability to
perform externally visible actions in parallel dramatically complicates
the job of ensuring that those actions appear in a sensible order.

The main simplification that makes management of internal concurrency
(i.e. parallelism) tractable is insisting that actions performed in one
parallel task are not visible to another (i.e. no sharing/coordination).
In practice, sometimes sharing is worth the complexity that it brings,
but it must be handled with extreme caution.


</div>
-->
<?php include 'copyright.html'; ?>

</div>
</body>
</html>
