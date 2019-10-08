Back in 2007, Dan Grossman published a really cool essay called "The Transactional Memory / Garbage Collection Analogy".
Dan noted the uncanny number of ways in which garbage collection and transactional memory are very similar, if you swap the role of time and space.
(If you prefer audio, Dan explained these ideas on episode 68 of the Software Engineering Radio podcast.)

While I am definitely not accusing Dan of irresponsibly advocating for transactional memory (his wording is very carefully neutral), he has invested some of his research effort in that technology and clearly sees/saw hope for its wider use in the popularity of garbage collection.

In this essay I am going to expand Dan's analogy to broader memory/concurrency points of similarity.
My primary agenda is to emphasize that concurrency has two very different faces (parallelism and multitasking) and to argue that programming languages are better off having completely separate features to support them.

Warm-up: A Reason to be Cautious about Betting on Transactional Memory Achieving the Same Ubiquity as Garbage Collection

The memory/concurrency analogy does not say that things on either side of the analogy are identical (of course).
In particular, there are many ways in which time and space play dual roles, and time and space are pretty different beasts.
For example, consider the following (analogous) trivial strategies for memory management and parallelism:

Never reclaim allocated memory.
Once a particular location is allocated to a particular value, it will never be used for another one, even if that value will never be used by the program again.
This strategy has the obvious problem that a program that has nontrivial memory churn will use much more memory than it needs to.
This can lead to running out of memory in the worst case.

Serialize all tasks.
Even if a program expresses the possibility of parallel execution, all code will be run on a single processor.
The problem that is technically analogous to memory exhaustion is deadlock, which is certainly possible if the system insists on the "wrong" serialization of tasks.
However, one could reinterpret this strategy as the programmer just wrote a sequential program to begin with and never worried about parallel execution.

I think that we see a major issue with the analogy here.
All programs that run for a nontrivial amount of time and have some nontrivial memory churn (an awful lot of programs) need some kind of memory reuse strategy.
On the other hand, the one and only point of parallelism is to speed up programs that have a lot of computational work to do.
An awful lot of programs in the world are not limited by the amount of computational work they have to do, so just shouldn't bother with parallelism at all.

To state that even a little more strongly, I don't think software developers should even think about parallelism until they have put a lot of effort into making sure their algorithms are good, and they've done some serious profiling work to eliminate incidental inefficiencies.
On a good day parallelism gives constant-factor speedups and is hard to get right.

So while garbage collection is relevant to a very wide range of program kinds, parallelism is only relevant to programs with moderately high performance requirements and moderately large amounts of work to do.
Once we have narrowed our scope to this collection of programs (admittedly, still a large collection) we find another sticky tension with the garbage collection analogy.
Though modern garbage collectors are engineering marvels that are far more efficient than they have any right to be, they do still have some efficiency concerns.
(See e.g. "Quantifying the Performance of Garbage Collection vs. Explicit Memory Management" by Hertz and Berger.)
Languages like Rust and C++ are still sticking with explicit memory management by default.
So if TM comes with any hard to avoid inefficiencies (by analogy with GC) it will have a harder time gaining adoption for highly performance sensitive programs than GC did for not-especially-performance-sensitive programs.

The Main Event: Parallelism vs. Multitasking

I believe that concurrency in software has two very different faces.
I sometimes like to think of these as "internal" and "external".

Internal concurrency is parallelism.
The program has lots of work to do an it speeds it up by breaking it into subtasks and running those on different processors.
From an external interface perspective, parallelism is largely irrelevant.
It just make a program faster or slower.

External concurrency is multitasking.
The world outside the program (or sub-program) is non-deterministic and has lots of input and output connections to the program.
In order to remain responsive to these unpredictable external connections, the program has to be ready to switch from one task to another with reasonably low latency.
Whether these internal concurrent tasks run physically in parallel or share a single processor is completely irrelevant in most situations.

Dan brushes up against this distinction in the I/O section of his TM/GC essay.
However, the essay seems almost exclusively focused on parallelism.
Interaction with agents outside the program is an annoyance that makes TM harder.

To expand on that I/O section, the internal view of memory is good old memory management (new, malloc, pointers, garbage collection, etc).
The external view of memory is file storage, and the interesting problem is serializing data structures with references to external storage.
The especially interesting part of that problem is when data structures have reference cycles.
Such cycles make it impossible to use simple serialization strategies that just follow the object graph to its leaves and serialize bottom-up.
Most language built-in support for serialization fails to handle cyclic data structures (even having references at all is a challenge to some of the crappier ones).
Attempting to serialize a cyclic data structure in a piecemeal way, interleaved with unrelated data is likely to end in tears because the serialization process may paint itself into a corner where the location it has already committed to putting the serialization of a particular object is already occupied by some other data.

I claim that the concurrency idea analogous to serialization of data structures is the serialization of chunks of concurrent tasks.
(OMG, apparently unrelated uses of the word "serialization" in different contexts are actually the same thing through the concurrency/memory analogy mirror 🤯.)

Event loops/callbacks and coroutines (a.k.a. async functions) are already very widely used technologies for multitasking.


The Punchline: Multithreading is a Terrible Hybrid

Multithreading is a technology that has one foot in the parallelism camp and one foot in the multitasking camp.
I think this is generally bad.
One way to gain some intuition about this is to look through the analogy mirror.
I believe that what we find there for multithreading is memory/storage technologies that try to blur the line between in-memory data structures and external storage; things like object-relational mappers and memory-mapped files (interesting coincidence of the word "map").
While these technologies have some legitimate uses, I think they're vulnerable to internal/external confusions the same way multithreading is.
In most applications most of the time it's better to completely avoid memory/storage mapping and multithreading.

Bonus Feature: Better Parallelism through Isolation

So if I dislike multithreading so strongly, how should we write parallel code?
We can't let those sad underutilized multicores go to waste!

As I suggested earlier, I think most applications most of the time just shouldn't bother with parallelism.
It's really a technology for fairly late-stage performance optimization.
With that caveat out of the way ...

Before I get to the meat of the analogy, a quick endorsement for Cilk-style worker pool/task dispatcher style parallelism.
This pattern has already become quite popular (Java Fork/Join, Microsoft TPL, Apple GCD, etc).
Application code should not be creating workers (i.e. threads/processes).
Rather, each application should have a single global task dispatcher that is the intermediary between tasks submitted by application code and the underlying workers.

I think parallelism should take a hint from Rust and modern C++.
Those language communities have resisted garbage collection, but it is a truth universally acknowledged that an object with the good fortune of multiple references from unrelated objects is in want of a memory bug.
So those explicit memory management languages have pushed hard on the idea of there only being a single reference to any dynamically allocated object.
In Rust this is the elaborate ownership rules in the type system.
C++ is (naturally) more laid-back about this, but unique_ptr is now being pushed by the standards committee as a solution to many memory problems.

The concurrency analogy to single reference is isolated workers.
By default workers should be isolated processes, not shared-memory threads.
Memory sharing need not be completely outlawed, but application code should be forced to explicitly ask for it, and only use it for performance-critical data structures.