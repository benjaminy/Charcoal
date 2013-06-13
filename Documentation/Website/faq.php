<html>
<head>
<title>Charcoal FAQ</title>
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

<h1>"Frequently" Asked Questions</h1>
<h3>a.k.a. questions that the authors feel like answering</h3>

<h4>Q01: Why design another programming language?</h4>

<p>It's true that there are too many programming languages in the world
already.  However, concurrency is one of the facets of programming
language design that really can't be done in any other way.  See, e.g., 
<a href="http://www.hpl.hp.com/techreports/2004/HPL-2004-209.html">Threads
Cannot be Implemented as a Library</a>.</p>

<h4>Q02: Why start with C?</h4>

<p>C is no longer the dominant application programming language in lots
of software domains.  However, it has two properties that make it
attractive for projects like this one:</p>

<ul>
<li>C is relatively simple, which means that extensions to its syntax
and semantics can be designed relatively easily.
<li>C is still the common language of system implementation.  If the
ideas that I am experimenting with in Charcoal gain any traction, I hope
that they will be integrated into a wide range of languages.  I expect
that choosing C as the basis will make it as easy as possible for other
developers to steal the ideas and implementation techniques.
</ul>

<a id="parallel">
<h4>Q03: Isn't it dumb to make a concurrent language that can't be run
in parallel?</h4>

<p>As discussed elsewhere on this site, concurrency and parallelism
are <em>very</em> different topics.  Part of the inspiration behind this
project is that insisting on parallel execution is one of the
ingredients that makes multithreaded programming so error-prone.  There
is really no reason to want concurrency and parallelism to be mushed
together into a single language feature.  You need concurrency for
interaction and parallelism for speeding up CPU-bound algorithms.  There
is no reason to put the two in the same bucket.</p>

<h4>Q04: Fine.  Be a pedantic jerk.  Parallelism and concurrency are
different.  I care about parallelism; what are you going to do for
me?</h4>

<p>For the time being, Charcoal doesn't have anything novel to offer on
the parallelism front.  My basic philosophy re: parallelism is that it
should be done with processes instead of threads.  Not sharing memory by
default is a good way to avoid concurrency bugs.  Maybe I'll work more
on this problem elsewhere.</p>

<h4>Q05: The world has existed happily with event handlers, threads,
etc. for decades now; did anything change to create a demand for a new
concurrency primitive?</h4>

<p>I'm not sure.  My sense is that with so much of the application
development action going on in the web, mobile and embedded parts of the
universe, there is genuinely more reactivity and interactivity compared
to a decade or two ago.  As circumstantial evidence, I point to all the
experimentation with funky event handling and cooperative threading
models going on in the "scripting" language space.  In particular, my
sense is that "regular joe" developers are coming face-to-face with
concurrency issues much more than in the past.  I would love to know if
anyone has actually studied these trends.</p>

<div><hr/>&copy; Copyright 2013 <a href="http://www.cs.cornell.edu/~biy3/">Benjamin Ylvisaker</a></div>

</div>
</body>
</html>
