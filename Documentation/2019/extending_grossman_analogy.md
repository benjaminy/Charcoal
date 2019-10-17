Back in 2007, Dan Grossman published a cool essay called "The Transactional Memory / Garbage Collection Analogy".
He noted several striking similarities between garbage collection (GC) and transactional memory (TM).
The essay is mostly focused on TM and GC in particular, but there are some comments on a broader analogy between parallelism and memory management, which at first blush seem like just "two different problems in software development".
(If you prefer audio, Dan discussed these ideas on episode 68 of the Software Engineering Radio podcast.)
Especially intriguing is that time and "space" (i.e. memory or parallel processors) play dual roles in many of the particular points of the analogy.

Though it wasn't exactly a piece of advocacy, Dan was clearly interested in TM as a technology and drew from this analogy some hope that TM can help make parallel programming easier in the way the GC made programming in general easier.
(He carefully does not make the claim that TM makes parallel programming as easy as plain old sequential programming with GC.)

In this essay I am going to expand Dan's analogy to broader memory/concurrency points of similarity.
Such expansion is risky in the sense that this is just an analogy after all, and it's possible to stretch it beyond its breaking point; I hope I haven't done that.
I do have an agenda, which is roughly:

- I think multithreading (i.e. parallel programming with shared memory by default) is too hard to be adopted widely in the foreseeable future (progress on TM notwithstanding).
- In an effort to malign multithreading I argue that when you look at it through the concurrency-memory analogy what you find are object-relational mapping (ORMs) and memory-mapped files, technologies that have their uses but also significant limitations.
- This view of multithreading comes from a broader attempt to cleanly separate parallelism from multitasking/asynchrony, which I claim is analogous to the separation between in-memory data structures and external file/database storage.

## Warm-up: A Reason Bet Against Transactional Memory Achieving GC-Like Ubiquity

The concurrency/memory analogy does not imply that things on either side of the analogy are identical (of course).
In particular, there are many ways in which time and space play dual roles, and time and space are pretty different beasts.
For example, consider the following (analogous) trivial strategies for memory management and parallelism:

> Never reclaim allocated memory.
> Once a particular location is allocated to a particular value, it will never be used for another one, even after that value is no longer live.
> This strategy has the obvious problem that a program that has nontrivial memory churn will use much more memory than it needs to.
> This can lead to memory exhaustion in the worst case.

> Serialize all tasks.
> Even if a program expresses the possibility of parallel execution, all code will be run on a single processor.
> The problem that is analogous to memory exhaustion is deadlock, which is certainly possible if the system insists on the "wrong" serialization of tasks.
> However, one could reinterpret this strategy as the programmer having written a sequential program to begin with and never worried about parallel execution.

I think that we see an important issue with the analogy here.
All programs that run for a nontrivial amount of time and have some memory churn (an awful lot of programs) need some kind of memory reuse strategy.
On the other hand, the one and only point of parallelism is to speed up programs that have a lot of computational work to do.
An awful lot of programs in the world are not limited by the amount of computational work they have to do, so just shouldn't bother with parallelism at all.

To state that even a little more strongly, I don't think software developers should even think about parallelism until they have put a lot of effort into making sure their algorithms are good, and they've done some serious profiling work to eliminate incidental inefficiencies.
On a good day parallelism gives constant-factor speedups and is hard to get right.

So while garbage collection is relevant in a very wide range software engineering scenarios, parallelism is only relevant when moderately high performance requirements and moderately large amounts of work are in play.
Once we have narrowed our scope to these scenarios (admittedly, still a large collection) we find another sticky tension with the garbage collection analogy.
Though modern garbage collectors are engineering marvels that are far more efficient than they have any right to be, they do still have some efficiency concerns.
(See e.g. "Quantifying the Performance of Garbage Collection vs. Explicit Memory Management" by Hertz and Berger.)
Languages like Rust and C++ are still sticking with explicit memory management by default.
So if TM comes with any hard to avoid inefficiencies (by analogy with GC) it will have a harder time gaining adoption for highly performance sensitive programs than GC did for not-especially-performance-sensitive programs.

## The Main Event: Parallelism vs. Multitasking

I believe that concurrency in software has two very different faces.
I sometimes like to think of these as "internal" and "external".

_Internal_ concurrency is parallelism.
The program has lots of work to do and it speeds it up by breaking it into subtasks and running those on different processors.
From an external interface perspective, parallelism is largely irrelevant.
It just make a program faster or slower.

_External_ concurrency is multitasking.
The outside world is non-deterministic and has lots of I/O connections to programs.
One good strategy for handling these outside interactions is to have different logical sub-tasks for different connections.
In order to remain responsive, programs have to be ready to switch from one task to another with reasonably low latency.
Whether these internal concurrent tasks run physically in parallel or timeshare a single processor is completely irrelevant in most situations.

Dan brushes up against this distinction in the I/O section of his TM/GC essay.
However, the essay seems almost exclusively focused on parallelism.
Interaction with agents outside the program is an annoyance that makes TM harder.

To expand on that I/O section, the internal view of memory is good old memory management (new, malloc, pointers, garbage collection, regions, etc).
The external view of memory is file/database storage, and the interesting problem is serializing data structures with pointers/references.
The especially interesting part of that problem is when data structures have reference cycles.
Such cycles make it impossible to use simple serialization strategies that just follow the object graph to its leaves and serialize bottom-up.
Most language built-in support for serialization fails to handle cyclic data structures (even having references at all is a challenge to some of the crappier ones).
Attempting to serialize a cyclic data structure in a piecemeal way, interleaved with unrelated data is likely to end in tears because the serialization process may paint itself into a corner where the location it has already committed to putting the serialization of a particular object is already occupied by some other data.

I claim that the concurrency idea analogous to serialization of data structures is the serialization of chunks of concurrent tasks.
(OMG, apparently unrelated uses of the word "serialization" in different contexts are actually the same thing through the concurrency/memory analogy mirror 🤯.)

Event loops/callbacks and coroutines (a.k.a. async functions) are already very widely used technologies for multitasking...


## The Punchline: Multithreading is a Terrible Hybrid

Multithreading is a technology that has one foot in the parallelism camp and one foot in the multitasking camp.
I think this is generally bad.
One way to gain some intuition about this is to look through the analogy mirror.
I believe that what we find there for multithreading is memory/storage technologies that try to blur the line between in-memory data structures and external storage; things like object-relational mappers (ORMs) and memory-mapped files (interesting coincidence of the word "map").
While these technologies have some legitimate uses, I think they're vulnerable to internal/external confusions the same way multithreading is.
For example, using an ORM it would be easy to accidentally store a data structure into a database that contained pointers to local memory that would be totally meaningless in any other memory space.
In most applications most of the time it's better to completely avoid _magical_ memory/storage mapping and multithreading.

## Bonus Feature: Better Parallelism through Isolation

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