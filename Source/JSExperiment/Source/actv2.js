/* Top Matter */

import assert from "assert";
const P = Promise;

function yieldP()
{
    return new Promise( ( res ) => setImmediate( res ) );
}

let ATOMICIFY_TAG = Symbol( "atomicable" );

let scheduler =
    {
        all_actxs           : {};
        active_actxs        : new Set( [] );
        num_activities      : 0;
        atomic_stack        : [];
    };

/* The global context is syntactically like a regular activity context,
 * but it never resolves. */
let global_context = new Promise( function() {} );
makeContext( global_context, null );
global_context[ SCHEDULER_TAG ] = scheduler;

var current_context = global_context;

/* "prelude" is a convenience function */
function prelude()
{
    let actx = current_context;
    assert( isContext( actx ) );
    let scheduler = actx[ SCHEDULER_TAG ];
    assert( sched.active_actxs.has( parent[ ID_TAG ] ) );
    let atomic_stack = scheduler.atomic_stack;
    return [ actx, scheduler, atomic_stack ];
}

/* Reminder: f can't be an async function, because we need to reset the
 * cur_ctx global variable after every yield. */
/* Reminder: It would be a neat convenience if atomicified functions
 * could be called as regular async functions.  This seems to mess with
 * the global variable hack. */
function atomicify( f )
{
    /* assert( f is generator function ) */
    if( ATOMICIFY_TAG in f )
        return f;

    async function stepper( ...params )
    {
        let [ actx, scheduler, atomic_stack ] = prelude();
        let call_stack = actx[ CALL_STACK_TAG ];
        let generator = f( ...params );
        if( !( generator
               && ( typeof generator[Symbol.iterator] === "function" ) ) )
        {
            throw new Error( "atomicify passed (" + typeof( f )
                             + "). Needs generator function." );
        }
        call_stack.push( [ f, generator ] );

        function blockedByAtomic()
        {
            let not_blocked = ( atomic_stack.length < 1 )
                || ( scheduler.active_actxs.has( actx[ ID_TAG ] ) );
            return !not_blocked;
        }

        try {
            var val = undefined; /* 1st call to .next doesn't need a val */
            var is_err = false;
            /* implicit yield before calls: */
            await yieldP();
            while( true ) /* return in loop, when generator is done */
            {
                let blocks = []; /* TODO: stats about blocking */
                while( blockedByAtomic() )
                {
                    /* assert( actx not in scheduler.waiting_activities ); */
                    let top = atomic_stack[ atomic_stack.length - 1 ];
                    actx[ STATE_TAG ] = WAITING;
                    actx[ WAITS_TAG ]++;
                    actx[ QUEUE_LEN_TAG ] = top.waiting.length;
                    top.waiting.add( actx );
                    let wait = new Promise( function( resolve, reject ) {
                        actx[ CONTINUE_TAG ] = resolve;
                        actx[ ABORT_TAG ]    = reject;
                    } );
                    blocks.push( await wait );
                }

                current_context = actx;
                if( is_err )
                    var generated = generator.throw( val );
                else
                    var generated = generator.next( val );

                if( generated.done )
                    return generated.value;

                try {
                    val = await generated.value;
                    is_err = false;
                }
                catch( err ) {
                    val = err;
                    is_err = true;
                }
            }
        }
        finally {
            assert( call_stack.length > 0 );
            let [ fa, ga ] = call_stack.pop();
            assert( ga === generator );
            assert( fa === f );
        }
    }

    stepper[ ATOMICIFY_TAG ] = undefined;
    return wrapper;
}

function makeContext( child, parent )
{
    /* assert( child is Promise ) */
    assert( parent === null || isContext( parent ) );
    child[ CONTINUE_TAG ]   = null;
    child[ ABORT_TAG ]      = null;
    child[ WAITS_TAG ]      = 0;
    child[ CALL_STACK_TAG ] = [];
    child[ SCHEDULER_TAG ]  = parent ? parent[ SCHEDULER_TAG ] : scheduler;
    child[ ID_TAG ]         = Symbol( "activity_id" );
    child[ STATE_TAG ]      = RUNNABLE;
    child[ PARENT_TAG ]     = parent;
}

