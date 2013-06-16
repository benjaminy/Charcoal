<html>
<head>
<title>Some Examples</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<?php include 'code_examples.php'; ?>

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

<p>What if we add something like <span class="mono">done</span> to the
by-value variables list?</p>

<?php format_code(
'<i>void</i> <b>multi_dns_conc</b>(
    <i>size_t</i> <b>N</b>, <i>char</i> **<b>names</b>, <b><u>struct</u></b> <i>addrinfo</i> **<b>infos</b> )
{
    <i>size_t</i> <b>i</b>, <b>done</b> = 0;
    <i>semaphore_t</i> <b>done_sem</b>;
    sem_init( &amp;done_sem, 0 );
    <b><u>for</u></b>( i = 0; i &lt; N; ++i )
    {
        <b><u>activate</u></b> ( i, <span class="yellow">done</span> ) 
        {
            assert( 0 == getaddrinfo(
                names[i], NULL, NULL, &amp;infos[i] ) );
            <b><u>if</u></b>( ( ++done ) == N )
                sem_inc( &amp;done_sem );
        }
    }
    sem_dec( &amp;done_sem );
}' ); ?>

<p>This should be a relatively easy bug to find and fix.  All of the
lookup activities will get their own copy
of <span class="mono">done</span> (each of which will be initialized to
0).  None of the activities will perform the semaphore increment on line
14, so the main activity will be stuck forever.</p>

</div>
</body>
</html>
