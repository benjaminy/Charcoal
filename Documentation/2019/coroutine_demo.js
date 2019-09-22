const readline = require( "readline" );

const rl = readline.createInterface(
    {
        input: process.stdin,
        output: process.stdout
    } );

var close_stdin;
const stdin_closed = new Promise( ( s, j ) => { close_stdin = j; } );
rl.on( "close", close_stdin );

function ask( question )
{
    p = new Promise( ( resolve ) => {
        rl.question( question, ( name ) => { resolve( name ) } );
    } );
    return Promise.race( [ p, stdin_closed ] );
}

function* fib_coroutine()
{
    var x = 1;
    yield x;
    var y = 1;
    yield y;
    while( true )
    {
        var z = x + y;
        yield z;
        x = y;
        y = z;
    }
}

async function main()
{
    var coroutine_handle = fib_coroutine();
    while( true )
    {
        try {
            await ask( "Want a fib? " );
        }
        catch( err ) {
            console.log( "Well alright then" );
            break;
        }
        const x = coroutine_handle.next();
        console.log( "Next fib: " + x.value );
    }
}

main();
