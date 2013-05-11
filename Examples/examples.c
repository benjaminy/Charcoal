

one_shot_event_broadcaster()
{
    /* register listeners and wait for signal */
    choose();
    /* send pings */
    choose();
}

void build_one_shot_event_broadcaster()
{
    buffer;
    channel;
    channel_init( &channel, &buffer );
    start_activity( one_shot_event_broadcaster )
}

void multidns_seq( dnsname *names, ipaddr *addrs, int n )
{
    int i;
    for( i = 0; i < n; ++i )
    {
        int rc = getaddrinfo( names[i], servname, hints, &addrs[i] );
    }
}

void multidns_conc( dnsname *names, ipaddr *addrs, int n )
{
    int i, completed = 0;
    semaphore done_sem = sem_init( 0 );
    for( i = 0; i < n; ++i )
    {
        activate (i)
        {
            int rc = getaddrinfo( names[i], servname, hints, &addrs[i] );
            ++completed;
            if( completed == n )
                sem_inc( done_sem );
        }
    }
    sem_dec( done_sem );
}

void multidns_conc_lim( dnsname *names, ipaddr *addrs, int n, int lim )
{
    int i, completed = 0;
    semaphore done_sem  = sem_init( 0 ),
              limit_sem = sem_init( lim );
    for( i = 0; i < n; ++i )
    {
        activate (i)
        {
            sem_dec( limit_sem );
            int rc = getaddrinfo( names[i], servname, hints, &addrs[i] );
            sem_inc( limit_sem );
            ++completed;
            if( completed == n )
                sem_inc( done_sem );
        }
    }
    sem_dec( done_sem );
}

void multidns_conc_lim_ev(
    dnsname *names, ipaddr *addrs, int n, int lim, event *ev )
{
    int i, completed = 0;
    semaphore done_sem  = sem_init( 0 ),
              limit_sem = sem_init( lim );
    for( i = 0; i < n; ++i )
    {
        activate (i)
        {
            sem_dec( limit_sem );
            int rc = getaddrinfo( names[i], servname, hints, &addrs[i] );
            sem_inc( limit_sem );
            ++completed;
            if( completed == n )
                sem_inc( done_sem );
        }
    }
    if( ev )
        sem_dec_ev( ev, done_sem );
    else
        sem_dec( done_sem );
}

#if 0
activate (i,j,k) return init_fn( i, j, k );
#endif
