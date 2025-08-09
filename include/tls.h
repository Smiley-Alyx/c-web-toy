#ifndef TLS_H
#define TLS_H
#ifdef ENABLE_TLS
#include <openssl/ssl.h>

typedef struct { SSL_CTX* ctx; } TlsContext;

int  tls_init(TlsContext* t, const char* cert_file, const char* key_file);
SSL* tls_accept_client(TlsContext* t, int client_fd);
void tls_free(TlsContext* t);
#endif
#endif
