class basic_race
{
    // "bank_account" is the shared state/data the threads will "race" on
    int bank_account_a = 100, bank_account_b = 100;
    Object pay_no_attention_to_me = new Object();

    class Transfer implements Runnable
    {
        public boolean made_transfer = false;

        void money_transaction_hello_world()
        {
            if( bank_account_a >= 10 )
            {
                sleep_helper();
                bank_account_a -= 10;
                bank_account_b += 10;
                made_transfer = true;
            }
        }

        public void run() { money_transaction_hello_world(); }
    }

    void real_main() throws InterruptedException
    {
        Transfer transfers[] = new Transfer[ N ];
        Thread   spool[]     = new Thread[ N ];

        /* allocate memory for N threads */
        for( int i = 0; i < N; ++i )
        {
            transfers[ i ] = new Transfer();
            spool[ i ]     = new Thread( transfers[ i ] );
        }

        /* Start all the threads running concurrently.
         * Each new thread starts in its "run" method. */
        for( Thread t : spool )
            t.start();

        /* wait until all N threads have finished */
        for( Thread t : spool )
            t.join();

        int transfer_count = 0;
        for( Transfer t : transfers )
            if( t.made_transfer )
                ++transfer_count;

        int cores = Runtime.getRuntime().availableProcessors();
        int total_space_bucks = bank_account_b + bank_account_a;

        System.out.println( "Physical processors available: " + cores );
        System.out.println( "Acct A: " + bank_account_a );
        System.out.println( "Acct B: " + bank_account_b );
        System.out.println( "Threads that made transfers: " + transfer_count );
        System.out.println( "" );
        System.out.println( "Sanity?" );
        System.out.println( "A + B   : " + total_space_bucks );
        System.out.println( "B - Txn : " + ( bank_account_b - 10 * transfer_count ) );
        System.out.println( "A + Txn : " + ( bank_account_a + 10 * transfer_count ) );
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
            long ms = sleep_nanoseconds;
            int  ns = 0;
            // long ms =        sleep_nanoseconds / 1000000L;
            // int  ns = (int)( sleep_nanoseconds % 1000000L );
            try {
                Thread.sleep( ms, ns );
            }
            catch( InterruptedException exn )
            {}
        }
    }
}
