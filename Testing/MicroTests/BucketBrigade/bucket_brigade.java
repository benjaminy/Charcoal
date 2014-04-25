import java.util.concurrent.Semaphore;

class bucket_brigade
{
    static int N = 10;
    static int m = N;

    static class T extends Thread
    {
        Semaphore sem1, sem2;
        public T( Semaphore s1, Semaphore s2 )
        {
            sem1 = s1;
            sem2 = s2;
        }
        public void run()
        {
            for( int i = 0; i < m; ++i )
            {
                try
                {
                    sem1.acquire();
                }
                catch( Exception e )
                {
                    System.out.println( "Oops" );
                }
                sem2.release();
            }
        }
    }

    public static void main( String args[] )
    {
        System.out.println( "Hello" );
        if( args.length > 0 )
        {
            m = Integer.parseInt( args[0] );
        }
        Thread    ts[]   = new Thread[N];
        Semaphore sems[] = new Semaphore[N];
        for( int i = 0; i < N; ++i )
        {
            sems[i] = new Semaphore(0);
        }
        for( int i = 0; i < N; ++i )
        {
            ts[i] = new T( sems[i], sems[(i+1)%N] );
            ts[i].start();
        }
        sems[0].release();
        for( int i = 0; i < N; ++i )
        {
            try
            {
                ts[i].join();
            }
            catch( Exception e )
            {
                System.out.println( "oops" );
            }
        }
    }
}
