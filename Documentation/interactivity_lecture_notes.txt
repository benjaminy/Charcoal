# Interactive Software

## Abstract

Modern mainstream software is substantially more interactive than its
forbears.  Even as recently as 10 years ago it was much more common to
see applications that ran entirely locally and used simple
keyboard-and-mouse UI.  This change can mostly be chalked up to the rise
of the internet as a software platform and mobile devices with rich user
interfaces.  A lot of smart money is on this trend continuing for the
foreseeable future.  Existing frameworks for making software interactive
(most commonly event handlers and threads) have serious weaknesses that
have become painful to software developers.

In this talk we will discuss the existing approaches to interactivity;
especially their weaknesses.  Then we will look at a new altnerative:
activities (a.k.a. pseudo-preemptive threads).

/Abstract

## Outline:

- Intro
- Background: strengths and weaknesses of existing frameworks
  - Event handling
  - Threads
  - Coroutines
  - Cooperative threads
  - Processes
  - Functional reactive programming
- Activities (pseudo-preemptive threads) def'n
- Activities compared to events, threads, ...
- Research project opportunities

### Interactive software def'n:
Software that _gives the appearance_ of multiple tasks making progress
simultaneously.

###Examples of interactive software:
- Anything with a graphical user interface
- Anything that talks to a network
- Pipelines/phases (e.g. parsing a large log file)
(A common user-level model: a single "foreground" task and any number of
"background" tasks.)

Another way to say interactivity: _virtual simultaneity_.  _physical
simultaneity_ (a.k.a. parallelism) is using multiple processors to make
a single task go faster.  I'm not going to talk about parallelism today.
Brief side note on terminology (warning: a lot of this jargon is used
inconsistently by different people): Interactivity and parallelism
together make up concurrency.  A pet peeve of mine is people muddling
concurrency, parallelism and interactivity together.

A surprising fact: Interactivity is important and has existed for a long
time (relative to computing as a field), yet we still do not have
programming language-level frameworks for interactive software that get
consensus approval from software engineers and researchers.  Contrast
this with primitives for non-interactive software:
- Sequencing. "Do action X, then action Y"
- Decisions. "Do action X or action Y, depending on condition Z"
- Loops and recursion. "Repeat action X until condition Y"
- Procedure calling. "Do a bunch of stuff, then come back here"
- Exceptional case handling. "Try action X, but if it fails do Y"

Quick historical note: The first substantial interactive software
demonstration was Ivan Sutherland's Sketchpad in the mid-1960s.
However, interactivity didn't go mainstream until the Macintosh and
Amiga in the 1980s.

Give away the punchline: Mainstream software today is substantially more
interactive than it was 10 years ago and I believe that trend will
continue for a while.  Most applications have richer user interfaces and
more network integration.  This has exposed the weaknesses of the
frameworks we use to build interactive software.  I'm working on an
alternative called "activities" or "pseudo-preemptive threads" that I
claim addresses those weaknesses.

Software is getting increasingly interactive
and our existing abstractions for interactivity leave something to be
desired.  I'm working on a new one called "activities" or
"pseudo-preemptive threads".  But first let's survey the
state-of-the-art in interactive software.

- Event handling. (~All GUI frameworks; esp. JavaScript)
  - Strength: Simple.  Great for basic interaction patterns.
  - Weakness: Starvation; "callback hell".
- (Preemptive) Threads. (~All systems; interestingly not JavaScript)
  - Strength: Write each task using non-interactive control flow.
  - Weakness: Concurrency bugs galore (races, deadlocks, etc.).
- Coroutines. (.NET async/await, JavaScript generators/promises)
  - Strength: A sliver of threads' natural control flow.
  - Weakness: Same as event handlers; just pushed back a bit.
- Cooperative threads. Like threads, with yield
  - Strength: More like threads than coroutines in programming style.
  - Weakness: Where should the yields go???
- Processes. Isolated threads.
  - Strength: Strong concurrency bug resistance.
  - Weakness: Most applications want shared state.
- Functional reactive programming. Wacky.
  - Strength: Solid formal model.
  - Weakness: Lots of practical questions; still a research topic.



- Event handling.

- Threads.

- Coroutines.
  - .NET async/await
  - JavaScript generators/promises

- Cooperative threads.

- Processes.

- Functional reactive programming.

