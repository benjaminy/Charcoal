class basic_race
{
    // "bank_balance" is the shared state/data the threads will "race" on
    int bank_balance1 = 10, bank_balance2 = 10;

    class Transfer implements Runnable
    {
        public boolean made_transfer = false;

        public void run()
        {
            // The classic "hello world" of concurrency bugs: bank transfers
            if( bank_balance1 >= 1 )
            {
                sleep_helper();
                bank_balance1 = bank_balance1 - 1;
                bank_balance2 = bank_balance2 + 1;
                made_transfer = true;
            }
        }
    }

    void real_main() throws InterruptedException
    {
        Transfer transfers[] = new Transfer[ N ];
        Thread spool[] = new Thread[ N ];

        for( int i = 0; i < N; ++i )
        {
            transfers[ i ] = new Transfer();
            spool[ i ]  = new Thread( transfers[ i ] );
        }

        for( int i = 0; i < N; ++i )
        {
            spool[ i ].start();
        }

        for( int i = 0; i < N; ++i )
        {
            spool[ i ].join();
        }

        int transfer_count = 0;
        for( int i = 0; i < N; ++i )
        {
            if( transfers[ i ].made_transfer )
            {
                ++transfer_count;
            }
        }

        int cores = Runtime.getRuntime().availableProcessors();
        System.out.println( "Physical processors available: " + cores );
        System.out.println( "balance 1: " + bank_balance1 + " balance 2: " + bank_balance2 );
        System.out.println( "Threads that made transfers: " + transfer_count );
    }

    final int N = 1000;
    static long sleep_nanoseconds = 0;

    public static void main( String [] args ) throws InterruptedException
    {
        if( args.length > 0 )
        {
            sleep_nanoseconds = Long.parseLong( args[ 0 ] );
        }
        ( new basic_race() ).real_main();
    }

    static void sleep_helper()
    {
        if( sleep_nanoseconds > 0 )
        {
            long ms =        sleep_nanoseconds / 1000000L;
            int  ns = (int)( sleep_nanoseconds % 1000000L );
            try {
                Thread.sleep( ms, ns );
            }
            catch( InterruptedException exn )
            {}
        }
    }
}
