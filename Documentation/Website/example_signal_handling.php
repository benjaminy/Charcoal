<html>
<head>
<title>Charcoal Example: Signal Handling</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<?php include 'code_examples.php'; ?>

<?php include 'side_bar_examples.html'; ?>

<div class="main_div">

<h1>Signal Handling</h1>

<p>Under the hood there isn't much that's different about signal
handling in Charcoal, compared to C.  However, there is a pattern that
works out much nicer with activities than with threads.</p>

<p>First, I want to point out that signal handling in C plays two very
different roles that are smooshed together into one feature.
The conventional names for the roles are synchronous and asynchronous,
but I don't think those names sufficiently emphasize how different the
two roles are.
I think better names are "low-level exception handling" and "reacting to
the outside world".</p>

<p>Synchronous signals are delivered in direct response to something
that the executing process did (divide by zero, execute an invalid
instruction, access an illegal memory location, etc).
In most modern programming languages these kinds of actions would raise
an exception (or "throw" it depending on your preferred exception
terminology).
In C you get a signal delivered.</p>

<p>Asynchronous signals are delivered because of something external to
the executing process (keyboard press, network packet arrival, timer
expiration, etc).
Asynchronous signals can be delivered at essentially any time, which
makes them extremely dangerous.
The official rules about what exception handlers are allowed to do are
extremely restrictive, because if they do anything moderately
interesting terrible breakage is likely to ensue.
This example is about asynchronous signal handling.
There's not much interesting to say about the synchronous side.</p>

<p>Here's a true story from the code mines.  I once worked on a project
that would experience deadlocks once in a blue moon with debug builds
only.  It turns out that the problem was:</p>

<ul>
<li>We were using a version of malloc that (in debug mode) acquired a
mutex and recorded some statistics in a shared data structure.
<li>One of our signal handlers called malloc (indirectly, of course),
which signal handlers are strictly forbidden from doing.
</ul>

<p>So if a signal happened to arrive right in the middle of a malloc
call, the handler would deadlock on trying to acquire the mutex.  We
fixed this by reorganizing the handler to not call malloc, but that
required some unpleasant contortions.</p>

<p>In multithreaded applications, you can do something more pleasant for
asynchronous signals, which is have some thread block waiting for a
signal to arrive.  This looks like the following:</p>

<?php format_code(
'<i>int</i> <b>sig_hand_thread_entry</b>( <i>void *</i><b>args</b> )
{
    <i>int</i> <b>sig</b>;
    <i>sigset_t</i> <b>sigset</b>;
    /* initialize sigset */
    <b><u>while</u></b>( true )
    {
        <b><u>if</u></b>( 0 != sigwait( &amp;sigset, &amp;sig ) )
        { /* handle error */ }
        <b><u>switch</u></b>( sig )
        { /* handle signal */ }
    }
}

<i>void</i> <b>start_signal_handler</b>( <i>thrd_t *</i>thr )
{
    thrd_t _t;
    if( !thr ) thr = &amp;_t;
    int err_code = thrd_create(
        thr, sig_hand_thread_entry, NULL );
}'); ?>

<p>Using this pattern would have taken care of our deadlock in malloc
problem, but as I argue elsewhere in these pages I am not generally
enthusiastic about using threads for much of anything.  You can
implement the exact same pattern with activities:</p>

<?php format_code(
'<i>activity_t</i> <b>start_signal_handler</b>( <i>void</i> )
{
    <b><u>return</u></b> <b><u>activate</u></b>
    {
        <i>int</i> <b>sig</b>;
        <i>sigset_t</i> <b>sigset</b>;
        /* initialize sigset */
        <b><u>while</u></b>( true )
        {
            <b><u>if</u></b>( 0 != sigwait( &amp;sigset, &amp;sig ) )
            { /* handle error */ }
            <b><u>switch</u></b>( sig )
            { /* handle signal */ }
        }
    }
}' ); ?>

<p>Beyond the syntactic differences, there is one substantive difference
between using threads and activites to implement the blocking signal
handler pattern.  With threads, the delivery of a signal might interrupt
whatever thread is running to context switch over to the thread that
is <span class="mono">sigwait</span>ing.  This kind of rude interruption
is what makes multithreaded programming hard to get right.</p>

<p>With activities the delivery of a signal that is being waited on
causes a flag to be set such that the next time the running activity
yields, the program will context switch to the waiting activity.  On the
plus side, this gentler approach to interruption makes it easier to
avoid concurrency bugs.  On the minus side, a modest amount of time
might expire before the current activity yields.  This will add some
latency to the handling of the signal.  Well-behaved Charcoal code
should be yielding at least once every millisecond, so this latency
shouldn't be a killer for most applications.  If you're doing something
more real-time, a millisecond may be far too long to wait.  I'll address
that situation in a different example.</p>

<?php include 'copyright.html'; ?>

</div>
</body>
</html>
