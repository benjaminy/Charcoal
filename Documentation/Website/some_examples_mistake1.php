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

<p>What would happen if we just activate the DNS lookup without adding
the extra synchronization?</p>

<?php format_code(
'<i>void</i> <b>multi_dns_conc</b>(
    <i>size_t</i> <b>N</b>, <i>char</i> **<b>names</b>, <b><u>struct</u></b> <i>addrinfo</i> **<b>infos</b> )
{
    <i>size_t</i> <b>i</b>
    <b><u>for</u></b>( i = 0; i &lt; N; ++i )
    {
        <b><u>activate</u></b> ( i )
        {
            assert( 0 == getaddrinfo(
                names[i], NULL, NULL, &amp;infos[i] ) );
        }
    }
}' ); ?>

<p>In the above code there is no way to know when all the DNS lookups
are done.  Using a semaphore is not the only way to do this
synchronization.  Alternatively, the "main" activity could explicitly
wait for all of the lookup activities to finish.  But this code is
definitely buggy.  The application can never be sure that the lookups
are done.</p>

</div>
</body>
</html>
