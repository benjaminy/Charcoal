<html>
<head>
<title>Concurrency</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<div class="side_links">
<a href="index.html">Charcoal</a><br/>
- <a href="short_version.html">Why Charcoal?</a><br/>
- <a href="some_examples.html">Examples</a><br/>
- <a href="concurrency.html">Concurrency</a><br/>
&mdash; <a href="concurrency.html#expressiveness">Expressiveness</a><br/>
&mdash; <a href="concurrency.html#performance">Performance</a><br/>
- <a href="big_four.html">vs. Threads, etc.</a><br/>
- <a href="implementation.html">Implementation</a><br/>
- <a href="faq.html">FAQ</a>
</div>

<div class="main_div">

<h1>Concurrency</h1>

<p>Concurrency is a big topic.  In the most zoomed-out view, concurrent
software has multiple "active" components &mdash; sometimes
called <em>processes</em> or <em>tasks</em> &mdash; that can make
progress simultaneously.  The simultaneity can be physical (different
tasks executing on different processors) or simulated by interleaving
the execution of multiple tasks on a single processor.  This page is my
take on some of the most important concepts that fit under the
concurrency umbrella.</p>

<p>The reasons for making concurrent software can be lumped into two
main categories: performance and expressiveness.</p>

<ul>
<li><em>Performance</em> - There are multiple hardware resources of some
sort (processors, network interfaces, machines, ...) and we want our
software to use them simultaneously to improve some performance metric
(total run time, throughput, latency, whatever).
<li><em>Expressiveness</em> - Some program patterns are intrisically
concurrent (interactive programming <em>is</em> concurrenct
programming), and some programs can be more elegantly expressed by using
the right concurrency framework.
</ul>

<p>Of course a single application might use concurrency in multiple ways
for different reasons, but I think it's important to keep in mind that
these are two categories are quite distinct.  The Charcoal project is
principally focused on the expressiveness part of the picture.</p>

<div class="highlight">
<table width="100%">
<caption align="bottom"><hr/>Figure 1: A map of the primary reasons for
using concurrency in software.  The check marks indicate the concepts
that the Charcoal project most directly addresses.</caption>
<tbody><tr><td>
<img src="./concurrency_concept_map.svg" width="100%"/>
</td></tr></tbody>
</table>
</div>

<div class="hblock2">
<a id="expressiveness"/>
<h2>Expressiveness</h2>

<p>Expressiveness is something of a catch-all in the sense that we mean
"all the reasons for using concurrency that aren't directly related to
performance".  We put three primary concepts within the scope of
expressiveness:</p>

<ul>
<li>Reactive/interactive
<li>Algorithmic
<li>Isolation/independence
</ul>

<div class="hblock3">
<a id="reactive"/>
<h3>Reactive/Interactive</h3>

<p>All software interacts with the real world in some way.  Some
applications have a relatively simple interactions pattern: read some
input, compute for a while, write some output.  Compilers are a good
example of software that is quite complex, but mostly fits this simple
interaction pattern.</p>

<p>Many applications, including most that people directly use on a
regular basis, have more complex interactions with the world.  Take a
spreadsheet as an example.  The application must be ready to react to
keyboard and mouse input from the user at any time.  Recalculating the
spreadsheet values may take a relatively long time for large documents,
and this process should be interruptable.  Also various background tasks
are probably happening periodically, like auto-saving backup copies and
checking formulas for possible errors.</p>

<p>Building reactive/interactive software is different.</p>
</div>
<br/>
<div class="hblock3">
<a id="algorithmic"/>
<h3>Algorithmic</h3>

<p>Some algorithms look "procedural/functional" from the outside, but
can still use concurrency to good effect internally.  A classic example
is programming language parsing.  Scanner; parser.</p>
</div>
<br/>
<div class="hblock3">
<a id="isolation"/>
<h3>Isolation</h3>

<p>Sometimes we want software components to be isolated from each other
in one way or another.  A couple of classic examples:</p>

<ul>
<li><em>Plugins</em>.  Many applications have plugins.  Often we want
the core application to be isolated from the plugins, in case the plugin
does something bad.
<li><em>Long-running Tasks</em>.  Often in GUI applications, many tasks
can be taken care of very quickly, but a few might run for a long time.
We need to isolate the long-running tasks in some way to ensure that the
application remains responsive.
</ul>
</div>
</div>

<div class="hblock2">
<a id="performance"/>
<h2>Performance</h2>

<p>Some applications </p>

<div class="hblock3">
<a id="processor"/>
<h3>Processor Parallelism</h3>

<p>For CPU-bound tasks.</p>
</div>
<br/>
<div class="hblock3">
<a id="system"/>
<h3>System Parallelism</h3>

<p>For I/O-bound tasks.</p>
</div>
</div>
<div class="hblock2">
<h2>Miscellaneous</h2>

<p>Here we have a grab-bag of concepts that are either combinations of
the ideas discussed above or more specitic instances of them.  They are
kinda sorta in order starting with the most relevant to Charcoal.</p>

<div class="hblock3">
<a id="disk"/>
<h3>Disk/Network/UI</h3>

<p>One common and simple concurrency pattern in GUI applications is
starting long-running I/O operations, like disk or network reads,
while remaining responsive to user input.</p>

<p>This category is also connected to the concurrency part of the map,
because some software has high disk or network requirements and needs to
overlap many accesses in order to perform well.</p>
</div>
<br/>
<div class="hblock3">
<a id="distributed"/>
<h3>Distributed</h3>

<p>Clusters.  Could be CPU-bound or IO/bound.</p>
</div>
<br/>
<div class="hblock3">
<a id="realtime"/>
<h3>Real-time</h3>

<p>A special case of interactive software is those that have real-time
constraints (hard or soft).  This category includes multimedia software,
games, embedded systems controllers.</p>

</div>
<br/>
<div class="hblock3">
<a id="multi"/>
<h3>Multi-processors</h3>

<p>Multi-cores and such.</p>

</div>
<br/>
<div class="hblock3">
<a id="gpu"/>
<h3>Accelerators (GPU/FPGA)</h3>

<p>Turbo boost.</p>
</div>
</div>
<?php include 'copyright.html'; ?>

</div>
</body>
</html>
