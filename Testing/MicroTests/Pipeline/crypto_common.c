#include "crypto_common.h"

void digest_message(
    const unsigned char *message,
    size_t               message_len,
    unsigned char      **digest,
    unsigned int        *digest_len )
{
	if( 1 != EVP_DigestInit_ex( mdctx, EVP_sha256(), NULL ) )
		handleErrors();

	if( 1 != EVP_DigestUpdate( mdctx, message, message_len ) )
		handleErrors();

	if( ( *digest = (unsigned char *)OPENSSL_malloc( EVP_MD_size( EVP_sha256() ) ) ) == NULL )
		handleErrors();

	if( 1 != EVP_DigestFinal_ex( mdctx, *digest, digest_len ) )
		handleErrors();
}

void crypto_open( void )
{
    /* Load the human readable error strings for libcrypto */
    ERR_load_crypto_strings();

    /* Load all digest and cipher algorithms */
    OpenSSL_add_all_algorithms();

    /* Load config file, and other important initialisation */
    OPENSSL_config(NULL);

	if( ( mdctx = EVP_MD_CTX_create() ) == NULL )
		handleErrors();

}

void crypto_close( void )
{
    /* Clean up */

	EVP_MD_CTX_destroy( mdctx );

    /* Removes all digests and ciphers */
    EVP_cleanup();

    /* if you omit the next, a small leak may be left when you make use
     * of the BIO (low level API) for e.g. base64 transformations */
    CRYPTO_cleanup_all_ex_data();

    /* Remove error strings */
    ERR_free_strings();
}

void handleErrors( void )
{
    unsigned long err = ERR_get_error();
    printf( "Error: %li\n", err );
    exit( 1 );
}
