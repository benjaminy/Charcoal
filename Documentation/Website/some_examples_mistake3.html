<html>
<head>
<title>Some Examples</title>
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

<p>What if we add something like <span class="mono">done</span> to the
by-value variables list?</p>

<div class="highlight mono">
<table><tbody><tr>
<td align="right" valign="top">
1:<br/>2:<br/>3:<br/>4:<br/>5:<br/>6:<br/>7:<br/>8:<br/>9:<br/>10:<br/>
11:<br/>12:<br/>13:<br/>14:<br/>15:<br/>16:<br/>17:<br/>18:</td>
<td>&nbsp;</td>
<td valign="top">
<i>void</i> <b>multi_dns_conc</b>(<br/>
<pre>    </pre><i>size_t</i> <b>N</b>, <i>char</i> **<b>names</b>, <b><u>struct</u></b> <i>addrinfo</i> **<b>infos</b> )<br/>
{<br/>
<pre>    </pre><i>size_t</i> <b>i</b>, <b>done</b> = 0;<br/>
<pre>    </pre><i>semaphore_t</i> <b>done_sem</b>;<br/>
<pre>    </pre>sem_init( &amp;done_sem, 0 );<br/>
<pre>    </pre><b><u>for</u></b>( i = 0; i &lt; N; ++i )<br/>
<pre>    </pre>{<br/>
<pre>        </pre><b><u>activate</u></b> ( i, <span class="yellow">done</span> ) <br/>
<pre>        </pre>{<br/>
<pre>            </pre>assert( 0 == getaddrinfo(<br/>
<pre>                </pre>names[i], NULL, NULL, &amp;infos[i] ) );<br/>
<pre>            </pre><b><u>if</u></b>( ( ++done ) == N )<br/>
<pre>                </pre>sem_inc( &amp;done_sem );<br/>
<pre>        </pre>}<br/>
<pre>    </pre>}<br/>
<pre>    </pre>sem_dec( &amp;done_sem );<br/>
}
</td>
</tr></tbody></table>
</div>

<p>This should be a relatively easy bug to find and fix.  All of the
lookup activities will get their own copy
of <span class="mono">done</span> (each of which will be initialized to
0).  None of the activities will perform the semaphore increment on line
14, so the main activity will be stuck forever.</p>

</div>
</body>
</html>
