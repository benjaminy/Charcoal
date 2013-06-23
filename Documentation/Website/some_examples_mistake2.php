<html>
<head>
<title>Some Examples</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<?php include 'code_examples.php'; ?>

<?php include 'side_bar_examples.html'; ?>

<div class="main_div">

<p>What if we leave <span class="mono">i</span> off the by-value
variables list?</p>

<?php format_code(
'<i>void</i> <b>multi_dns_conc</b>(
    <i>size_t</i> <b>N</b>, <i>char</i> **<b>names</b>, <b><u>struct</u></b> <i>addrinfo</i> **<b>infos</b> )
{
    <i>size_t</i> <b>i</b>, <b>done</b> = 0;
    <i>semaphore_t</i> <b>done_sem</b>;
    sem_init( &amp;done_sem, 0 );
    <b><u>for</u></b>( i = 0; i &lt; N; ++i )
    {
        <b><u>activate</u></b> <span class="yellow">&nbsp;&nbsp;&nbsp;&nbsp;</span>
        {
            assert( 0 == getaddrinfo(
                names[i], NULL, NULL, &amp;infos[i] ) );
            <b><u>if</u></b>( ( ++done ) == N )
                sem_inc( &amp;done_sem );
        }
    }
    sem_dec( &amp;done_sem );
}' ); ?>

<p>This is a pretty nasty little bug, because the code will probably
work most of the time.  The potential problem is the following
sequence:</p>

<ol>
<li>One of the lookup activities yields
<li>The "main" activity increments <span class="mono">i</span>
<li>The lookup activity resumes and now has the wrong value
for <span class="mono">i</span>
</ol>

<p>This kind of bug deserves some more thinking.  Maybe some automatic
warnings could happen.</p>

</div>
</body>
</html>
