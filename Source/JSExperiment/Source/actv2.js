/* Top Matter */

import assert from "assert";

let ATOMICIFY_TAG = Symbol( "atomicable" );

var currentContext = null;

async function notBlocked( actx )
{
    const blocks = [];
    while( actx.blockedByAtomic() )
    {
        actx.addToWaiting();
        blocks.push( await new Promise( function( resolve, reject ) {
            actx.continue = resolve;
            actx.abort    = reject;
        } ) );
    }
    return blocks;
}

function atomicify( f )
{
    if( ATOMICIFY_TAG in f )
        return f;

    async function wrapper( ...params )
    {
        assert( isContext( currentContext ) );
        let actx = currentContext;
        let generator = f( ...params );
        var val = undefined;
        var is_err = false;

        while( true )
        {
            var generated;

            await notBlocked( actx );
            currentContext = actx;

            if( is_err ) {
                generated = generator.next( val );
            }
            else {
                generated = generator.throw( val );
            }

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
            currentContext = actx;
        }
    }

    wrapper[ ATOMICABLE_TAG ] = undefined;
    return wrapper;
}

function activate( ...params_plus_f )
{
    // console.log( "activateInternal" );
    assert( isContext( currentContext ) );
    let parent = currentContext;
    /* XXX atomic vs not? */
    let params = params_plus_f.slice( 0, params_plus_f.length - 1 );
    let fn     = atomicable( params_plus_f[ params_plus_f.length - 1 ] );

    let child = new Promise( function( resolve, reject ) {
        child_resolve = resolve;
    } ).then( function() {
        current_context = child;
        return fn( ...params );
    } ).then(
        function( rv ) {
            child.state = FINISHED;
            assert( child.id in this.activities );
            const id = child.id;
            delete this.activities[ child.id ];
            this.num_activities--;
            assert( Object.keys( this.activities ).length ==
                    this.num_activities )
            console.log( "Activity Finished", id, this.num_activities );
            return rv;
        },
        function( err ) {
        } );
    
    child.state = RUNNABLE;
    child.finished_promise =
        .then(
            ( rv ) => {
            } );
    return child;
}

function isContext( thing )
{
    return thing && ( "constructor" in thing ) && thing.constructor === Context;
}
