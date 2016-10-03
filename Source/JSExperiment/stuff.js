
var scheduler;

function actProc( generator_function )
{
    function handleYield( generator, result )
    {
        if( result.done )
        {
            /* yield to scheduler */
            return Promise.resolve( result.value );
        }
        /* "else" */
        return Promise.resolve( result.value ).then(
            function( res )
            {
                return handleYield( generator, generator.next( res ) );
            }
            function( err )
            {
                return handleYield( generator, generator.throw( err ) );
            }
        );
    }

    return function( actx, ...params )
    {
        var actx = actx.concat( generator_function.name );
        var generator = generator_function( actx, ...params );
        try
        {
            /* yield to scheduler */
            return handleYield( generator, generator.next() );
        }
        catch( err )
        {
            return Promise.reject( err );
        }
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

handle = actx.spawn( 1, '2', [ 3 ], function*( actx, a, b, c ) {
} )

handle = actx.spawn( function*( actx ) {
    log( 'blah' );
} )

function activities_js_makeRootContext()
{
    actx = {};
    return
}
