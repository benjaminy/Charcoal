"use strict";

/*
 * Header Comment
 */

var P = Promise;

/* TODO: Better assert */
function assert( condition, message )
{
    if( condition )
        return;
    throw new Error( message );
}

function makeUniqueId( ids, min, max )
{
    var id = undefined;
    var found = false;
    if( !max )
    {
        max = 100;
    }
    if( !min )
    {
        min = 0;
    }
    while( !found )
    {
        id = Math.floor( Math.random() * ( max - min ) ) + min;
        if( !( id in ids ) )
            found = true;
    }
    return id;
}

var activity_state = Object.freeze( {
    RUNNING         : 1,
    WAITING         : 2,
    RESOLVING       : 3,
    GENERATOR_ERROR : 4,
    FINISHED        : 5,
} );

/* Function for defining "interruptible functions"
 * intFn can be called in two ways:
 *  - with just a generator function
 *  - an activity context, then a generator function
 * The former creates a function that can be invoked in different contexts and passes
 * an activity context reference to the generator function.
 * The latter creates a function that can only be invoked in the given context, and
 * does _not_ pass that context on to the generator function.
 * The latter is primarily for internal use (with "atomic"), but can be used by client
 * code.
 */
function intFn( ...intFn_params )
{
    // console.log( "intFn", intFn_params )
    assert( intFn_params.length > 0 && intFn_params.length < 3, 'XXX Replace Me' );
    if( intFn_params.length === 1 )
    {
        var generator_function = intFn_params[ 0 ];
        var actx_maybe         = null;
    }
    else
    {
        var generator_function = intFn_params[ 1 ];
        var actx_maybe         = intFn_params[ 0 ];
    }

    /* runToNextYield is the heart of the interruptible function
     * implementation.  It gets called after every yield performed by
     * 'generator_function'.
     */
    function runToNextYield( actx, generator, is_err, yielded_value )
    {
        /* Parameter Types: */
        /* actx          : activity context type */
        /* generator     : generator type */
        /* is_err        : boolean */
        /* yielded_value : any */
        // console.log( "runToNextYield", generator );

        assert( actx.continuation === null );

        var scheduler = actx.scheduler;

        if( scheduler.inAtomicMode() && !scheduler.inAtomicMode( actx ) )
        {
            /* Must suspend self */
            /* assert( actx not in scheduler.waiting_activities ); */
            actx.state = activity_state.WAITING;
            actx.waits++;
            actx.queue_len = scheduler.waiting_activities.length;
            scheduler.waiting_activities.push( actx );
            return new Promise( function( resolve, reject ) {
                /* XXX: I hope this function is called immediately by the Promise constructor.
                 *      If not, maybe there is a race with leaving atomic mode. */
                actx.continuation = resolve;
            } ).then(
                function() {
                    return runToNextYield(
                        actx, generator, is_err, yielded_value );
                } );
        }

        /* Either the system is not in atomic mode, or actx is in atomic mode */
        actx.state = activity_state.RUNNING;
        actx.waits = 0;
        try {
            if( is_err )
                var next_yielded = generator.throw( yielded_value );
            else
                var next_yielded = generator.next( yielded_value );
            actx.state = activity_state.RESOLVING;
        }
        catch( err ) {
            console.log( "!!! generator error", err );
            console.log( "!!! generator error", err.stack );
            actx.state = activity_state.GENERATOR_ERROR;
            return P.reject( err );
        }
        /* next_yielded : { done : boolean, value : `b } */

        function realReturn( v )
        {
            assert( actx.generator_fns.length > 0 );
            var g = actx.generator_fns.pop();
            assert( g === generator_function );
            return P.resolve( v );
        }

        if( next_yielded.done )
            return realReturn( next_yielded.value );
        /* "else": The generator yielded; it didn't return */

        return P.resolve( next_yielded.value ).then(
            function( next_yielded_value ) {
                try {
                    if( 'ACTIVITIES_JS_RETURN_FROM_ATOMIC' in next_yielded_value
                        && !( next_yielded_value.value === undefined ) )
                    {
                        return realReturn( next_yielded_value.value );
                    }
                }
                catch( err ) {}
                return runToNextYield( actx, generator, false, next_yielded_value );
            },
            function( err ) {
                return runToNextYield( actx, generator, true, err );
            } );
    }


    /* Finally, the actual code that runs when intFn is called */
    function fnEitherMode( pass_actx, actx, ...params )
    {
        /* actx : activity context type */
        try {
            if( pass_actx )
            {
                var generator = generator_function( actx, ...params );
            }
            else
            {
                var generator = generator_function( ...params );
            }
            /* generator : iterator type */
        }
        catch( err ) {
            console.log( "Error starting generator" );
            return P.reject( err );
        }
        actx.generator_fns.push( generator_function );
        /* NOTE: leaving the value parameter out of the following call,
         * because the first call to 'next' on a generator doesn't expect
         * a real value. */
        return runToNextYield( actx, generator, false );
    }

    if( actx_maybe )
    {
        var f = function( ...params )
        {
            return fnEitherMode( false, actx_maybe, ...params );
        }
        f.ACTIVITIES_JS_EXPECTS_CTX = false;
    }
    else
    {
        var f = function( ...params )
        {
            /* assert( params[ 0 ] is an activity context ) */
            return fnEitherMode( true, ...params );
        }
        f.ACTIVITIES_JS_EXPECTS_CTX = true;
    }
    f.ACTIVITIES_JS_TOKEN = {};
    return f;
}


