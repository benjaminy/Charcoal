import java.util.Random;
import java.util.Arrays;

class merge_sort
{
    static final int N = 100_000_000;
    static final int THRESHOLD = 50;
    static final int THRESHOLD2 = 200_000;

    public void real_main( String[] args )
    {
        Random r = new Random( 42 );
        int [] ns = new int[ N ];
        for( int i = 0; i < N; ++i )
        {
            ns[ i ] = r.nextInt();
        }

        // insertion_sort( ns, 0, ns.length );

        int [] scratch = new int[ N ];
        // normal_merge_sort( ns, scratch, 0, ns.length );

        MergeSortWorker w = new MergeSortWorker(
            ns, scratch, 0, ns.length );
        w.run();

        // System.out.println( Arrays.toString( ns ) );
        System.out.println( "S"+ is_sorted( ns ) );
    }

    void normal_merge_sort( int [] nums, int [] scratch, int low, int high )
    {
        int range = high - low;
        if( range < THRESHOLD )
        {
            insertion_sort( nums, low, high );
            return;
        }
        int mid = ( high + low ) / 2;
        normal_merge_sort( nums, scratch, low, mid );
        normal_merge_sort( nums, scratch, mid, high );
        merge( nums, scratch, low, high );
    }

    void merge( int [] nums, int [] scratch, int low, int high )
    {
        int mid = ( high + low ) / 2;
        int out = low;
        int left = low;
        int right = mid;
        while( left < mid && right < high )
        {
            if( nums[ left ] < nums[ right ] )
            {
                scratch[ out ] = nums[ left ];
                ++left;
            }
            else
            {
                scratch[ out ] = nums[ right ];
                ++right;
            }
            ++out;
        }
        while( left < mid )
        {
            scratch[ out ] = nums[ left ];
            ++left;
            ++out;
        }
        while( right < high )
        {
            scratch[ out ] = nums[ right ];
            ++right;
            ++out;
        }
        for( int i = low; i < high; ++i )
        {
            nums[ i ] = scratch[ i ];
        }
    }

    class MergeSortWorker implements Runnable
    {
        int [] nums, scratch;
        int low, high;
        public MergeSortWorker( int [] ns, int [] s, int l, int h )
        {
            nums = ns;
            scratch = s;
            low = l;
            high = h;
        }

        public void run()
        {
            int range = high - low;
            if( range < THRESHOLD2 )
            {
                normal_merge_sort( nums, scratch, low, high );
                return;
            }
            int mid = ( high + low ) / 2;
            Thread tl =
                new Thread(
                    new MergeSortWorker(
                        nums, scratch, low, mid ) );
            Thread tr =
                new Thread(
                    new MergeSortWorker(
                        nums, scratch, mid, high ) );
            tl.start();
            tr.start();
            try {
                tl.join();
                tr.join();
            }
            catch( Exception e ) {}
            merge( nums, scratch, low, high );
        }
    }

    void insertion_sort( int [] nums, int low, int high )
    {
        for( int i = low; i < high; ++i )
        {
            int j = i;
            while( j > low &&
                   nums[ j ] < nums[ j - 1 ] )
            {
                int temp = nums[ j ];
                nums[ j ] = nums[ j - 1 ];
                nums[ j - 1 ] = temp;
                --j;
            }
        }
    }

    boolean is_sorted( int [] nums )
    {
        for( int i = 1; i < nums.length; ++i )
        {
            if( nums[ i ] < nums[ i - 1 ] )
                return false;
        }
        return true;
    }

    public static void main( String [] args )
    {
        ( new merge_sort() ).real_main( args );
    }
}
