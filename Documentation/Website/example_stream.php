<html>
<head>
<title>Giving a Library a Streaming Interface</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css">
<style media="screen" type="text/css">
</style>
</head>
<body style="background-color:darkgray">

<?php include 'code_examples.php'; ?>

<?php include 'side_bar_examples.html'; ?>

<div class="main_div">

<h1>Giving a Library a Streaming Interface</h1>

<p>"Stream programming" means different things to different people.
What I mean is the generalization of classic UNIX pipes to arbitrary
stream graphs.  Each component receives from some channels and sends to
others.</p>

<p>Simple example: Parsing a large file</p>

<p>Note: Unlike the other examples on this page, this one doesn't
actually work.  The problem is that Charcoal inherited a lack of
exceptions from C.  If one were to implement activities in a language
with exception handling, this example would be relevant.  Read on if
that sounds interesting to you.</p>

<p>An asynchronous exception is one that is thrown (or raised) by some
entity other than the currently executing code.  One of the classic use
cases for asynchronous exceptions is an algorithm with multiple
implementation approaches with very different performance profiles.
Different approaches perform dramatically better or worse based on some
hard-to-predict characteristic of the input data.</p>

<p>If we have asynchronous exceptions, we can try all approaches
concurrently.  When one of them finishes, it kills the other ones.
Here's a sketch of how it would work in Charcoal (if it had exception
handling with Java-ish syntax).</p>

<?php format_code(
'<i>void</i> <b>algo_X</b>( <b>input data</b> )
{
    <i>activity_t</i> <b>a1</b>, <b>a2</b>;
    a1 = <b><u>activate</u></b>
    {
        <b><u>try</u></b>
        {
            algo_X_approach_1( input data );
            <b><u>deliver</u></b> a2 terminate_thyself;
        }
        <b><u>catch</u></b>( terminate_thyself ) { }
        /* clean up approach 1 */
    }
    a2 = <b><u>activate</u></b>
    {
        <b><u>try</u></b>
        {
            algo_X_approach_2( input data );
            <b><u>deliver</u></b> a1 terminate_thyself;
        }
        <b><u>catch</u></b>( terminate_thyself ) { }
        /* clean up approach 2 */
    }
    join( a1 );
    join( a2 );
}' ); ?>

<p>Quick side note: Remember, activities do not run in parallel, so the
goal here is not to benefit from processor parallelism.  Rather we
should expect this code to take about twice as much time as the better
of the two approaches on any given input.  If the two approaches have
sufficiently different performance profiles, this is a big win in terms
of worst-case performance, relative to simply doing one or the
other.</p>

<p>The first important thing to say about asynchronous exceptions (or
their cousin, thread cancelation) is that it is a sufficiently tempting
feature that many multithreading APIs include it in version 1.  However,
it is sufficiently problematic to implement correctly that most API
documentation strongly discourages its use, and many APIs/languages have
explicitly deprecated it.  The one notable exception [sorry] I am aware
of
is <a href="http://dl.acm.org/citation.cfm?id=378858">Haskell</a>.</p>

<p>Why is the implementation of asynchronous exceptions so hairy?  I
encourage you to look at the paper linked above; it's a good one.  The
short story is that compilers and processors can and do reorder
expressions and statements all the time for the purpose of optimization.
As a result, if one interrupts a running thread at any given time there
may be no valid high-level program state that accurately reflects the
low-level state.  So where should the exception appear that it was
thrown from?  Threads can stay in such "ambiguous" states for
arbitrarily long periods of time.</p>

<p>With activities (and really anything in the cooperative thread
family), we can actually implement asynchronous exceptions sensibly.
When one activity delivers an exception to another activity, the
exception will appear to be raised by the next yield expression that the
receiver activity evaluates.</p>

<p>(By the way, I
used <span class="mono">deliver</span> instead of
<span class="mono">throw</span> because throwing and
delivering exceptions are wildly different.  Throwing changes the
control flow of the current activity to the relevant handler.
Delivering affects some other activity, and the current activity goes
along its merry way.)</p>

<h3>We Can, But Should We?</h3>

<p>So delivering asynchronous exceptions between activities is not the
train wreck that it is in the context of threads, but is it worth
actually offering as a language feature?  This is a tricky question.
The main problem is that asynchronous exceptions can arrive at any time,
so developers have to be extremely cautious to avoid getting interrupted
in the middle of a delicate sequence of operations.  I think activities
have a fighting chance
because <span class="mono">unyielding</span> can be
used to block asynchronous exceptions for regions of code.  However, I
think more research is needed on this topic.</p>

<p>Even "synchronous" (i.e. normal) exception handling has its skeptics
(<a href="http://www.joelonsoftware.com/items/2003/10/13.html">1</a>, 
<a href="http://blogs.msdn.com/b/oldnewthing/archive/2004/04/22/118161.aspx">2</a>, 
<a href="http://blogs.msdn.com/b/larryosterman/archive/2004/09/10/228068.aspx">3</a>, 
<a href="http://silkandspinach.net/2005/06/14/exceptions-considered-harmful/">4</a>, 
<a href="http://www.lighterra.com/papers/exceptionsharmful/">5</a>, 
<a href="http://www.ckwop.me.uk/Why-Exceptions-Suck.html">6</a>).  The
big-picture issue here is that non-local control transfers always carry
a cost in terms of code understandability (and thus maintainability and
robustness).  My own current rule of thumb is that exception handling
should mostly be restricted to weird near-catastrophic situations, not
"normal" error conditions.</p>

<p>For asynchronous exception handling, my intuition is that it has some
legitimate use cases, but it should be used <em>only</em> when there is
some huge win relative to alternative patterns.</p>

<p>This is on the back burner for the Charcoal project, because C
doesn't have exception handling, but I am very interested in hearing
other peoples' thoughts on the topic.</p>

<?php include 'copyright.html'; ?>

</div>
</body>
</html>
