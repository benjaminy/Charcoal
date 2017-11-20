"use strict";

/*
 * Top Matter
 */

var P = Promise;

/* TODO: Better assert */
function assert( condition, message )
{
    if( condition )
        return;
    throw new Error( message );
}

let ACT_FN_TAG           = Symbol( "activity" );
let ACT_STATE_TAG        = Symbol( "activity_state" );
let EXPECTS_CTX_TAG      = Symbol( "expects_ctx" );
let RTRN_FROM_ATOMIC_TAG = Symbol( "rtrn_from_atomic" );
let DO_NOT_PASS_TAG      = Symbol( "do_not_pass" );
let RUNNABLE             = Object.freeze( { t: ACT_STATE_TAG } );
let RUNNING              = Object.freeze( { t: ACT_STATE_TAG } );
let WAITING              = Object.freeze( { t: ACT_STATE_TAG } );
let RESOLVING            = Object.freeze( { t: ACT_STATE_TAG } );
let GENERATOR_ERROR      = Object.freeze( { t: ACT_STATE_TAG } );
let FINISHED             = Object.freeze( { t: ACT_STATE_TAG } );

/* Function for defining "activity functions" (basically a special flavor of async function)
 * actFn can be called in two ways:
 *  - with just a generator function
 *  - an activity context, then a generator function
 * The former creates a function that can be invoked in different contexts and passes
 * an activity context reference to the generator function.
 * The latter creates a function that can only be invoked in the given context, and
 * does _not_ pass that context on to the generator function.
 * The latter is primarily for internal use (with "atomic"), but can be used by client
 * code.
 */
