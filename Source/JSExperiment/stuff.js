
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


/* private */ function onJSYield( generator, yielded_promise )
{
    /* yielded_promise<a> : { done: bool, value: Promise<a> } */
    if( yielded_promise.done )
    {
        /* TODO: pop frame. If last, close activity. */
        return P.resolve( yielded_promise.value );
    }
    /* "else" */
    return P.resolve( yielded_promise.value ).then(
        function( res )
        {
            /* TODO: Maybe yield to scheduler here */
            try {
                var next_yielded_promise = generator.next( res );
            }
            catch( err ) {
                return P.reject( err );
            }
            return onJSYield( next_promise_value );
        },
        function( err )
        {
            /* TODO: Maybe yield to scheduler here */
            return onJSYield( generator.throw( err ) );
        }
    );
}

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
        if( actx.scheduler.runnable.length < 1 )
            return runToNextYield(
                actx, generator, is_err, next_yielded_value );
        /* "else": */

        return new Promise( function( resolve, reject ) {
            actx.continuation = resolve;
            scheduleForExec( actx );
        } ).then(
            function() {
                return runToNextYield(
                    actx, generator, is_err, next_yielded_value );
            },
            function( async_err ) {
                /*  */
            } );
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


            
                scheduler.last_run
    var act = actx.activities[ 0 ];
    var frames = act.frames;
    var frame = frames[ frames.length - 1 ];
    if( frame.yielded.done )
    {
        frames.pop();
        if( frames.length < 1 )
        {
        }
        else
        {
            frames[ frames.length - 1 ].yielded.value = ;
        }
        return P.resolve( last_yield.value );
    }
    
    return P.resolve( act.next_promise.value ).then(
        function( val )
        {
        },
        function( err )
        {
        } );





            
        }
        return schedulerLoop();

    }
    f.good_name_js = {};
    return f;
}

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

/* private */ var entryFn = actProc( function* ( actx, f, ...params ) {
    actx.result = yield f.apply( [ actx ].concat( params ) );
    actx.state  = activity_state.FINISHED;
    /* TODO. Remove actx from the scheduler's collection */
} );

function activities_js_makeContext()
{
    actx = {};
    actx.activities = [];

    actx.atomic_enter = function()
    {
        this //
    }

    actx.atomic_exit = function()
    {
        this //
    }

    actx.yield = function()
    {
    }

    actx.activate = function( ...params_plus_f )
    {
        var params = params_plus_f.slice( 0, params_plus_f.length - 1 );
        var fn = params_plus_f[ params_plus_f.length - 1 ];
        // fn should either be a generator or an act fun
        if( !x.hasOwnProperty( 'GOOD_NAME_JS' ) )
        {
            fn = actProc( fn );
        }
        actx_child = this.clone();
        actx_child.activity = {};
        var something = entryFn()
        var cont = function() {
            
        }
        if( this.child_first )
        {
            this.activities.push( actx_child.activity );
        }
        else
        {
            this.activities.unshift( actx_child.activity );
        }
        return schedulerLoop( this );
    }

    return actx;
}
