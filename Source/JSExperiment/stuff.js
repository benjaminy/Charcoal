"use strict";

var scheduler;

var P = Promise;


function makeUniqueId( ids, min, max )
{
    var id;
    var found = false;
    if( !max )
    {
        max = 1000000;
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

/* actProc can be called in two ways:
 *  - with just a generator function
 *  - an activity context, then a generator function
 * The latter is mainly for internal use, but is available to client code
 */
function actProc( ...ap_params )
{

    function runToNextYield( actx, generator, is_err, yielded_value )
    {
        /* Parameter Types: */
        /* actx          : activity context type */
        /* generator     : generator type */
        /* is_err        : boolean */
        /* yielded_value : any */

        /* assert( actx.continuation === null ) */

        var scheduler = actx.scheduler;

        if( scheduler.inAtomicMode() && !( scheduler.inAtomicMode( actx ) ) )
        {
            /* Must suspend self */
            /* assert( actx not in scheduler.waiting_activities ) */
            actx.state = activity_state.WAITING;
            actx.waits++;
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

        /* Either no activities in atomic mode, or actx is in atomic mode */
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
            actx.state = activity_state.GENERATOR_ERROR;
            return P.reject( err );
        }
        /* next_yielded : { done : boolean, value : `b } */

        if( next_yielded.done )
        {
            return P.resolve( next_yielded.value );
        }
        /* "else": The generator yielded; it didn't return */

        return P.resolve( next_yielded.value ).then(
            function( next_yielded_value ) {
                return runToNextYield( actx, generator, false, next_yielded_value );
            },
            function( err ) {
                return runToNextYield( actx, generator, true, err );
            } );
    }


    /* Finally, the actual code that runs when actProc is called */
    if( ap_params.length === 1 )
    {
        var generator_function = ap_params[ 0 ];
        var actx_maybe         = null;
    }
    else if( ap_params.length === 2 )
    {
        var generator_function = ap_params[ 1 ];
        var actx_maybe         = ap_params[ 0 ];
    }
    else
    {
        throw "XXX Replace Me";
    }

    function f_either_mode( pass_actx, actx, ...params )
    {
        /* actx : activity context type */
        try {
            if( pass_actx )
                var generator = generator_function( actx, ...params );
            else
                var generator = generator_function( ...params );
            /* generator : iterator type */
        }
        catch( err ) {
            return P.reject( err );
        }
        /* NOTE: leaving the value parameter out of the following call,
         * because the first call to next on a generator doesn't expect
         * a real value. */
        return runToNextYield( actx, generator, false );
    }

    if( actx_maybe )
    {
        var f = function( ...params )
        {
            return f_either_mode( false, actx_maybe, ...params );
        }
        f.ACTIVITIES_JS_EXPECTS_CTX = false;
    }
    else
    {
        var f = function( ...params )
        {
            /* assert( params[ 0 ] is an activity context ) */
            return f_either_mode( true, ...params );
        }
        f.ACTIVITIES_JS_EXPECTS_CTX = true;
    }
    f.ACTIVITIES_JS_TOKEN = {};
    return f;
}


class ActivityContext
{
    constructor( ctx ) {
        this.continuation            = null;
        this.waits                   = 0;
        if( ctx )
        {
            this.scheduler = ctx.scheduler;
            this.id = makeUniqueId( this.scheduler.activities );
        }
        else
        {
            scheduler = {}
            scheduler.activities         = {};
            scheduler.atomic_actx        = null;
            scheduler.waiting_activities = [];
            scheduler.inAtomicMode       =
                function( a ) {
                    if( a )
                        return this.atomic_actx === a;
                    else
                        return !( this.atomic_actx === null );
                }

            this.scheduler = scheduler;
            this.id        = 0;
        }
        this.scheduler.activities[ this.id ] = this;
    }

    atomic( ...params_plus_fn )
    {
        var this_actx = this;
        var params = params_plus_fn.slice( 0, params_plus_fn.length - 1 );
        var fn = params_plus_fn[ params_plus_fn.length - 1 ];
        /* fn : ActFn | generator function */
        if( !fn.hasOwnProperty( 'ACTIVITIES_JS_TOKEN' ) )
        {
            fn = actProc( this_actx, fn );
        }
        var scheduler = this_actx.scheduler;
        var first_entry = scheduler.atomic_actx === null;
        if( !first_entry )
        {
            /* assert( this_actx === scheduler.atomic_actx ) */
        }

        function leave_atomic()
        {
            /* assert( scheduler.atomic_actx === this_actx ) */
            if( !first_entry )
            {
                /* NOTE: nested atomics are effectively ignored */
                return;
            }
            scheduler.atomic_actx = null;
            scheduler.waiting_activities.sort( function( a, b ) {
                return a.waits - b.waits;
            } );
            while( scheduler.waiting_activities.length > 0 )
            {
                wactx = scheduler.waiting_activities.pop();
                var cont = wactx.continuation;
                wactx.continuation = null;
                cont();
            }
        }

        try {
            if( fn.ACTIVITIES_JS_EXPECTS_CTX )
                var p = fn( this, ...params );
            else
                var p = fn( ...params );
        }
        catch( err ) {
            leave_atomic();
            return P.reject( err );
        }

        return p.then(
            function( val ) {
                leave_atomic();
                return P.resolve( val );
            },
            function( err ) {
                leave_atomic();
                return P.reject( err );
            } );
    }

    activate( ...params_plus_f )
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
        var actx_child = new ActivityContext( this );
        actx_child.state = activity_state.RUNNABLE;
        actx_child.finished_promise =
            fn( actx_child, ...params ).then(
                function( rv ) {
                    actx_child.state = activity_state.FINISHED;
                    /* TODO. Improve scaling, maybe */
                    for( var i = 0; i < scheduler.activities.length; ++i )
                    {
                        
                    }
                    return P.resolve( rv );
                } );
        return actx_child;
    }

}

/* scribbling */

var ctx = new ActivityContext()

var f = actProc( function*( actx, letter ) {
    while( true )
    {
        yield new Promise( resolve => setTimeout( resolve, 1000 ) );
        console.log( letter );
    }
} );

ctx.activate( "A", f );
ctx.activate( "B", f );

// actx.call( function*( actx))


// var ex1 = actProc( function*( actx, a, b, c )
// {
    
// } );

// var blah = actProc( function* blah( actx )
// {
//     var handle = actx.activate( function*( child ) {
//         do_stuff();
//         var whatever = yield child.atomic( function*() {
//             yield do_something();
//             return yield do_other();
//         } );
//     } );
// } );

// actjs.makeCtx( function*( actx ) {
    
// } )

// handle = actx.activate( 1, '2', [ 3 ], function*( actx, a, b, c ) {
// } )

// handle = actx.activate( function*( actx ) {
//     log( 'blah' );
// } )
