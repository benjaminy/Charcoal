# ORMs : Memory/Storage :: Multithreading : Concurrency
## ... and other analogies you won't find on the SATs

Table of Contents:

- [GC/TM Analogy Recap](#super-quick-recap-of-dan-grossmans-gctm-analogy)
- [Betting Against TM](#warm-up-a-reason-to-bet-against-transactional-memory-achieving-gc-like-ubiquity)
- [Concurrency = Parallelism ∪ Asynchrony](#the-main-event-concurrency--parallelism-span-stylefont-sizexx-large∪span-asynchronybrparallelism-span-stylefont-sizexx-large⋂span-asynchrony-span-stylefont-sizexx-large≈span-span-stylefont-sizexx-large∅span)
- [Better Asynchrony](#better-asynchrony)
- [Better Parallelism](#better-parallelism)

<hr>

Back in 2007, Dan Grossman published a cool essay called "The Transactional Memory / Garbage Collection Analogy" ([ACM](https://dl.acm.org/citation.cfm?id=1297080), [LtU](http://lambda-the-ultimate.org/node/2990), [pdf](https://homes.cs.washington.edu/~djg/papers/analogy_oopsla07.pdf)).
He noticed several striking similarities between garbage collection (GC) and transactional memory (TM).
His essay is _mostly_ focused on GC and TM in particular, but there are some hints at a broader analogy between memory management and concurrency, which seem like just "two different problems in software development".
(If you prefer audio, Dan discussed these ideas on episode 68 of the Software Engineering Radio podcast.)
An especially intriguing feature of the analogy is that time and "space" (i.e. memory or parallel processors) play complementary roles in many of the particulars.

Though Dan's essay is more analysis than advocacy, he was clearly enthusiastic about TM and drew from this analogy inspiration regarding TM making parallel programming easier in the way that GC makes programming in general easier.
(He carefully does _not_ make the claim that TM makes parallel programming as easy as plain old sequential programming with GC.)

In this essay I elaborate on Dan's analogy to cover broader memory/concurrency points of similarity.
Such expansion is risky in the sense that this is just an analogy after all, and it's possible to stretch it beyond its breaking point; I hope I haven't done that.

I do have an agenda, which is roughly:
- I think that concurrency is too often thought of as a single concept.
I think it's better to think of concurrency as two _mostly different_ things: parallelism and asynchrony.
I think these two concepts are analogous to in-process memory and external storage (databases, files), which programmers mostly treat as two different things.
- I think multithreading (i.e. explicitly spawned concurrent processes that share all memory by default) is too hard to be really mainstream in the foreseeable future (progress on TM notwithstanding).
(By "mainstream" I mean something like commonly used as a matter of course by Jane and Joe programmers working on some business app.)
- In an effort to malign multithreading I argue that when you translate it through the memory-concurrency analogy what you find are object-relational mapping (ORMs) and memory-mapped files, technologies that have their uses but also significant limitations.

## Super Quick Recap of Dan Grossman's GC/TM Analogy

This is just a bullet-pointy summary of some of the particularities of Dan's analogy.
I highly recommend reading his essay for more details.

### Explicit malloc/free : Multithreading with explicit lock/mutex-based critical sections

### Use-after-free : Data races

On the memory side there's a kind of collision in space, on the concurrency side a collision in time.

### Memory leak : Over-broad Synchronization

On the memory side, wasting space, on the concurrency side wasting time.

### Memory exhaustion : Deadlock

### Garbage collection : Transactional memory

On the memory side the programmer is responsible for allocating memory and managing references to live memory correctly.
The system automagically decides when memory locations are no longer needed.
On the concurrency side the programmer decides where to place critical sections.
The system automagically decides which critical sections can be run in parallel without creating a conflict.

### More details

Dan goes into a fair amount of detail about the implementation of GC and TM and how one finds analogous techniques and trade-offs on either side.

## Warm-up: A Reason to Bet Against Transactional Memory Achieving GC-Like Ubiquity

The memory/concurrency analogy does not imply that things on either side of the analogy are identical (of course).
In particular, there are many ways in which time and space play complementary roles, and time and space are pretty different beasts.
For example, consider the following (analogous) trivial strategies for memory management and parallelism:

> Never reclaim allocated memory.
> Once a location is allocated for an object, that location will never be used for another object, even after that object is no longer live.
> This strategy has the obvious problem that a program that exhibits any memory churn will use more memory than it needs to.
> This can lead to memory exhaustion in the worst case.

> Serialize all tasks.
> Even if a program expresses the possibility of parallel execution, all execution is serialized on a single worker.
> This strategy has the obvious problem that a program that has any potential parallelism will use more time than it needs to.
> The problem that is analogous to memory exhaustion is deadlock, which is certainly possible if the system insists on the "wrong" serialization of tasks.
> However, one could reinterpret this strategy as the programmer having written a sequential program to begin with and never worried about parallel execution.

I think that we see an important issue with the analogy here.
All programs that run for a nontrivial amount of time and have some memory churn (an awful lot of programs) need some kind of memory recycling strategy.
On the other hand, the one and only point of parallelism is to speed up programs that have a lot of computational work to do.
An awful lot of programs in the world are not limited by the amount of computational work they have to do, so just shouldn't bother with parallelism at all.

To state that even a little more strongly, I don't think software developers should even think about parallelism until they have put a lot of effort into making sure their algorithms are good, and they've done some serious profiling work to eliminate incidental inefficiencies.
On a good day parallelism gives constant-factor speedups and is hard to get right.

So while memory recycling is almost universal (with some exceptions like high reliability embedded systems), parallelism is only relevant when moderately high performance requirements and moderately large amounts of work are in play.
Once we have narrowed our scope to these scenarios (admittedly, still a large collection) the garbage collection analogy highlights another tension.
Though modern garbage collectors are engineering marvels that are far more efficient than they have any right to be, they do still have some efficiency concerns.
(See e.g. "Quantifying the Performance of Garbage Collection vs. Explicit Memory Management" by Hertz and Berger.)
Systems-y languages like Rust, C++ and D are still strongly biased towards explicit memory management.
So if TM comes with any hard to avoid inefficiencies (by analogy with GC) it will have a harder time gaining adoption for highly performance sensitive programs than GC did for not-especially-performance-sensitive programs.

## The Main Event: Concurrency = Parallelism <span style="font-size:xx-large;">∪</span> Asynchrony<br/>Parallelism <span style="font-size:xx-large;">⋂</span> Asynchrony <span style="font-size:xx-large;">≈</span> <span style="font-size:xx-large;">∅</span>


I believe that concurrency in software has two very different faces that I call parallelism and asynchrony.
Note: I'm not trying to claim that this distinction is any sort of groundbreaking observation on its own.
There are plenty of examples of existing language features, frameworks, etc that are aligned with this distinction.
Rather, it is also still common to think of _concurrency_ as a single thing, which I think is mostly detrimental.

Let's start with the memory/storage side of the analogy since it's often easier to think/talk/write about memory than concurrency.

### Locations Names: Meaningful Outside the System or Not?

Among the many concepts, abstractions, frameworks and systems around memory/storage, one of the most important distinctions is whether location names are meaningful only within a particular system (e.g. a running program) or outside of that.
Locally meaningful location names leads to the world of pointers, in-memory data structures and conventional memory management.
For example, a common beginner mistake is to save (i.e. serialize) a pointer to some external storage, and then try to restore it later.
Of course, it is likely that whatever was at that memory location when the pointer was saved is no longer there.

Externally meaningful location names leads to the world of databases and files; things like `Alice's date of birth in Table Users` and `Cell D6 in Sheet 4`.


### Event Ordering: Meaningful Outside the System or Not?

If the concurrency of a system is only meaningful within that system we are in the world of parallelism.
The programmer creates order flexibility among the actions performed by a program for the purpose of speeding it up by running different sub-tasks on different workers.
Note: I am not saying anything at all about the _scale_ of the parallelism.
Instruction-level parallelism within processors, GPUs, multicores, warehouse-scale supercomputers; these are all examples of hardware that can support parallelism.
Of course the way one goes about programming these different kinds of systems is very different, but they all share the essential property of supporting speeding up computational work by doing pieces of it physically simultaneously.
And ideally the result of the program doesn't depend on scheduling.

If the concurrency of a system is meaningful outside of that system we are in the asynchronous world of latency hiding, reacting promptly to user input, servers handling multiple concurrent network connections, etc.
The most important concept related to asynchrony is _interruption_.
In this world, the program is interacting with some agent outside of itself whose timing the program is at least partially not in control of.

### "But but but" said Butt the Hoopoe ...

> I want BOTH parallelism and asynchrony.
> I'm writing a wicked scalable NoSQL eventually consistent agile platform server.
> It needs to be horizontal and vertical and have 1μs response time to every human on the planet!

Wanting both parallelism and asynchrony is perfectly reasonable.
However, the desire for efficient in-memory data structures and efficient external storage does not lead to using a single interface for these two things.
I think we'd be in a better place with respect to concurrency if we didn't try to do it with a single abstraction (I'm looking at you, multithreading).

### Asynchrony Without Parallelism

There is already pretty rich support for asynchrony without parallelism in many programming languages.
The most prominent example of this is the widespread adoption of coroutines/generators/async functions in the last decade or so.
While async functions are a pretty good tool for implementing asynchrony, I have a few analogy-inspired thoughts about possible improvements below.

<!-- I claim that the concurrency idea analogous to serialization of data structures is the serialization of chunks of concurrent tasks. -->
<!-- (OMG, apparently unrelated uses of the word "serialization" in different contexts are actually the same thing through the concurrency/memory analogy mirror 🤯.) -->


<!-- Dan brushes up against this distinction in the I/O section of his TM/GC essay. -->
<!-- However, the essay seems almost exclusively focused on parallelism. -->
<!-- Interaction with agents outside the program is an annoyance that makes TM harder. -->

<!-- To expand on that I/O section, the internal view of memory is good old memory management (new, malloc, pointers, garbage collection, regions, etc). -->
<!-- The external view of memory is file/database storage, and the interesting problem is serializing data structures with pointers/references. -->
<!-- The especially interesting part of that problem is when data structures have reference cycles. -->
<!-- Such cycles make it impossible to use simple serialization strategies that just follow the object graph to its leaves and serialize bottom-up. -->
<!-- Most language built-in support for serialization fails to handle cyclic data structures (even having references at all is a challenge to some of the crappier ones). -->
<!-- Attempting to serialize a cyclic data structure in a piecemeal way, interleaved with unrelated data is likely to end in tears because the serialization process may paint itself into a corner where the location it has already committed to putting the serialization of a particular object is already occupied by some other data. -->




### Parallelism Without Asynchrony

This is where I think current programming languages do less well.
In particular, multithreading is still the most widespread tool for implementing parallelism in mainstream systems.

In search of a better way to do parallelism, it is interesting to look at the trend in memory management in "systems-y" languages like C++ and Rust.
That trend is towards not allowing/encouraging multiple references to an object.
In Rust this restriction is supported extensively in the type system with ownership, borrowing, etc.
In C++ the support is less formal, but `unique_ptr` has gained popularity quickly since its standardization in C++11.

I claim that what one finds on the concurrency side of the analogy across from the single object ownership is not sharing memory between workers.
Of course non-shared memory concurrency is not a new idea.
However, with a few notable exceptions (e.g. Erlang) isolation is not a feature of mainstream parallel systems.
Below I have a few analogy-inspired thoughts about improving mainstream parallel programming.

### Parallelism Muddled with Asynchrony = Multithreading

Multithreading is a technology that has one foot in the parallelism camp and one foot in the asynchrony camp.
One response I sometimes get to this claim is that multithreading is really for parallelism; people don't really use it for asynchrony.
If the previous sentence sounds reasonable to you, I encourage you to fire up your favorite system monitor on whatever computer you're using to read this.
What you will likely find is lots of processes that are consuming very little CPU time (because they're not doing much of anything), yet have dozens of idle threads hanging out.
What are these threads doing?
Probably most of them are waiting to react to some system event like a mouse click, network packet arrival or file modification.
In other words, they're threads doing asynchrony, not parallelism.

### In-Memory Objects Muddled with External Storage = ORMs

One way to gain some intuition about the multithreading muddle is to look through the analogy mirror.
I believe that what we find there for multithreading is memory/storage technologies that try to blur the line between in-memory data structures and external storage; things like object-relational mappers (ORMs) and memory-mapped files (interesting coincidence of the word "map").
While these technologies have some legitimate uses, I think they're vulnerable to internal/external confusions the same way multithreading is.
For example, using an ORM it would be easy to accidentally store a data structure into a database that contained pointers to local memory that would be totally meaningless in any other memory space.
In most applications most of the time it's better to completely avoid _magical_ memory/storage mapping and multithreading.

## Better Asynchrony

As suggested above, I think the best thing going for asynchrony in mainstream programming right now is async functions.
My main complaint with async functions is that they give very little control over the ganularity of atomic sections.
As Dan points out in his essay, choosing how "big" atomic sections should be is one of the most important things programmers do.
If they're too big, responsiveness can be a problem.
If they're too small, atomicity violations become more likely.

One bit of evidence that this atomicity is "duplicate" functions in the node.js and .NET standard libraries.
These are two of the most widely used programming frameworks in the world, and arguably the two most important that make significant use of async functions.
Interestingly one can find quite a few function pairs that have the same functionalty, but one is async and one is a regular atomic function.
(A couple random examples: `readFile` / `readFileSync`, `AcceptTcpClient` / `AcceptTcpClientAsync`.)

This kind of duplication does demand some explanation.
Naively it would seem that having the async version would be sufficient, because a call to it can always be `await`ed.
One possible explanation is simple backwards compatibility cruft (i.e. the atomic version came first and cannot be removed for compatibility reasons).
I suspect that the atomic versions are actually useful in some (perhaps niche) situations.

It might be better to have an atomic block for async functions to let the programmer enforce atomicity wherever they see fit.
My own wild-n-crazy crack at doing this in JavaScript is [here](https://github.com/benjaminy/atomicable).

My other beef with async functions is that I think they get the default wrong regarding spawning asynchronous flows versus regular blocking function calls.
What I mean by this is that most (by a pretty hefty margin) calls to async functions in the most popular JavaScript and C# repos on github are immediately `await`ed.
This suggests to me that it would be better to start with something more like cooperative threads where regular function calls are like `await`ed calls to async functions, and asynchronous flows have to be explicitly created with some syntax.


## Better Parallelism

So if I dislike multithreading so strongly, how should we write parallel code?
We can't let those sad underutilized multicores go to waste!

As I suggested earlier, I think most applications most of the time just shouldn't bother with parallelism.
It's really a technology for fairly late-stage performance optimization.
With that caveat out of the way ...

Before I get to the meat of the analogy, a quick endorsement for Cilk-style worker pool/task dispatcher style parallelism.
Versions of this pattern have already become quite popular: Java Fork/Join, Microsoft TPL, Apple GCD, etc.
Application code should not be creating workers (i.e. threads/processes).
Rather, each application should have a single global task dispatcher that is the intermediary between tasks submitted by application code and the underlying workers.
(This is maybe analogous to malloc/free or new/delete being a single global dispatcher for memory; application code should think in terms of data structures, not raw byte arrays.)

I think parallelism should take a hint from Rust and modern C++.
Those language communities have resisted garbage collection, but it is a truth universally acknowledged that an object, with the good fortune of multiple references from unrelated objects, must be in want of a CVE number.
So those explicit memory management languages have pushed hard on the idea of exactly one reference to dynamically allocated objects.
In Rust this is the memory ownership and borrowing system.
C++ is (naturally) more laid-back about this, but <code>unique_ptr</code> is now being pushed by the standards committee as a solution to many memory problems.

The concurrency analogy to ownership/unique object references is isolated workers.
By default workers should be isolated processes, not shared-memory threads.
Memory sharing need not be completely outlawed, but application code should be forced to explicitly ask for it, and only use it for performance-critical data structures.
Most sharing between workers should be accomplished through external databases that are designed to work with multiple processes and have fancy caching logic built in.
