<html>
<head>
<title>Memory/Concurrency Analogy</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<?php include 'side_bar_basic.html'; ?>

<div class="main_div">

<header>
<h1>Memory/Concurrency Analogy</h1>
</header>

<p>
This page will make a lot more sense if you are familiar with Dan
Grossman's
<a href="http://www.google.com/search?q=The+Transactional+Memory+Garbage+Collection+Analogy">
memory management/concurrency analogy essay</a>.
</p>

<p>
Spoiling the punchline: I think that concurrency in software should
really be considered two different things: parallelism and
interactivity.  Parallelism should be programmed with the worker pool
pattern (e.g. Cilk), but the workers should be <em>memory-isolated</em>
by default.  Interactivity should be programmed with a serial framework.
This website is devoted to a new thread-like primitive that is intended
to fill this role better than existing options (event loops, cooperative
threads).
</p>

<p>
This page is an attempt to extend Dan's memory/concurrency analogy in
support of this vision.
</p>

<div class="hblock2" style="background-color:rgb(225,235,255)">

<p>
Data storage is a fundamental facet of software.  Managing memory for
storing data is a challenging part of software construction that can
cause poor performance and failures if it is done badly.  Most
applications written today exist in a framework that assumes garbage
collection will manage internal memory use.
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
interleaving of actions from different concurrent actors is a
challenging part of software construction that can cause poor
performance and failures if it is done badly.  Most applications written
today avoid concurrency if at all possible.  When retreat is not
possible, most software uses a motley collection of techniques like
event loops, threads, locks, &hellip; that have widely-acknowledged
failings.
</p>

<p>
Transactional memory is one of the more appealing strategies developed
to ease the burden of constructing concurrent software.  TM has pretty
severe performance drawbacks relative to more manual approaches to
concurrency management.  It also has some of its own weird problems that
can be programmed around, but are major roadblocks to wider use of TM
(e.g. I/O in transactions).  Because of these issues, TM is barely used
at all in mainstream software development.
</p>

<p>
In search of a simple, reliable way to help developers write concurrent
software, we observe that concurrency is really two mostly distinct
concepts: parallel execution and interaction.  Exploiting parallelism is
all about using multiple processors to speed up software.  Interaction
is about overlapping multiple tasks in time either to handle interaction
with the real world or because that pattern is simply more elegant.
</p>

</div>

<div class="hblock2" style="background-color:rgb(225,235,255)">

<p>
Though it is not often necessary, it is possible to think of data as
coming in two distinct flavors: transient and persistent.  Smart
management of transient data is all about reusing finite memory
resources to maximize performance.  Persistent data is often outside the
control of a particular application (e.g. secondary storage or network
resources), but we can also think of global variables as a kind of
persistent data.
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
