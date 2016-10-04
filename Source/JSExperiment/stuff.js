
var scheduler;

var P = Promise;

function actProc( generator_function )
{
    function onYield( generator, yielded_promise )
    {
        /* generator : { next: ..., throw: ... } */
        /* yielded_promise : { done: bool, value: Promise( alpha ) } */
        if( yielded_promise.done )
        {
            /* TODO: yield to scheduler */
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
                return onYield( generator, next_promise_value );
            },
            function( err )
            {
                /* TODO: Maybe yield to scheduler here */
                return onYield( generator, generator.throw( err ) );
            }
        );
    }

    return function( actx, ...params )
    {
        var actx = actx.concat( generator_function.name );
        try {
            var generator = generator_function( actx, ...params );
        }
        catch( err ) {
            return P.reject( err );
        }
        try {
            /* TODO: yield to scheduler */
            /* The value passed to the first call to next is discarded */
            var first_yielded_promise = generator.next()
        }
        catch( err ) {
            return P.reject( err );
        }
        return onYield( generator, first_yielded_promise );
    }
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

function activities_js_makeRootContext()
{
    actx = {};
    return
}
