
var scheduler;

var P = Promise;

var SCHED_QUANTUM = 50;

var activity_state = Object.freeze( {
    RUNNABLE = 1,
    WAITING  = 2,
    FINISHED = 3,
} );

/* A TBN.js context is a collection of activity handles.
 * An activity is generator plus its next value */

/* private */ function scheduleForExec( actx )
{
    /* actx : activity context */
    var scheduler = actx.scheduler;
    if( scheduler.activities.length < 1 )
    {
        return P.resolve( 42 /* XXX not sure what a whole context should resolve to */ );
    }
    /* assert( actx.activities.length > 0 ) */
    /* HMMMM: Maybe this should never happen */
    if( scheduler.runnable.length < 1 )
    {
        return Promise; /* XXX How to sleep? */
    }
    /* assert( actx.activities.length > 0 ) */
    if( scheduler.runnable.length > 1 )
    {
        if( scheduler.runnable[ scheduler.runnable.length - 1 ] === scheduler.last_run )
        {
            var now = Date.now();
            var diff = now - scheduler.timestamp;
            if( diff > SCHED_QUANTUM )
            {
                scheduler.runnable.unshift( scheduler.runnable.pop() );
            }
        }
    }
    var actx = scheduler.runnable[ scheduler.runnable.length - 1 ];
    scheduler.last_run = actx;
    var continuation = actx.continuation;
    actx.continuation = null;
    return continuation();
}

/* private */
function runToNextYield( actx, generator, is_err, yielded_value )
{
    /* Parameter Types: */
    /* actx          : activity context type */
    /* generator     : generator type */
    /* is_err        : boolean */
    /* yielded_value : `a or Error */

    /* assert( actx.continuation === null ) */

    try {
        if( is_err )
            var next_yielded = generator.throw( yielded_value );
        else
            var next_yielded = generator.next( yielded_value );
    }
    catch( err ) {
        return P.reject( err );
    }
    /* next_yielded : { done : boolean, value : `b } */

    if( next_yielded.done )
    {
        return P.resolve( next_yielded.value );
    }
    /* "else": */

    function valueOrError( is_err, next_yielded_value )
    {
        /* TODO: Maybe a special case for only one activity? (for performance)
         * pitfall: if we add special case, starvation might be a problem. */
        if( scheduler.atomic_actx === null )
        {
            return runToNextYield( actx, generator, is_err, next yielded_value );
        }
        else
        {
            return new Promise( function( resolve, reject ) {
                actx.continuation = resolve;
                scheduler.waiting_activities.push( actx );
            } ).then(
                function() {
                    return runToNextYield(
                        actx, generator, is_err, next_yielded_value );
                } );
        }
    }

    return P.resolve( next_yielded.value ).then(
        function( next_yielded_value ) {
            return valueOrError( false, next_yielded_value );
        },
        function( err ) {
            return valueOrError( true, err );
        } );
}

function actProc( generator_function )
{
    function f( actx, ...params )
    {
        /* actx : activity context type */
        try {
            var generator = generator_function( actx, ...params );
            /* generator : iterator type */
            generator.good_name_entry_fn = false;
        }
        catch( err ) {
            return P.reject( err );
        }
        /* NOTE: leaving the value parameter out of the following call,
         * because the first call to next on a generator doesn't expect
         * a real value. */
        return runToNextYield( actx, generator, false );
    }
    f.good_name_js = {};
    return f;
}


function activities_js_makeContext()
{
    actx = {};
    actx.activities = [];

    actx.atomic = function( ...params_plus_f )
    {
        var this_actx = this;
        var params = params_plus_f.slice( 0, params_plus_f.length - 1 );
        var fn = params_plus_f[ params_plus_f.length - 1 ];
        // fn should either be a generator or an act fun
        if( !x.hasOwnProperty( 'GOOD_NAME_JS' ) )
        {
            fn = actProc( fn );
        }
        var scheduler = this_actx.scheduler;
        if( !( scheduler.atomic_actx === null ) )
        {
            /* assert( this_actx === scheduler.atomic_actx ) */
            /* NOTE: nested atomics are effectively ignored */
            return fn( params );
        }

        function leave_atomic()
        {
            /* assert( scheduler.atomic_actx == this_actx ) */
            scheduler.atomic_actx = null;
            while( scheduler.waiting_activities.length > 0 )
            {
                wactx = scheduler.waiting_activities.pop();
                var cont = wactx.continuation;
                wactx.continuation = null;
                cont();
            }
        }

        return fn( params ).then(
            function( val ) {
                leave_atomic();
                return P.resolve( val );
            },
            function( err ) {
                leave_atomic();
                return P.reject( err );
            } );
    }

    actx.activate = function( ...params_plus_f )
    {
        var scheduler = this.scheduler;
        var params = params_plus_f.slice( 0, params_plus_f.length - 1 );
        var fn = params_plus_f[ params_plus_f.length - 1 ];
        // fn should either be a generator or an act fun
        // fn must expect actx as its first parameter
        if( !fn.hasOwnProperty( 'GOOD_NAME_JS' ) )
        {
            fn = actProc( fn );
        }
        var actx_child = this.clone();
        /* TODO: Add new activity to the scheduler */
        actx_child.state = activity_state.RUNNABLE;
        actx_child.finished_promise =
            fn.apply( null, [ actx_child ].concat( params ) ).then(
                function( rv ) {
                    actx_child.state = activity_state.FINISHED;
                    /* TODO. Remove actx from the scheduler's collection */
                    return P.resolve( rv );
                } );
        return actx_child;
    }

    return actx;
}


/* scribbling */

actx.call( function*( actx))


var ex1 = actProc( function*( actx, a, b, c )
{
    
} );

var blah = actProc( function* blah( actx )
{
    var handle = yield activate( actx, 1, 2, 3 );
} );

actjs.makeCtx( function*( actx ) {
    
} )

handle = actx.activate( 1, '2', [ 3 ], function*( actx, a, b, c ) {
} )

handle = actx.activate( function*( actx ) {
    log( 'blah' );
} )