export function actFn( ...actFn_params )
{
    // console.log( "actFn", actFn_params )

    let np = actFn_params.length;

    assert( np > 0, 'actFn called with no parameters.  Requires at least a function*' );
    assert( np < 3, 'actFn called with too many parameters ('+np+').' );

    if( np === 1 )
    {
        let generator_function = actFn_params[ 0 ];
        let actx_maybe         = null;
    }
    else
    {
        let generator_function = actFn_params[ 1 ];
        let actx_maybe         = actFn_params[ 0 ];
        assert( actx_maybe.constructor === Context );
    }
    if( generator_function.hasOwnProperty( ACT_FN_TAG ) )
        return generator_function;


    /* runToNextYield is the heart of the activity function
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

        if( actx.blockedByAtomic() )
        {
            actx.addToWaiting();
            return new Promise( function( resolve, reject ) {
                actx.continuation = resolve;
            } ).then(
                function() {
                    return runToNextYield( actx, generator, is_err, yielded_value );
                } );
        }

        /* Either the system is not in atomic mode, or actx can run in the current atomic */
        actx.state = RUNNING;
        actx.waits = 0;
        try {
            if( is_err )
            {
                let next_yielded = generator.throw( yielded_value );
            }
            else
            {
                let next_yielded = generator.next( yielded_value );
            }
            actx.state = RESOLVING;
        }
        catch( err ) {
            // console.log( "!!! generator error", err, err.stack );
            actx.state = GENERATOR_ERROR;
            // Error.captureStackTrace( err, runToNextYield );
            return P.reject( err );
        }
        /* next_yielded : { done : boolean, value : `b } */

        function realReturn( v )
        {
            assert( actx.generator_fns.length > 0 );
            let g = actx.generator_fns.pop();
            assert( g === generator_function );
            return P.resolve( v );
        }

        if( next_yielded.done )
            return realReturn( next_yielded.value );
        /* "else": The generator yielded; it didn't return */

        return P.resolve( next_yielded.value ).then(
            function( next_yielded_value ) {
                try {
                    if( RTRN_FROM_ATOMIC_TAG in next_yielded_value
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


    /* Finally, the code that actually runs when actFn is called */
    function fnEitherMode( actx, ...params )
    {
        /* actx : activity context type */
        let pass_actx = !actx.hasOwnProperty( DO_NOT_PASS );
        try {
            if( pass_actx )
            {
                let generator = generator_function( actx, ...params );
            }
            else
            {
                let generator = generator_function( ...params );
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
            actx_maybe[ DO_NOT_PASS ] = 0;
            return fnEitherMode( actx_maybe, ...params );
        }
    }
    else
    {
        var f = function( ...params )
        {
            assert( params[ 0 ].constructor === Context );
            return fnEitherMode( ...params );
        }
    }
    f[ EXPECTS_CTX_TAG ] = !actx_maybe;
    f[ ACT_FN_TAG ] = 0;
    return f;
}

class Scheduler
{
    constructor()
    {
        this.activities           = {};
        this.num_activities       = 0;
        this.atomic_stack         = {};
        this.atomic_stack.waiting = new Set();
        this.atomic_stack.next    = null;
    }

    activateInternal( parent, ...params_plus_f )
    {
        /* XXX atomic vs not? */
        // console.log( "activateInternal" );
        var params = params_plus_f.slice( 0, params_plus_f.length - 1 );
        var fn     = actFn( params_plus_f[ params_plus_f.length - 1 ] );
        var child  = new Context( this, parent );

        child.state = RUNNABLE;
        child.finished_promise =
            fn( child, ...params ).then(
                function( rv ) {
                    child.state = FINISHED;
                    assert( child.id in this.activities );
                    delete this.activities[ child.id ];
                    this.num_activities--;
                    assert( Object.keys( this.activities ).length ==
                            this.num_activities )
                    console.log( "DONE", this.num_activities );
                    return P.resolve( rv );
                } );
        return child;
    }

    activate( ...params_plus_f )
    {
        return activateInternal( null, ...params_plus_f );
    }
}

class Context
{
    constructor( scheduler, parent ) {
        this.continuation  = null;
        this.waits         = 0;
        this.generator_fns = [];
        this.scheduler     = scheduler;
        this.id            = Symbol( "activity_id" );
        this.parent        = parent;
        scheduler.activities[ this.id ] = this;
        scheduler.num_activities++;
    }

    addtoWaiting()
    {
        /* assert( actx not in scheduler.waiting_activities ); */
        this.state = WAITING;
        this.waits++;
        this.queue_len = this.scheduler.atomic_stack.waiting.size;
        this.scheduler.waiting.add( this );
    }

    blockedByAtomic()
    {
        if( !this.atomic_stack.next )
            return false;
        return !this.atomic_stack.hasOwnProperty( this.id );
    }

    activate( ...params_plus_f )
    {
        return this.scheduler.activateInternal( this, ...params_plus_f );
    }

    /* Curried version. (Maybe useful in some HOF situations.) */
    activate_c( f )
    {
        return function( ...params ) {
            params.push( f );
            return this.activate( ...params );
        }
    }

    atomic( ...params_plus_fn )
    {
        let params    = params_plus_fn.slice( 0, params_plus_fn.length - 1 );
        let fn        = actFn( this, params_plus_fn[ params_plus_fn.length - 1 ] );
        let scheduler = this.scheduler;

        function leaveAtomic()
        {
            // console.log( "leaveAtomic", first_entry );
            let top = scheduler.atomic_stack;
            assert( top.hasOwnProperty( this.id ) );
            scheduler.atomic_stack = top.next;

            /* TODO: There's a potential scalability bug here, if the
             * number of waiting activities is large and many of them
             * are forced to go back to waiting before making any
             * progress.
             *
             * However, the simplest alternatives seem to have important
             * fairness problems with atomic mode.  That is, how can we
             * guarantee that an activity is not stuck indefinitely
             * while other activities go in and out of atomic mode?
             *
             * For now I'm going to leave it, because I think it would
             * take a really weird program to trigger the scalability
             * bug. */

            top.waiting.sort( function( a, b ) {
                var diff = b.waits - a.waits;
                if( diff == 0 )
                    return a.queue_len - b.queue_len;
                else
                    return diff;
            } );
            for( let actx of top.waiting.values() )
            {
                let cont = actx.continuation;
                actx.continuation = null;
                cont();
            }
        }

        scheduler.atomic_actx = this;
        try {
            if( fn[ EXPECTS_CTX_TAG ] )
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
                var rv = { "value" : val,
                           RTRN_FROM_ATOMIC_TAG : true };
                return P.resolve( val );
            },
            function( err ) {
                leaveAtomic();
                return P.reject( err );
            } );
    }

    log( ...params )
    {
        assert( this.generator_fns.length > 0 );
        var gen_fn = this.generator_fns[ this.generator_fns.length - 1 ];
        // XXX bracket??? console.log( this.id, names.bracket, ...params );
        console.log( this.id, gen_fn.name, ...params );
    }
}

export function makeScheduler( options )
{
    return new Schduler();
}

/* scribbling */

function sleep( ms )
{
    return new Promise( resolve => setTimeout( resolve, ms ) );
}

var TC = 10;

var f = actFn( function*f( actx, letter ) {
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

Error.stackTraceLimit = Infinity;

var throwTest1 = actFn( function*throwTest1( actx ) {
    console.log( "TT1" );
    throw new Error( "Yay" );
} );

var throwTest2 = actFn( function*throwTest2( actx ) {
    console.log( "TT2" );
    return yield throwTest1( actx );
} );

var throwTest3 = actFn( function*throwTest3( actx ) {
    console.log( "TT3" );
    return yield throwTest2( actx );
} );

var throwTest4 = actFn( function*throwTest4( actx ) {
    try {
        var x = yield throwTest3( actx );
    }
    catch( err ) {
        console.log( "CAUGHT2", err );
        console.log( "STACK", err.stack );
    }
} );

ctx.activate( throwTest4 );