class ActivityContext
{
    constructor( ctx ) {
        this.continuation  = null;
        this.waits         = 0;
        this.generator_fns = [];
        if( ctx )
        {
            this.scheduler = ctx.scheduler;
            this.id = makeUniqueId( this.scheduler.activities );
        }
        else
        {
            var scheduler = {}
            scheduler.activities         = {};
            scheduler.num_activities     = 0;
            scheduler.atomic_actx        = null;
            scheduler.waiting_activities = [];
            scheduler.inAtomicMode       =
                function( actx ) {
                    if( actx )
                        return this.atomic_actx === actx;
                    else
                        return !( this.atomic_actx === null );
                }

            this.scheduler = scheduler;
            this.id        = 0;
        }
        this.scheduler.activities[ this.id ] = this;
        this.scheduler.num_activities++;
    }

    atomic( ...params_plus_fn )
    {
        var this_actx = this;
        var params = params_plus_fn.slice( 0, params_plus_fn.length - 1 );
        var fn = params_plus_fn[ params_plus_fn.length - 1 ];
        /* fn : ActFn | generator function */
        if( !fn.hasOwnProperty( 'ACTIVITIES_JS_TOKEN' ) )
        {
            fn = intFn( this_actx, fn );
        }
        var scheduler = this_actx.scheduler;
        var first_entry = scheduler.atomic_actx === null;
        if( !first_entry )
        {
            assert( this_actx === scheduler.atomic_actx );
        }

        function leaveAtomic()
        {
            // console.log( "leaveAtomic", first_entry );
            assert( scheduler.atomic_actx === this_actx );
            if( !first_entry )
            {
                /* NOTE: nested atomics are effectively ignored */
                return;
            }
            scheduler.atomic_actx = null;
            scheduler.waiting_activities.sort( function( a, b ) {
                var diff = b.waits - a.waits;
                if( diff == 0 )
                    return a.queue_len - b.queue_len;
                else
                    return diff;
            } );
            var waiting_activities = scheduler.waiting_activities;
            scheduler.waiting_activities = [];
            /* TODO: Consider the pattern where the first activity
             * enters atomic mode immediately, so all the others
             * immediately go back to waiting.  This code is inefficient
             * for the pattern.  But it seems unlikely to happen much. */
            for( var i = 0; i < waiting_activities.length; i++ )
            {
                var wactx = waiting_activities[ i ];
                var cont = wactx.continuation;
                wactx.continuation = null;
                cont();
            }
        }

        scheduler.atomic_actx = this_actx;
        try {
            if( fn.ACTIVITIES_JS_EXPECTS_CTX )
                var p = fn( this, ...params );
            else
                var p = fn( ...params );
        }
        catch( err ) {
            leaveAtomic();
            return P.reject( err );
        }

        return p.then(
            function( val ) {
                leaveAtomic();
                var rv = { value : val,
                           ACTIVITIES_JS_RETURN_FROM_ATOMIC : true };
                return P.resolve( val );
            },
            function( err ) {
                leaveAtomic();
                return P.reject( err );
            } );
    }

    activate( ...params_plus_f )
    {
        // console.log( "activate" );
        var scheduler = this.scheduler;
        var params = params_plus_f.slice( 0, params_plus_f.length - 1 );
        var fn = params_plus_f[ params_plus_f.length - 1 ];
        // fn should either be a generator or an act fun
        // fn must expect actx as its first parameter
        if( !fn.hasOwnProperty( 'ACTIVITIES_JS_TOKEN' ) )
        {
            fn = intFn( fn );
        }
        var actx_child = new ActivityContext( this );
        actx_child.state = activity_state.RUNNABLE;
        actx_child.finished_promise =
            fn( actx_child, ...params ).then(
                function( rv ) {
                    actx_child.state = activity_state.FINISHED;
                    assert( actx_child.id in scheduler.activities );
                    delete scheduler.activities[ actx_child.id ];
                    scheduler.num_activities--;
                    assert( Object.keys( scheduler.activities ).length ==
                            scheduler.num_activities )
                    console.log( "DONE", scheduler.num_activities );
                    return P.resolve( rv );
                } );
        return actx_child;
    }

    log( ...params )
    {
        assert( this.generator_fns.length > 0 );
        var gen_fn = this.generator_fns[ this.generator_fns.length - 1 ];
        // XXX bracket??? console.log( this.id, names.bracket, ...params );
        console.log( this.id, gen_fn.name, ...params );
    }
}

/* scribbling */

function sleep( ms )
{
    return new Promise( resolve => setTimeout( resolve, ms ) );
}

var TC = 10;

var f = intFn( function*f( actx, letter ) {
    for( var j = 0; j < 3; j++ ) {
        yield sleep( 5 * TC );
        actx.log( letter );
        yield actx.atomic( function*() {
            for( var i = 0; i < 5; i++ ) {
                yield sleep( 2 * TC );
                actx.log( letter, "*" );
            }
        } );
    }
} );

/*
function^ f( letter ) {
    for( i = 0; i < 3; i++ ) {
        sleep( 500 );
        log( letter );
        atomic {
            for( var j = 0; j < 5; j++ ) {
                sleep( 200 );
                log( letter + "*" );
            }
        }
    }
}
*/

var ctx = new ActivityContext()
ctx.activate( "A", f );
ctx.activate( "B", f );
ctx.activate( "C", f );
ctx.activate( "D", f );
