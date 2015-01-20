# Software Primitives for Multitasking

## Abstract

Most introductory programming courses focus on _procedural_ software.
Procedural software has a single task: it starts, reads some input, does
some work, produces some output, then finishes.  In contrast, the vast
majority of software we use on a daily basis is interactive.
Interactive software reacts to user input, communicates with other
machines over the network, controls hardware devices, etc.  Interactive
software is also called multitasking software; it has to keep multiple
balls in the air at the same time.

Modern mainstream software is substantially more interactive than its
forbears.  Even as recently as 10 years ago it was much more common to
see applications that ran entirely locally and used simple
keyboard-and-mouse UI.  This change can mostly be chalked up to the rise
of the internet as a software platform and mobile devices with rich user
interfaces.  A lot of smart money is on this trend continuing for the
foreseeable future.  Existing frameworks for multitasking (most
commonly event handlers and threads) have serious weaknesses that have
become painful to software developers.

In this talk we will survey and critique existing primitives for
interactivity.  Then we will look at a new alternative:
pseudo-preemptive threads (a.k.a. activities).

[Short version]

Most introductory programming courses focus on software that does one
task: get input, compute, produce output, done.  But most of the
software we use is multitasking (also called reactive or interactive).
Surprisingly, the software engineering community is not yet settled on
what the "right" framework for building multitasking software is.  In
this talk we will survey and critique the existing options and introduce
a new alternative: pseudo-preemptive threads (a.k.a. activities).

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
- Embedded systems (e.g. self-driving cars)
- Pipelines/phases (e.g. parsing a large log file)
(A common user-level model: a single "foreground" task and any number of
"background" tasks.)

Another way to say interactivity: _virtual simultaneity_.  _Physical
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

| Primitive           | Description                                         |
|---------------------|-----------------------------------------------------|
| [Sequencing](http://media02.hongkiat.com/action-sequence-photography/Snowboard-Sequence-Photography.jpg)          | "Do action X, then action Y"                        |
| [Decisions](http://svprojectmanagement.com/wp-content/uploads/Taming_Email_Decision_Tree.jpg)           | "Do action X or action Y, depending on condition Z" |
| [Loops and recursion](http://creativegibberishcom.ipage.com/wpfiles/wp-content/uploads/2011/08/creative-repetition-andy-monroe.jpg) | "Repeat action X until condition Y"                 |
| [Procedure calling](?)   | "Do a bunch of stuff, then come back here"          |
| [Exception handling](?)  | "Try action X, but if it fails do Y"                |

Quick historical note: The first substantial interactive software
demonstration was Ivan Sutherland's Sketchpad in the mid-1960s.
However, interactivity didn't go mainstream until the Macintosh and
Amiga in the 1980s.  One of the most forward-looking demonstrations of
interactive software was Douglas Engelbart' "[mother of all demos](https://www.youtube.com/watch?v=yJDv-zdhzMY)".

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

