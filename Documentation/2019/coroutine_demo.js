const readline = require( "readline" );
const fs       = require( "fs" );
const util     = require( "util" );
// Convert fs.readFile into Promise version of same
const readFile = util.promisify( fs.readFile );

/* Async function-style coroutine */
async function main()
{
    try {
        /* Calling an async function-style coroutine */
        var name = await ask( "File name? " );
    }
    catch( err ) {
        console.log( "Well alright then" );
        return;
    }

    try {
        /* Calling an async function-style coroutine */
        const readFilePromise = readFile( name );
        var text = ( await readFilePromise ).toString();
    }
    catch( err ) {
        console.log( "Sorry, failed to read file "+name );
        return;
    }

    const generator_handle = find_thes( text );
    while( true )
    {
        const yielded_container = generator_handle.next();
        if( yielded_container.done )
            break;
        const line = yielded_container.value
        console.log( line.num + ": " + line.text );
    }
}

/* Generator-style coroutine */
function* find_thes( text )
{
    var start_of_line = 0;
    var line_num = 1;

    while( true )
    {
        const next_the  = text.indexOf( "the", start_of_line );
        const next_line = text.indexOf( "\n",  start_of_line );

        if( next_the < 0 )
            return;

        if( next_the < next_line )
        {
            const line = text.substring( start_of_line, next_line );
            yield { num: line_num, text: line };
        }
        start_of_line = next_line + 1;
        line_num += 1;
    }
}

function ask( question )
{
    p = new Promise( ( resolve ) => {
        rl.question( question, ( name ) => { resolve( name ) } );
    } );
    return Promise.race( [ p, stdin_closed ] );
}

const rl = readline.createInterface(
    {
        input: process.stdin,
        output: process.stdout
    } );

var close_stdin;
const stdin_closed = new Promise( ( s, j ) => { close_stdin = j; } );
rl.on( "close", close_stdin );

main().then( () => { console.log( "... and we're done!" ); } );

// console.log( "Interrupting cow... MOOOO!" );
