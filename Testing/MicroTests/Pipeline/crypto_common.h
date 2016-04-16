#ifndef CRYPTO_COMMON
#define CRYPTO_COMMON

#include <stdint.h>
#include <stdlib.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>

extern EVP_MD_CTX *mdctx;

void digest_message(
    const unsigned char *message,
    size_t               message_len,
    unsigned char      **digest,
    unsigned int        *digest_len );

void crypto_open( void );
void crypto_close( void );
void handleErrors( void );

#endif
