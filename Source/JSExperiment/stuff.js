
var scheduler;

function actProc( generator_function )
{
    function handleYield( generator, yielded_value )
    {
        /* yielded_value : ( bool * alpha ) */
        if( yielded_value.done )
        {
            /* yield to scheduler */
            return Promise.resolve( yielded_value.value );
        }
        /* "else" */
        return Promise.resolve( yielded_value.value ).then(
            function( res )
            {
                try {
                    var next_yielded_value = generator.next( res );
                }
                catch( err ) {
                    return Promise.reject( err );
                }
                return handleYield( generator, next_yielded_value );
            },
            function( err )
            {
                return handleYield( generator, generator.throw( err ) );
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
            return Promise.reject( err );
        }
        try
        {
            /* yield to scheduler */
            /* The value passed to the first call to next is discarded */
            var first_yielded_value = generator.next()
        }
        catch( err )
        {
            return Promise.reject( err );
        }
        return handleYield( generator, first_yielded_value );
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
