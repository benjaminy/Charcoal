<html>
<head>
<title>Charcoal Example: Data Structure Libraries</title>
<link rel="stylesheet" media="screen" type="text/css" href="charcoal.css"/>
</head>
<body style="background-color:darkgray">

<div class="side_links">
<a href="index.html">Charcoal</a><br/>
- <a href="short_version.html">Why Charcoal?</a><br/>
- <a href="some_examples.html">Examples</a><br/>
&mdash; <a href="example_multi_dns.html">Multi-DNS</a><br/>
&mdash; <a href="example_signal_handling.html">Signals</a><br/>
&mdash; <a href="example_tight_loops.html">Loops</a><br/>
&mdash; <a href="example_data_structure.html">Data structures</a><br/>
&mdash; <a href="example_lazy_initialization.html">Singleton</a><br/>
&mdash; <a href="example_asynch_exceptions.html">Asynchronous</a><br/>
- <a href="concurrency.html">Concurrency</a><br/>
- <a href="big_four.html">vs. Threads, etc.</a><br/>
- <a href="implementation.html">Implementation</a><br/>
- <a href="faq.html">FAQ</a>
</div>

<div class="main_div">

<h1>Data Structure Libraries</h1>

<p>In this example we look at how to write an "activity safe" data
structure library.  Activity safe means that a structure can be accessed
concurrently from multiple activities without anything breaking too
badly.  Data structure libraries are interesting because they have to
work well enough for many different potential use cases.</p>

<p>Operations that are guaranteed to run in a modest amount of time can
simply be marked unyielding:</p>

<div class="highlight mono">
<b><u>unyielding</u></b> <i>tree_t *</i><b>tree_insert</b>( tree_t *, void *val );
<b><u>unyielding</u></b> <i>tree_t *</i><b>tree_delete</b>( tree_t *, void *val );
</div>

<p>It's interesting to think about what this does to the implementations
a little bit.</p>

<p>Operations that might run for a long time are a little harder to deal
with.</p>

tree_t *tree_fold( void *(*f)(tree_t *, void *), tree_t *t, void *val )
{
    if( t->left )
        val = tree_fold( f, t->left, val );
    val = f( t, val );
    if( t->right )
        val = tree_fold( f, t->right, val );
    return val;
}

tree_t *tree_fold( void *(*f)(tree_t *, void *), tree_t *t, void *val )
{
    if( t->left )
        val = call_no_yield tree_fold( f, t->left, val );
    yield;
    val = f( t, val );
    if( t->right )
        val = call_no_yield tree_fold( f, t->right, val );
    return val;
}

tree_t *tree_fold( void *(*f)(tree_t *, void *), tree_t *t, void *val )
{
    if( t->left )
        val = call_no_yield tree_fold( f, t->left, val );
    if( yield )
    {
        /* consistency check or something */
    }
    val = f( t, val );
    if( t->right )
        val = call_no_yield tree_fold( f, t->right, val );
    return val;
}

tree_t *tree_fold_helper( void *(*f)(tree_t *, void *), tree_t *t, void *val, size_t *i )
{
    if( t->left )
        val = call_no_yield tree_fold( f, t->left, val );
    ++( *i );
    if( 0 == ( *i ) % XXX )
        yield;
    val = f( t, val );
    if( t->right )
        val = call_no_yield tree_fold( f, t->right, val );
    return val;
}

void *tree_fold( void *(*f)(tree_t *, void *), tree_t *t, void *val )
{
    size_t i = 0;
    return tree_fold_helper( f, t, val, &i );
}



for( i = 0; i < N; ++i )
{
    (*f)( ... );
}

for( i = 0; i < N; )
{
    for_no_yield( ; i < N && i % 32 != 0; ++i )
    {
        (*f)( ... );
    }
}

void foo( void )
{
    yield;
    for_no_yield( i = 0; i < 32; ++i )
    {
        (*f)( ... );
    }
    yield;
}

<div><hr/>&copy; Copyright 2013 <a href="http://www.cs.cornell.edu/~biy3/">Benjamin Ylvisaker</a></div>

</div>
</body>
</html>
