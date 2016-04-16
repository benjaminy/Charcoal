#include <stdint.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <crypto_common.h>

EVP_MD_CTX *mdctx;

#define RATIO 4

#define DO_RNG
#define DO_HASH

int main( int argc, char *argv[] )
{
    if( argc < 3 )
    {
        printf( "ARGS suggested: 8 19\n" );
        exit( 1 );
    }
    long M = atol( argv[1] );
    M = 1 << M;
    long N = atol( argv[2] );
    N = 1 << N;

    crypto_open();

    uint8_t buffer[ RATIO * M ];
    int i;
    for( i = 0; i < N; ++i )
    {
#ifdef DO_RNG
        size_t chunk = sizeof(buffer) / RATIO;
        int rc = RAND_pseudo_bytes( buffer + ( ( i % RATIO ) * chunk), chunk );
        if( rc != 0 && rc != 1 )
        {
            handleErrors();
        }
#endif
#ifdef DO_HASH
        unsigned char *digest;
        unsigned int len;
        digest_message( buffer, RATIO * M, &digest, &len );
        // printf( "%i %i %i\n", i, buffer[0], digest[0] );
        OPENSSL_free( digest );
#endif
    }

    crypto_close();
    return 0;
}