function activateOpts( options )
{
return function activate( ...params_plus_f )
{
    let [ parent, sched, atomic_stack ] = prelude();
    console.log( "activate" );
    var tieKnot;
    let knot = new Promise( function( r ) { tieKnot = r } );
    let child = async function()
    {
        await knot;
        makeContext( child, parent );
        assert( isContext( child ) );
        sched.all_actxs[ child[ ID_TAG ] ] = child;
        sched.num_activities++;
        assert( Object.keys( sched.all_actxs ).length === sched.num_activities );
        if( atomic_stack.length > 0 )
        {
            /* XXX More cases */
            atomic_stack[ atomic_stack.length - 1 ].peers.add( child );
        }
        else
        {
            sched.active_actxs.add( child[ ID_TAG ] );
            current_context = child;
        }
        try {
            let params = params_plus_f.slice( 0, params_plus_f.length - 1 );
            let fn     = atomicable( params_plus_f[ params_plus_f.length - 1 ] );
            return await fn( ...params );
        }
        finally {
            /* XXX what about atomic stuff??? */
            child[ STATE_TAG ] = FINISHED;
            const id = child[ ID_TAG ];
            assert( id in sched.all_actxs );
            assert( sched.active_actxs.has( id ) );
            delete sched.all_actxs[ id ];
            sched.active_actxs.delete( id );
            sched.num_activities--;
            assert( Object.keys( sched.all_actxs ).length === sched.num_activities );
            console.log( msg, id, sched.num_activities );
        }
    } ();

    tieKnot();
    /* Wrapping the child Promise in an extra Promise to force the
     * caller of activate to yield so that current_context is set
     * properly. */
    return P.resolve( c );
}
}

function activateOpts( options )
{
return function activate( ...params_plus_f )
{
    let [ parent, sched, atomic_stack ] = prelude();
    console.log( "activate" );

    let child = R.resolve().then( function() {
        makeContext( child, parent );
        assert( isContext( child ) );
        sched.all_actxs[ child[ ID_TAG ] ] = child;
        sched.num_activities++;
        assert( Object.keys( sched.all_actxs ).length === sched.num_activities );
        if( atomic_stack.length > 0 )
        {
            /* XXX More cases */
            atomic_stack[ atomic_stack.length - 1 ].peers.add( child );
        }
        else
        {
            sched.active_actxs.add( child[ ID_TAG ] );
            current_context = child;
        }
        try {
            let params = params_plus_f.slice( 0, params_plus_f.length - 1 );
            let fn     = atomicable( params_plus_f[ params_plus_f.length - 1 ] );
            return fn( ...params );
        }
        catch( err ) {
            return P.reject( err );
        }
    } ).then(
        function( rv ) {
            cleanupActivity( "Activity Finished" );
            return rv;
        },
        function( err ) {
            cleanupActivity( "Activity Aborted" );
            return P.reject( err );
        } );

    function cleanupActivity( msg )
    {
        /* XXX what about atomic stuff??? */
        child[ STATE_TAG ] = FINISHED;
        const id = child[ ID_TAG ];
        assert( id in sched.all_actxs );
        assert( sched.active_actxs.has( id ) );
        delete sched.all_actxs[ id ];
        sched.active_actxs.delete( id );
        sched.num_activities--;
        assert( Object.keys( sched.all_actxs ).length === sched.num_activities );
        console.log( msg, id, sched.num_activities );
    }

    child_resolve();
    /* Wrapping the child Promise in an extra Promise to force the
     * caller of activate to yield so that current_context is set
     * properly. */
    return P.resolve( child );
}
}

let activate = activateOpts( {} );
let activateShortLived = activateOpts( { "short-lived":true } );

function atomicOpts( options )
{
return async function atomic( ...params_plus_fn )
{
    let [ parent, sched, atomic_stack ] = prelude();
    console.log( "atomic" );

    let new_top      = {};
    new_top.peers = new Set( scheduler.active_actxs );
    scheduler.active_actxs = new Set( [ actx ] );
    scheduler.atomic_stack.push( new_top );

    try {
        var params = params_plus_fn.slice( 0, params_plus_fn.length - 1 );
        let fn     = atomicable( this, params_plus_fn[ params_plus_fn.length - 1 ] );
        var rv = await fn( ...params );
        var is_err = false;
    }
    catch( err ) {
        var rv = err;
        var is_err = true;
    }
    /* pop atomic stack -- complicated shit */

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
        let diff = b.waits - a.waits;
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

    if( is_err )
    {
        throw rv;
    }
    return rv;
}
}

let atomic = atomicOpts( {} );

function selfActivity()
{
    assert( isContext( current_context ) );
    return current_context;
}



function isContext( thing )
{
    if( thing == null )
        return false;
    return ID_TAG in thing;
}

atomicify.Scheduler = Scheduler;
atomicify.isContext = isContext;
atomicify.activate  = activate;

export default atomicify;
