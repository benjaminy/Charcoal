<html>
<head>
<title>Charcoal FAQ</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
<style>
div.qanda {
  background-color:lightyellow;
}
h4.question {
  background-color:lightblue;
}
</style>  
</head>
<body style="background-color:darkgray">

<?php include 'side_bar_basic.html'; ?>

<div class="main_div">

<h1>"Frequently" Asked Questions</h1>
<h3>a.k.a. questions that the authors feel like answering</h3>
<br/>
<br/>

<div class="qanda">
<h4 class="question">Q01: Why design another programming language?</h4>

<p>It's true that there are too many programming languages in the world
already.  However, concurrency is one of the facets of programming
language design that really can't be done in any other way.  See, e.g., 
<a href="http://www.hpl.hp.com/techreports/2004/HPL-2004-209.html">Threads
Cannot be Implemented as a Library</a>.</p>
</div>

<div class="qanda">
<h4 class="question">Q02: Why start with C?</h4>

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
</div>

<div class="qanda">
<a id="parallel">
<h4 class="question">Q03: Isn't it dumb to make a concurrent language that can't be run
in parallel?</h4>

<p>As discussed elsewhere on this site, concurrency and parallelism
are <em>very</em> different topics.  Part of the inspiration behind this
project is that insisting on parallel execution is one of the
ingredients that makes multithreaded programming so error-prone.  There
is really no reason to want concurrency and parallelism to be mushed
together into a single language feature.  You need concurrency for
interaction and parallelism for speeding up CPU-bound algorithms.  There
is no reason to put the two in the same bucket.</p>
</div>

<div class="qanda">
<h4 class="question">Q04: Fine.  Be a pedantic jerk.  Parallelism and concurrency are
different.  I care about parallelism; what are you going to do for
me?</h4>

<p>For the time being, Charcoal doesn't have anything novel to offer on
the parallelism front.  My basic philosophy re: parallelism is that it
should be done with processes instead of threads.  Not sharing memory by
default is a good way to avoid concurrency bugs.  Maybe I'll work more
on this problem elsewhere.</p>
</div>

<div class="qanda">
<h4 class="question">Q05: The world has existed happily with event handlers, threads,
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
</div>

<a id="github">

<div class="qanda">
<h4 class="question">Q06: Is there code available for this project?</h4>

<p>Yes.  My github name
is <span style="font-family:monospace;">benjaminy</span>.  Search for
that and you should find the Charcoal repo.  Be aware that the project
is still is a very early and incomplete state.</p>
</div>

<?php include 'copyright.html'; ?>

</div>
</body>
</html>
